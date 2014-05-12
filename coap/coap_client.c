#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "coap_net.h"
#include "coap_client.h"


int coap_client_create(DB *cache_db,
		void *data,
		void (*response_received)(coap_pdu_t *, void *),
		void (*request_removed)(char *, void *),
		coap_client_t **client)
{
	if ((*client = (coap_client_t *) malloc (sizeof(coap_client_t)))
		== NULL) {

		return 0;
	}

	if(!coap_context_create(&(*client)->context)) {

		free(*client);
		return 0;
	}

	(*client)->response_received = response_received;
	(*client)->pending_removed = request_removed;

	/* Create UDP socket */
	if ((((*client)->context->socket) = coap_net_udp_create()) == -1) {
		coap_list_delete((*client)->context->pending_queue);
		coap_list_delete((*client)->context->dispatch_queue);
		free((*client)->context);
		free(*client);
		return 0;
	}

	(*client)->data = data;
	(*client)->cache_db = cache_db;

	return 1;
}

void coap_client_delete(coap_client_t *client)
{
	coap_context_delete(client->context);
}

void coap_client_read(coap_client_t *client)
{
	coap_context_read(client->context);
}

int coap_client_send(coap_client_t *client, coap_pdu_t *pdu)
{
	char tmp[(int)pow(2,16)];

		if (pdu->hdr.type != COAP_MESSAGE_ACK){
			pdu->hdr.id = client->context->message_id;
			client->context->message_id = (client->context->message_id + 1) % (int)pow(2,16);

			sprintf(tmp, "%d", client->context->token);
			coap_pdu_option_insert(pdu, COAP_OPTION_TOKEN,tmp, strlen(tmp));
			client->context->token = (client->context->token + 1) % (int)pow(2,16);
		}

		coap_context_send(client->context, pdu);
	
	return 1;
}

/*static void update_cache(coap_client_t *client, coap_pdu_t *request,
	coap_pdu_t *response)
{

	switch (response->hdr.code) {
		case COAP_RESPONSE_203 :
			coap_pdu_clean(response);
			coap_cache_revalidate(client->cache_db, request, &response);
			break;
		case COAP_RESPONSE_205 :
			response->timestamp = time(NULL);
			coap_cache_add(client->cache_db, request, response);
			break;
		default:
			if (coap_cache_get(client->cache_db, request, &response)) {
				coap_cache_delete(client->cache_db, response);
			}
			break;
	}
}*/

void coap_client_dispatch(coap_client_t *client) {
	coap_context_t *context;
	coap_pdu_t *response;
	coap_context_connection_t *conn;
	coap_option_t *token_req, *token_res, *observe;
	coap_list_index_t *li_disp, *li_pen;

	context = client->context;

	for (li_disp = coap_list_first(context->dispatch_queue);li_disp; li_disp = coap_list_next(li_disp)) {
		coap_list_this(li_disp, NULL,(void **) &response);
		//coap_debug_error("--> Message received by Client:\n");
		//coap_pdu_print(response);

		/* Get the Token contained in the response */
		if (!coap_pdu_option_get(response, COAP_OPTION_TOKEN, &token_res)) {
			coap_list_node_delete(context->dispatch_queue, &response->hdr.id, sizeof(int));
			coap_pdu_delete(response);
			continue;
		}

		for (li_pen = coap_list_first(context->pending_queue);li_pen; li_pen = coap_list_next(li_pen)) {
			coap_list_this(li_pen, NULL,(void **) &conn);

			if (!coap_pdu_option_get(conn->pdu, COAP_OPTION_TOKEN, &token_req)) {
				coap_list_node_delete(context->pending_queue, &conn->pdu->hdr.id, sizeof(int));
				continue;
			}

			if (strcmp(token_req->data, token_res->data) == 0) {
				switch (response->hdr.type) {
					case COAP_MESSAGE_ACK:
						if (conn->pdu->hdr.id == response->hdr.id) {
							if (response->hdr.code != 0) {
								/* Look for an Observe option. If it is found, it means
								 * that a subscription relationship has been established
								 * with the server. Then simply mark the request as subscription.*/
								client->response_received(response, client->data);
								if (coap_pdu_option_get(response, COAP_OPTION_OBSERVE, &observe)) {
									conn->is_observe = 1;
								} else {
									/* Piggy-backed response: remove from connections list. */
									coap_debug_error("piggy backed\n");
									if (client->pending_removed)
										client->pending_removed(token_res->data, client->data);
									coap_list_node_delete(context->pending_queue, &response->hdr.id, sizeof(unsigned int));
									coap_debug_error("node_delete\n");
								}
							} else {
								/* Separate response: preserve the request in the request list
								 * and mark the connection to not re-send the request. */
								conn->is_separate_response = 1;
							}
						}
						break;
					case COAP_MESSAGE_RST:
						/* Re-send the CoAP request */
						/* coap_client_send(client, conn->pdu, conn->is_obs); */
						break;
					case COAP_MESSAGE_CON:
					case COAP_MESSAGE_NON:
						/* Separate response */
						if (!conn->is_observe) {
							/* remove from connections if its not from a subscription. */
							if (client->pending_removed)
								client->pending_removed(token_res->data, client->data);
							coap_list_node_delete(context->pending_queue, &conn->pdu->hdr.id,
								sizeof(unsigned int));
						} else {
							coap_debug_error("rec: %s\n", response->payload);
							if (!coap_pdu_option_get(response, COAP_OPTION_OBSERVE, &observe)) {
								continue;
							}

							// Update last observe value and last valid notification
							conn->last_observe_value = atoi(observe->data);
							conn->last_valid_notification = time(NULL);
						}
						client->response_received(response, client->data);

						//Send ACK
						response->hdr.type = COAP_MESSAGE_ACK;
						response->hdr.code = COAP_RESPONSE_205;

					    memset(response->payload, 0, COAP_MAX_PDU_SIZE);
					    response->payload_len = 0;

						coap_client_send(client, response);


						break;
					default:
						continue;
				}
			} else {
				coap_debug_error("Token does not match\n");
			}
			break;
		}
		coap_list_node_delete(context->dispatch_queue, &response->hdr.id, sizeof(int));
	}
}

void coap_client_clean_pending(coap_client_t *client)
{
	coap_list_index_t *li;
	coap_context_connection_t *conn;
	coap_option_t *token;
	coap_context_t *context;

	context = client->context;

	for (li = coap_list_first(context->pending_queue);li; li = coap_list_next(li)) {
		coap_list_this(li, NULL,(void **) &conn);

		if(!conn->is_observe && !conn->is_separate_response
			&& ((time(NULL) - conn->timestamp + (conn->n_retransmit+1)*1000) >= 0)) {
			if (conn->n_retransmit + 1 > MAX_RETRANSMIT) {
				

				coap_pdu_option_get(conn->pdu, COAP_OPTION_TOKEN, &token);

				if (client->pending_removed)
					client->pending_removed(token->data, client->data);

				coap_list_node_delete(context->pending_queue, token->data, strlen(token->data));
#ifdef NDEBUG
				fprintf(stderr,"Request removed\n");
#endif
			} else {
				conn->timestamp = time(NULL);
				conn->n_retransmit++;
#ifdef NDEBUG
				fprintf(stderr,"Resending request...\n");
#endif
				coap_context_send(context, conn->pdu);
			}
		}
	}
}
