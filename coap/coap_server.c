#ifndef COAP_SERVER_C_
#define COAP_SERVER_C_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "coap_resource.h"
#include "coap_context.h"
#include "coap_server.h"
#include "coap_net.h"

extern void coap_resource_node_delete(struct coap_node_t *node);
extern void coap_pdu_node_delete(struct coap_node_t *node);

int coap_observe_request(char *uri){
	/* control the presence of the observe request */
	if(strncmp(uri,"observe",strlen("observe")) == 0){
		return 1;
	} else {
		return 0;
	}
}

static int coap_server_observe_create(coap_resource_t *resource, coap_server_observe_t **obs)
{
	if (((*obs) = (coap_server_observe_t *) malloc (sizeof(coap_server_observe_t)))
			== NULL) {
		return 0;
	}
	if (!coap_list_create(coap_pdu_node_delete, NULL, &(*obs)->observers)) {
		free(*obs);
		return 0;
	}
	(*obs)->resource = resource;
	(*obs)->observe = 0;
	(*obs)->last_timestamp = time(NULL);
	return 1;
}

static void coap_server_observe_delete(coap_server_observe_t *obs)
{
	coap_list_delete(obs->observers);
	coap_resource_delete(obs->resource);
	free(obs);
}


static void coap_server_observe_node_delete(struct coap_node_t *node)
{
	coap_server_observe_t *obs;

	obs = (coap_server_observe_t *)node;
	coap_server_observe_delete(obs);
}

static void proxy_response_received(coap_pdu_t *response, void *data)
{
	coap_server_t *server;
	coap_option_t *token, *token_req;
	coap_pdu_t *proxy_pdu;

	server = (coap_server_t *)data;

	coap_pdu_option_get(response, COAP_OPTION_TOKEN, &token);

	if (coap_list_node_get(server->proxy, token->data, token->len, (void **)&proxy_pdu)) {

		if (!coap_pdu_option_get(response, COAP_OPTION_OBSERVE, NULL)) {
			coap_list_node_delete(server->proxy, token->data, token->len);
		}

		response->addr = proxy_pdu->addr;
		response->hdr.id = proxy_pdu->hdr.id;

		coap_pdu_option_get(proxy_pdu, COAP_OPTION_TOKEN, &token_req);
		strcpy(token->data, token_req->data);
		token->len = token_req->len,

		coap_pdu_print(response);
		coap_server_response_send(server, response);
	}
}

static void proxy_response_removed(char *token, void *data) {
	coap_server_t *server;
	coap_pdu_t *pdu;

	server = (coap_server_t *)data;

	if (coap_list_node_get(server->proxy, token, strlen(token), (void **)&pdu)) {
		coap_list_node_delete(server->context->dispatch_queue, &pdu->hdr.id, sizeof(int));
		coap_pdu_delete(pdu);
	}
}

int coap_server_create(int port, int cache, coap_server_t **server)
{

    DB *database;

	if ((*server = (coap_server_t *) malloc (sizeof(coap_server_t)))
			== NULL) {

		return 0;
	}

	if(!coap_context_create(&(*server)->context)) {
		free(*server);
		return 0;
	}

	if ((((*server)->context->socket) = coap_net_udp_bind(port)) == -1) {
		coap_context_delete((*server)->context);
		free(*server);
		return 0;
	}

	if (!coap_rd_create(&(*server)->rd))
	{
		close((*server)->context->socket);
		coap_context_delete((*server)->context);
		free(*server);
		return 0;
	}

	if (!coap_list_create(NULL, NULL, &(*server)->proxy)
			|| !coap_list_create(coap_resource_node_delete, NULL, &(*server)->resources)
			|| !coap_list_create(coap_server_observe_node_delete, NULL, &(*server)->observe)) {

		return 0;
	}

	if (cache) coap_cache_create("/opt/coap_cache", &database);
		else database = NULL;

	if (!coap_client_create(database,
			(void *)(*server),
			proxy_response_received,
			NULL,
			&(*server)->client)) {

		coap_list_delete((*server)->proxy);
		coap_list_delete((*server)->resources);
		coap_list_delete((*server)->observe);
		coap_context_delete((*server)->context);
		free(*server);
		return 0;
	}

	return 1;
}

