#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resource.h"
#include "connection.h"

extern void connection_node_delete(struct coap_node_t *node);

/* When a valid PDU is received. */
static void response_received(coap_pdu_t *response, void *data)
{
	coap_list_t *connections;
	connection_t *conn;
	coap_option_t *token;
	coap_list_index_t *li;

	connections = (coap_list_t *)data;

	coap_pdu_option_get(response, COAP_OPTION_TOKEN, &token);

	if (coap_list_node_get(connections, token->data, strlen(token->data), (void **)&conn)) {

		connection_response_send(conn, response);
		if (conn->type == FASTCGI) {
			coap_list_node_delete(connections, token->data, strlen(token->data));
		}
	}
}

/* When a PDU is removed from the pending list */
static void request_removed(char *token, void *data) {
	coap_list_t *connections;
	connection_t *conn;

	connections = (coap_list_t *)data;

	if (coap_list_node_get(connections, token, strlen(token), (void **)&conn)) {
		coap_list_node_delete(connections, token, strlen(token));
	}
}

void resource_node_delete(struct coap_node_t *node) {
	resource_t *res;

	res = (resource_t *)node;
	resource_delete(res);
}

int resource_create(char *name, coap_resource_t *resource, resource_t **res) {
	DB *database;


	if (((*res) = (resource_t *) malloc (sizeof(resource_t)))
			== NULL) {

		return 0;
	}
	if (!coap_list_create(connection_node_delete, NULL, &(*res)->connections)) {

		free(*res);
		return 0;
	}

	if (strcmp(name, "/test") == 0) {
		fprintf(stderr,"name %s\n", name);
		database = NULL;
	} else {
		coap_cache_create("/tmp/resource_cache", &database);
	}


	if (!coap_client_create(database,
			(void *)(*res)->connections,
			response_received,
			request_removed,
			&(*res)->client)) {

		coap_list_delete((*res)->connections);
		free(*res);
		return 0;
	}

	if (((*res)->path = (char *) malloc (sizeof(name))) == NULL) {
		coap_list_delete((*res)->connections);
		coap_client_delete((*res)->client);
		free(*res);
		return 0;
	}
	sprintf((*res)->path,"%s", name);

	(*res)->resource = resource;

	return 1;
}

void resource_delete(resource_t *resource) {
	coap_list_delete(resource->connections);
	coap_client_delete(resource->client);
	free(resource);
}

