#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "coap_general.h"
#include "coap_context.h"
#include "coap_pdu.h"
#include "coap_resource.h"

void coap_context_connection_node_delete(struct coap_node_t *node)
{
	coap_context_connection_t *conn;

	conn = (coap_context_connection_t *)node->data;
	coap_pdu_delete(conn->pdu);
	switch (conn->connection->type) {
		case FASTCGI:
			free(conn->connection->data);
			break;
		case WEBSOCKET:
			close(*(((int *)conn->connection->data)));
			break;
		case COAP:
			close(*(((int *)conn->connection->data)));
			break;
		default:
			coap_debug_error("delete: Bad connection provided\n");
			break;
	}
	free(conn->connection);
	free(conn);
}

int coap_context_connection_create(coap_context_connection_t **conn) {

	if ((*conn = (coap_context_connection_t *) malloc (sizeof(coap_context_connection_t)))
		== NULL) {

		return 0;
	}
	memset(*conn, 0, sizeof(coap_context_connection_t));
	(*conn)->connection = NULL;
	(*conn)->pdu = NULL;
	(*conn)->last_observe_value = 0;
	(*conn)->last_valid_notification = 0;
	return 1;
}


int coap_context_create(coap_context_t **context)
{
	if ((*context = (coap_context_t *) malloc (sizeof(coap_context_t)))
		== NULL) {

		return 0;
	}

	if (!coap_list_create(coap_context_connection_node_delete, NULL, &(*context)->pending_queue)
		|| !coap_list_create(NULL, NULL, &(*context)->dispatch_queue)) {

		free(*context);
		return 0;
	}

	/* Initialize token and message ID randomly */
	srand(time(NULL)); 
	(*context)->token = rand();
	(*context)->message_id = rand();
	return 1;
}

void coap_context_delete(coap_context_t *context)
{
	coap_list_delete(context->dispatch_queue);
	coap_list_delete(context->pending_queue);
	free(context);
}

void coap_context_read(coap_context_t *context)
{
	char buffer[COAP_MAX_PDU_SIZE];
	int len;
	coap_pdu_t *pdu;
	struct sockaddr_in6 addr; /* address. */
	socklen_t caddr = sizeof(struct sockaddr_in6);
	//char host[INET6_ADDRSTRLEN];

	memset(buffer, 0, COAP_MAX_PDU_SIZE);

	len = recvfrom(context->socket, buffer, COAP_MAX_PDU_SIZE, 0,
		(struct sockaddr *)&addr, &caddr);

	if (!coap_pdu_create(&pdu)) {
		return;
	}

	if (!coap_pdu_packet_read(buffer, len, pdu)) {
		coap_pdu_delete(pdu);
		return;
	}

	pdu->addr = addr;

   /* inet_ntop(AF_INET6, &(pdu->addr.sin6_addr), host, INET6_ADDRSTRLEN);
    fprintf(stderr,"rcv from : %s\n",host);
   */
	coap_list_node_insert(context->dispatch_queue, &pdu->hdr.id, sizeof(int), pdu);
}



void coap_context_send(coap_context_t *context, coap_pdu_t *pdu) {
	char buffer[COAP_MAX_PDU_SIZE];
	unsigned int len, result;
	coap_context_connection_t *conn;
	coap_option_t *token;

	if ((pdu->hdr.type == COAP_MESSAGE_CON) && (!coap_list_node_get(context->pending_queue, &pdu->hdr.id,
		sizeof(int), (void **)&conn))) {

	/*	if (!coap_context_connection_create(pdu, &conn)) {
			return;
		}
	 */
		if (coap_pdu_option_get(pdu, COAP_OPTION_OBSERVE, NULL))
			conn->is_observe = 1;

		/* Add request to the request connection list
		coap_list_node_insert(context->pending_queue,
				&conn->pdu->hdr.id, sizeof(int), conn);
		*/
	}

	/* Write packet */
	if (!coap_pdu_packet_write(pdu, buffer, &len)) {
		return;
	}
	printf("-->Client Send A Message:\n");

	/* Send PDU */
	result = sendto(context->socket, buffer, len, 0,
			(struct sockaddr *)&pdu->addr,
			sizeof(struct sockaddr_in6));

	if(result == -1){
		coap_debug_error("sendto() went wrong: %s\n", strerror(errno));
	}
}