void coap_server_delete(coap_server_t *server)
{
	coap_context_delete(server->context);
	coap_list_delete(server->resources);
	coap_list_delete(server->proxy);
	free(server);
}

void coap_server_read(coap_server_t *server)
{
	coap_context_read(server->context);
}

void coap_server_request_send(coap_server_t *server, coap_pdu_t *pdu)
{
	char tmp[5];

	pdu->hdr.id = server->context->message_id;
	server->context->message_id = (server->context->message_id + 1) % (int)pow(2,16);

	sprintf(tmp, "%d", server->context->token);
	coap_pdu_option_unset(pdu, COAP_OPTION_TOKEN);
	coap_pdu_option_insert(pdu, COAP_OPTION_TOKEN,tmp,strlen(tmp));
	server->context->token = (server->context->token + 1) % (int)pow(2,16);
	coap_context_send(server->context, pdu);
}

void coap_server_response_send(coap_server_t *server, coap_pdu_t *pdu)
{
	coap_context_send(server->context, pdu);
	if (pdu->hdr.type != COAP_MESSAGE_CON) {
		coap_list_node_delete(server->context->dispatch_queue, &pdu->hdr.id, sizeof(int));
	}
}

static int check_options(coap_pdu_t *pdu)
{
	coap_option_t *opt;
	coap_list_index_t *li;

	if (pdu->opt_list != NULL) {
		for (li = coap_list_first(pdu->opt_list);li;li = coap_list_next(li)) {
			coap_list_this(li,NULL,(void **)&opt);
			switch (opt->code) {
			case COAP_OPTION_CONTENT_TYPE :
			case COAP_OPTION_ETAG :
			case COAP_OPTION_MAXAGE :
			case COAP_OPTION_PROXY_URI :
			case COAP_OPTION_TOKEN :
			case COAP_OPTION_URI_PATH :
			case COAP_OPTION_URI_QUERY :
			case COAP_OPTION_OBSERVE:
				break;
			default: /* unrecognized option*/
				if (opt->code & 0x01) {
					/* do not ignore if it is critical.*/
					return 0;
				}
				break;
			}
		}
	}
	return 1;
}


static resource_filter(coap_option_t *query, coap_rd_entry_t *entry, char *data, int *len)
{
	coap_list_index_t *li_rd, *li_res;
	coap_resource_t *res;
	int filtered_res = 1;
	char *var;

	for (li_res = coap_list_first(entry->observers);li_res; li_res = coap_list_next(li_res)) {
		coap_list_this(li_res, NULL,(void **) &res);

		if (query) {

			if (coap_pdu_parse_query(query->data, "rt", query->len, &var)
				&& ((res->rt && strcmp(res->rt, var) != 0) || res->rt == NULL)) {

				filtered_res = 0;
			}

			if (coap_pdu_parse_query(query->data, "if", query->len, &var)
				&& ((res->ifd && strcmp(res->ifd, var) != 0) || res->ifd == NULL)) {

				filtered_res = 0;
			}

			if (coap_pdu_parse_query(query->data, "sz", query->len, &var)
				&& res->sz != atoi(var)) {

				filtered_res = 0;
			}
		}
		if (filtered_res) {
			coap_resource_link_format(res, data, 1, len);
			sprintf(data, "%s,", data);
		}

		filtered_res = 1;
	}
}

void coap_server_dispatch(coap_server_t *server) {
	coap_context_t *context;
	coap_list_index_t *li, *li_subs;
	coap_option_t *token, *path, *proxy_uri;
	coap_pdu_t *request;
	coap_resource_t *res;
	coap_server_observe_t *obs;
	coap_context_connection_t *conn;

	context = server->context;


	for (li = coap_list_first(context->dispatch_queue);li; li = coap_list_next(li)) {
		coap_list_this(li, NULL,(void **) &request);

		if (!coap_pdu_option_get(request, COAP_OPTION_TOKEN, &token)) {
			coap_debug_error("Couldn't get the token\n");
			coap_list_node_delete(context->dispatch_queue, &request->hdr.id, sizeof(int));
			coap_pdu_delete(request);
			continue;
		}

		if (request->hdr.type == COAP_MESSAGE_CON && !check_options(request)) {
			coap_debug_error("Check option fail\n");
			coap_pdu_clean_common_options(request);
			request->hdr.type= COAP_MESSAGE_ACK;
			request->hdr.code = COAP_RESPONSE_402;
			coap_server_response_send(server, request);
			continue;
		}

		switch(request->hdr.type) {
		case COAP_MESSAGE_ACK:
			if (coap_list_node_get(context->pending_queue, &request->hdr.id, sizeof(int), (void **)&conn)) {
				coap_list_node_delete(context->pending_queue, &request->hdr.id, sizeof(int));
			}
			coap_list_node_delete(context->dispatch_queue, &request->hdr.id, sizeof(int));
			coap_pdu_delete(request);
			continue;
		case COAP_MESSAGE_RST:
			/* A RST PDU message for a CoAP resource ends an existing
			 * observe relationship. */
			if(coap_list_node_get(context->pending_queue, token->data, strlen(token->data), (void **)&conn)) {
				if (conn->is_observe) {

					/* End relationship: find which was the PDU that caused an error. */
					for (li_subs = coap_list_first(server->observe);li_subs; li_subs = coap_list_next(li_subs)) {
						coap_list_this(li_subs, NULL,(void **) &obs);
						coap_list_node_delete(obs->observers, &conn->pdu->hdr.id, sizeof(int));
					}
					/* Remove from pending list */
					coap_list_node_delete(context->pending_queue, token->data, strlen(token->data));
				}
			}
			coap_list_node_delete(context->dispatch_queue, &request->hdr.id, sizeof(int));
			coap_pdu_delete(request);
			continue;
		case COAP_MESSAGE_CON:
		case COAP_MESSAGE_NON:

			if (coap_pdu_option_get(request, COAP_OPTION_PROXY_URI, &proxy_uri)) {
				char *uri_path, *uri_query;
				struct sockaddr_in6 to;
				coap_pdu_t *pdu;
				coap_option_t *token;
				int observe = 0;


				/* Parsing CoAP URI */
				if (!coap_pdu_parse_uri(proxy_uri->data, &to, &uri_path, &uri_query)) {
					coap_list_node_delete(context->dispatch_queue, &request->hdr.id, sizeof(int));
					coap_pdu_delete(request);
					continue;
				}

				coap_pdu_create(&pdu);
				coap_pdu_copy(request, pdu);

				pdu->addr = to;

				coap_pdu_option_unset(pdu, COAP_OPTION_PROXY_URI);

				coap_pdu_option_insert(pdu, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));

				if (uri_query) {
					coap_pdu_option_insert(pdu, COAP_OPTION_URI_QUERY, uri_query, strlen(uri_query));
				}

				if (coap_pdu_option_get(pdu, COAP_OPTION_OBSERVE, NULL)) {
					observe = 1;
				}

				/* Send response */
				if (!coap_client_send(server->client, pdu)) {

					pdu->hdr.id = request->hdr.id;
					pdu->addr = request->addr;

					coap_pdu_option_unset(pdu, COAP_OPTION_TOKEN);
					coap_pdu_option_get(request, COAP_OPTION_TOKEN, &token);
					coap_pdu_option_insert(pdu, COAP_OPTION_TOKEN, token->data, strlen(token->data));

					coap_server_response_send(server, pdu);
				} else {
					coap_pdu_option_get(pdu, COAP_OPTION_TOKEN, &token);
					coap_list_node_insert(server->proxy, token->data, token->len, request);
				}

				coap_list_node_delete(context->dispatch_queue, &request->hdr.id, sizeof(int));
				continue;
			}

			if (!coap_pdu_option_get(request, COAP_OPTION_URI_PATH, &path)) {
				coap_debug_error("No Uri Path\n");
				coap_list_node_delete(context->dispatch_queue, &request->hdr.id, sizeof(int));
				coap_pdu_delete(request);
				continue;
			}


			/* Check out whether the request matches a resource's alias in the case no subscription is found */
			if (!coap_list_node_get(server->observe, path->data, strlen(path->data), (void **)&obs)){

			}

			if(obs){
				coap_pdu_t *pdu;
				int found = 0;
				char host_pdu[INET6_ADDRSTRLEN], host_request[INET6_ADDRSTRLEN];
				for (li_subs = coap_list_first(obs->observers);li_subs; li_subs = coap_list_next(li_subs)) {
					coap_list_this(li_subs, NULL,(void **) &pdu);

					inet_ntop(AF_INET6, &(pdu->addr.sin6_addr), host_pdu, INET6_ADDRSTRLEN);
					inet_ntop(AF_INET6, &(request->addr.sin6_addr), host_request, INET6_ADDRSTRLEN);

					if (strcmp(host_pdu, host_request) == 0 && pdu->addr.sin6_port == request->addr.sin6_port) {
						pdu->timestamp = time(NULL);
						found = 1;
						break;
					}
				}

				if (coap_pdu_option_get(request, COAP_OPTION_OBSERVE, NULL)) {
					/* if it is a observe request... */
					if (!found) {
						/* a new request --> create observe relationship */
						coap_pdu_create(&pdu);
						coap_pdu_copy(request, pdu);
						/* Add the new request to the observers list. */
						printf("New Observe Relationship\n");
						coap_list_node_insert(obs->observers, &pdu->hdr.id, sizeof(int), pdu);
						if (server->subscription_new_callback)
							server->subscription_new_callback(obs->resource, request);

					}else{
						printf("Observe renewed\n");
						if (server->subscription_refresh_callback)
							server->subscription_refresh_callback(obs->resource, request);
					}
				} else if (found) {
					/* Termination request --> End observer relationship */
					printf("Ending Observe\n");
					coap_list_node_delete(obs->observers, &pdu->hdr.id, sizeof(int));

					if (server->subscription_delete_callback)
						server->subscription_delete_callback(obs->resource, request, 0);

					if(!obs->observers->first){
						/* it was the last observer of this resource */
						if (server->subscription_delete_callback)
							server->subscription_delete_callback(obs->resource, NULL, 0);
					}

				}else{
					/* one-time request --> handle regular GET */
					printf("ONE-TIME GET\n");
					obs->resource->handler(request);
				}

				/* Send the response */
				coap_server_response_send(server, request);
				continue;
			}

			if (strcmp(path->data, ".well-known/core") == 0) {
				coap_resource_t *resource;
				coap_server_observe_t *observing;
				coap_list_index_t *li;
				coap_option_t *query;
				int filtered_res = 1;
				char *var;

				if (!coap_pdu_option_get(request, COAP_OPTION_URI_QUERY, &query)) {
					query = NULL;
				}

				switch (request->hdr.code) {
					case COAP_REQUEST_GET:
						request->hdr.type = COAP_MESSAGE_ACK;
						request->hdr.code = COAP_RESPONSE_205;

						if (query) {
							if (!coap_pdu_parse_query(query->data, "rt", query->len, &var)
								|| strcmp("core-rd", var) == 0) {

								sprintf(request->payload, "</%s>;rt=core-rd;ins=\"Primary\",", COAP_RD_BASE_DIR);
								request->payload_len = strlen(request->payload);
							}
						}

						for (li = coap_list_first(server->resources);li;li = coap_list_next(li)) {
							coap_list_this(li,NULL,(void **)&resource);

							if (query) {
								if (coap_pdu_parse_query(query->data, "rt", query->len, &var)
									&& strcmp(resource->rt, var) != 0) {

									filtered_res = 0;
								}
								if (coap_pdu_parse_query(query->data, "if", query->len, &var)
									&& strcmp(resource->ifd, var) != 0) {

									filtered_res = 0;
								}

								if (coap_pdu_parse_query(query->data, "sz", query->len, &var)
									&& resource->sz != atoi(var)) {

									filtered_res = 0;
								}
							}

							if (filtered_res) {
								coap_resource_link_format(resource, request->payload, 0, &request->payload_len);
							}
							filtered_res = 1;
						}

						for (li = coap_list_first(server->observe);li;li = coap_list_next(li)) {
							coap_list_this(li,NULL,(void **)&observing);

							if (query) {
								if (coap_pdu_parse_query(query->data, "rt", query->len, &var)
									&& strcmp(observing->resource->rt, var) != 0) {

									filtered_res = 0;
								}
								if (coap_pdu_parse_query(query->data, "if", query->len, &var)
									&& strcmp(observing->resource->ifd, var) != 0) {

									filtered_res = 0;
								}

								if (coap_pdu_parse_query(query->data, "sz", query->len, &var)
									&& observing->resource->sz != atoi(var)) {

									filtered_res = 0;
								}
							}

							if (filtered_res) {
								coap_resource_link_format(observing->resource, request->payload, 0, &request->payload_len);
							}
							filtered_res = 1;
						}

						break;
					default:
						request->hdr.type = COAP_MESSAGE_ACK;
						request->hdr.code = COAP_RESPONSE_405;
						break;
				}
				coap_pdu_clean_common_options(request);
				coap_server_response_send(server, request);
				continue;
			} else if (strncmp(path->data, COAP_RD_BASE_DIR, strlen(COAP_RD_BASE_DIR)) == 0) {
				coap_rd_entry_t *rd, *entry;
				coap_option_t *query, *content_type;
				coap_list_index_t *li_rd;
				char *var;
				char full_path[64+strlen(COAP_RD_BASE_DIR)];

				switch (request->hdr.code) {
					case COAP_REQUEST_POST:
						if (!coap_pdu_option_get(request, COAP_OPTION_URI_QUERY, &query)
							|| !coap_pdu_option_get(request, COAP_OPTION_CONTENT_TYPE, &content_type)) {
							/* Bad request */
							request->hdr.code = COAP_RESPONSE_400;
							sprintf(request->payload, "%s", "Bad request");
							request->payload_len = strlen("Bad request");
							goto rd_send;
						}

						if (atoi(content_type->data) != COAP_MEDIATYPE_LINK_FORMAT) {
							/* Bad request */
							request->hdr.code = COAP_RESPONSE_400;
							sprintf(request->payload, "%s", "Not valid content type");
							request->payload_len = strlen("Not valid content type");
							goto rd_send;
						}

						/* Parse query: look for name parameter and create new entry. */
						if (!coap_pdu_parse_query(query->data, "ep", query->len, &var)) {
							/* Bad request */
							request->hdr.code = COAP_RESPONSE_400;
							sprintf(request->payload, "%s", "Host parameter not found.");
							request->payload_len = strlen("Host parameter not found.");
							goto rd_send;
						}

						/* If the request contains is of type POST add the resources sent. If
						 * resource were previously sent, then delete them if is a POST request.
						 * In this case the request should have been of type PUT.
						*/
						if (!coap_rd_entry_create(var, &rd)) {
							/* Bad request */
							request->hdr.code = COAP_RESPONSE_400;
							sprintf(request->payload, "%s", "Bad query string");
							request->payload_len = strlen("Bad query string");
							goto rd_send;
						}

						/* Optional parameters DA RIVEDERE */
						if (coap_pdu_parse_query(query->data, "lt", query->len, &var)) {
							rd->lt = atoi(var);
						}

						if (coap_pdu_parse_query(query->data, "rt", query->len, &var)) {
							sprintf(rd->rt, "%s", var);
						}

						/* Update resources */
						if (!coap_server_rd_update(server, rd, request)) {
							/* Bad request */
							request->hdr.code = COAP_RESPONSE_400;
							sprintf(request->payload, "%s", "Bad payload");
							request->payload_len = strlen("Bad payload");
							goto rd_send;
						}
						coap_rd_entry_add(server->rd, rd);

						request->hdr.code = COAP_RESPONSE_201;
						sprintf(full_path, "%s/%s", COAP_RD_BASE_DIR, rd->coap_res->path);
						coap_pdu_option_insert(request, COAP_OPTION_LOCATION_PATH, full_path, strlen(full_path));
						request->payload_len = 0;
						break;
					case COAP_REQUEST_GET:

						memset(request->payload, 0, COAP_MAX_PDU_SIZE);

						if (!coap_pdu_option_get(request, COAP_OPTION_URI_QUERY, &query)) {
							query = NULL;
						}

						request->hdr.type = COAP_MESSAGE_ACK;
						request->hdr.code = COAP_RESPONSE_205;


						if (strlen(path->data) > strlen(COAP_RD_BASE_DIR) + 1) {
							if (coap_list_node_get(server->rd->entries, &path->data[strlen(COAP_RD_BASE_DIR) + 1],
								strlen(&path->data[strlen(COAP_RD_BASE_DIR) + 1]), (void **)&entry)) {

								resource_filter(query, entry, request->payload, &request->payload_len);
							}
						} else {
							/* Applying filters */
							for (li_rd = coap_list_first(server->rd->entries);li_rd; li_rd = coap_list_next(li_rd)) {
								coap_list_this(li_rd, NULL,(void **) &entry);

								resource_filter(query, entry, request->payload, &request->payload_len);
							}
						}
						break;
					case COAP_REQUEST_DELETE:

						if (coap_list_node_get(server->rd->entries, &path->data[strlen(COAP_RD_BASE_DIR) + 1],
							strlen(&path->data[strlen(COAP_RD_BASE_DIR) + 1]), (void **)&rd)) {
							coap_list_index_t *li_res;
							coap_resource_t *res;

							for (li_res = coap_list_first(rd->observers);li_res; li_res = coap_list_next(li_res)) {
								coap_list_this(li_res, NULL,(void **) &res);

								if (server->rd_update_callback)
									server->rd_update_callback(RD_DELETE, rd, res);

							}

							coap_list_node_delete(server->rd->entries, &path->data[strlen(COAP_RD_BASE_DIR) + 1],
									strlen(&path->data[strlen(COAP_RD_BASE_DIR) + 1]));
#ifdef NDEBUG
							printf("Directory entry %s deleted.\n", path->data);
#endif
							request->hdr.type = COAP_MESSAGE_ACK;
							request->hdr.code = COAP_RESPONSE_202;
							request->payload_len = 0;
						}
						break;
					case COAP_REQUEST_PUT:

						if (coap_list_node_get(server->rd->entries, &path->data[strlen(COAP_RD_BASE_DIR) + 1],
							strlen(&path->data[strlen(COAP_RD_BASE_DIR) + 1]), (void **)&rd)) {

							if (!coap_pdu_option_get(request, COAP_OPTION_URI_QUERY, &query)) {
								query = NULL;
							}

							/* Optional parameters */
							if (query && coap_pdu_parse_query(query->data, "lt", query->len, &var)) {
								rd->lt = atoi(var);
							}

							if (!coap_server_rd_update(server, rd, request)) {
								/* Bad request */
								request->hdr.code = COAP_RESPONSE_400;
								sprintf(request->payload, "%s", "Bad payload");
								request->payload_len = strlen("Bad payload");
								goto rd_send;
							}
						}

						request->hdr.type = COAP_MESSAGE_ACK;
						request->hdr.code = COAP_RESPONSE_204;
						request->payload_len = 0;
						break;
					default:
						break;
				}
rd_send:
				request->hdr.type = COAP_MESSAGE_ACK;
				coap_pdu_option_unset(request, COAP_OPTION_URI_PATH);
				coap_pdu_option_unset(request, COAP_OPTION_URI_QUERY);
				coap_pdu_option_unset(request, COAP_OPTION_CONTENT_TYPE);
				coap_server_response_send(server, request);
			} else {
				coap_debug_error("Generic Request\n");
				if (!coap_list_node_get(server->resources, path->data, strlen(path->data),
						(void **)&res)) {

					coap_list_node_delete(context->dispatch_queue, &request->hdr.id, sizeof(int));
					coap_pdu_delete(request);
					continue;
				}
				res->handler(request);
			}

			coap_server_response_send(server, request);
			continue;
		}
	}
}

int coap_server_resource_add(coap_server_t *server, coap_resource_t *resource)
{
	if (resource->is_observe) {
		coap_server_observe_t *obs;
		coap_server_observe_create(resource, &obs);
		coap_list_node_insert(server->observe, resource->path, strlen(resource->path), obs);
	} else {
		coap_list_node_insert(server->resources, resource->path, strlen(resource->path), resource);
	}

	return 1;
}

void coap_server_notify_observers(coap_server_t *server, coap_resource_t *res, coap_pdu_t *response)
{
	coap_list_index_t *li;
	coap_server_observe_t *obs;
	coap_pdu_t *pdu;
	coap_option_t *token;
	char observe_value[5];

	if (coap_list_node_get(server->observe, res->path, strlen(res->path), (void **)&obs)) {
		/* If a response code 4.xx or 5.xx is sent
		 * end all subscriptions, otherwise notify all
		 * observers. */

		if (response->hdr.code > 100) {
			coap_list_delete(obs->observers);
			printf("response code is 100\n");
		} else {

			for (li = coap_list_first(obs->observers);li; li = coap_list_next(li)) {
				coap_list_this(li, NULL,(void **) &obs);

				coap_pdu_print(pdu);
				coap_pdu_option_get(pdu, COAP_OPTION_TOKEN, &token);
				if (coap_pdu_option_get(response, COAP_OPTION_TOKEN, NULL)) {

					coap_pdu_option_unset(response, COAP_OPTION_TOKEN);
				}


				coap_pdu_option_insert(response, COAP_OPTION_TOKEN, token->data, strlen(token->data));


				obs->observe = (obs->observe + (time(NULL) - obs->last_timestamp)) % ((int)pow(2,16));
				obs->last_timestamp = time(NULL);

				sprintf(observe_value, "%d", obs->observe);
				coap_pdu_option_insert(response, COAP_OPTION_OBSERVE, observe_value, strlen(observe_value));

				response->addr = pdu->addr;

				coap_server_response_send(server, response);
			}
		}

	}

}

void coap_server_clean_pending(coap_server_t *server){
	coap_list_index_t *li, *li_subs;
	coap_server_observe_t *obs;
	coap_context_connection_t *conn;
	coap_option_t *token, *max_age;
	coap_context_t *context;
	coap_pdu_t *pdu;

	context = server->context;

	coap_client_clean_pending(server->client);

	for (li = coap_list_first(context->pending_queue);li; li = coap_list_next(li)) {
		coap_list_this(li, NULL,(void **) &conn);

		coap_pdu_option_get(conn->pdu, COAP_OPTION_TOKEN, &token);

		if(!conn->is_observe && !conn->is_separate_response
				&& ((time(NULL) - conn->timestamp - (conn->n_retransmit+1)*1000) >= 0)) {
			if (conn->n_retransmit + 1 > MAX_RETRANSMIT) {
				coap_list_node_delete(context->pending_queue, &conn->pdu->hdr.id, sizeof(int));
#ifdef NDEBUG
				fprintf(stderr,"Response removed\n");
#endif
			} else {
				conn->timestamp = time(NULL);
				conn->n_retransmit++;
#ifdef NDEBUG
				fprintf(stderr,"Re-sending client response...\n");
#endif
				coap_context_send(context, conn->pdu);
			}
		}
	}
	for (li_subs = coap_list_first(server->observe);li_subs; li_subs = coap_list_next(li_subs)) {
		coap_list_this(li_subs, NULL,(void **) &obs);

		for (li = coap_list_first(obs->observers);li; li = coap_list_next(li)) {
			int maxage;

			coap_list_this(li, NULL,(void **) &pdu);

			maxage = coap_pdu_option_get(pdu, COAP_OPTION_MAXAGE, &max_age) ? atoi(max_age->data) : COAP_DEFAULT_MAX_AGE;

			fprintf(stderr,"MAXAGE: %d\n", maxage);
			//fprintf(stderr,"TIMESTAMP: %d\n", pdu->timestamp);

			/*coap_pdu_print(pdu);*/
			if ((time(NULL) - pdu->timestamp - maxage) >= 0) {

				if (server->subscription_delete_callback)
					server->subscription_delete_callback(obs->resource,pdu,1);

				coap_list_node_delete(obs->observers, &pdu->hdr.id, sizeof(int));
#ifdef NDEBUG
				fprintf(stderr,"Deleted observing\n");
#endif
				if(!obs->observers->first){
#ifdef NDEBUG
					printf("No more observers, lets eliminate the inner observing\n");
#endif
					if (server->subscription_delete_callback)
						server->subscription_delete_callback(obs->resource,NULL,1);
				}
			}
		}
	}
}

int coap_server_rd_update(coap_server_t *server, coap_rd_entry_t *rd, coap_pdu_t *request)
{
	coap_resource_t *resource;
	char *p, *q;
	char *path = NULL, *rt = NULL, *ifd = NULL, *sz = NULL;
	int sz_d = 0, sz_path = 0;
	int more = 1, i = 0;

	p = request->payload;
    q = request->payload;

    /* Parse the payload following the format of Link Mediatype*/

    while (i < request->payload_len){
		/* PATH */
		while (*p != '>' && more){
			p++;
			i++;
		}
            if(*p == '>'){
            	q++;
            	path = (char *)malloc(abs(p - q - 1));
            	memcpy(path, q, (p - q));
            	path[p - q] = '\0';
            }
			/* ATTRIBUTES */
			while (*p != ';' && *p != ','){
				p++;
				i++;
			}
				if(*p == ';'){
					more = 0;
					q = p;
					p++;
					i++;
					if (strncmp(q, ";rt=", strlen(";rt=")) == 0){
						/* RT */
						q += strlen(";rt=");
						while (*p != ';' && *p != ',') p++;
						rt = (char *)malloc(abs(p - q - 1));
						memcpy(rt, q, abs(p - q));
						rt[p - q] = '\0';
					} else if (strncmp(q, ";if=", strlen(";if=")) == 0){
						/* IF */
						q += strlen(";if=");
						while (*p != ';' && *p != ',') p++;
						ifd = (char *)malloc(abs(p - q - 1));
						memcpy(ifd, q, abs(p - q));
						ifd[p - q] = '\0';
					} else if (strncmp(q, ";ct=", strlen(";ct=")) == 0){
						/* SZ */
						q += strlen(";sz=");
						while (*p != ';' && *p != ',') p++;
						sz = (char *)malloc(abs(p - q - 1));
						memcpy(sz, q, abs(p - q - 1));
						sz[p - q] = '\0';
					}
				} else if (*p == ','){
					more = 1;
				}
		}

    /* ADD To The Resource Directory*/

		if (!coap_resource_create(path, path, rt, ifd, sz_d, 0, NULL, &resource))
			return 0;

		resource->server_addr = request->addr;

		rd->coap_res = resource;

	return 1;
}


#endif /* COAP_SERVER_C_ */
