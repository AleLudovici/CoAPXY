#ifndef COAP_CONTEXT_H_
#define COAP_CONTEXT_H_

#include "coap_list.h"
#include "coap_resource.h"
#include "coap_pdu.h"
#include "../proxy/connection.h"

struct coap_context_t {
	int socket; /* Socket used to send or receive CoAP PDUs. */
	coap_list_t *pending_queue; /* A list with pending CoAP connections.
	 	 	 	 	 	 	 	 in case of a client these are requests that can
	 	 	 	 	 	 	 	 be retransmitted and in case of a server these
	 	 	 	 	 	 	 	 are responses pending to be sent back to the client. */
	coap_list_t *dispatch_queue; /* This queue contains new responses in case of a client
									and new requests in case of a server. */
	unsigned int message_id; /* Message ID used in a new CoAP message created. It is updated
		 	 	 	 	 	 	 when a new CoAP message is created. Each message ID must be unique
		 	 	 	 	 	 	 for all the messages to this resource( which is identified by the
		 	 	 	 	 	 	 previous socket).*/
	unsigned int token; /* Token used in a new CoAP message created. It is updated
		 	 	 	 	 	when a new CoAP message is created. Each token must be unique
		 	 	 	 	 	for all the messages to this resource( which is identified by the
		 	 	 	 	 	previous socket). It can be any thing else than an integer but the
		 	 	 	 	 	CoAP protocol does not care about the format and for simplicity, an
		 	 	 	 		unsigned  integer has been chose.*/
};

typedef struct coap_context_t coap_context_t;

typedef struct {
	coap_pdu_t *pdu; /* Contains the CoAP PDU. */
	unsigned int n_retransmit; /* Counts the number of retransmissions. */
	time_t timestamp; /* When the CoAP PDU was added. */
	unsigned int is_observe; /* Is the connection for a subscription? */
	unsigned int is_separate_response; /* Is waiting for a separate response? */
	time_t last_valid_notification;
	time_t last_observe_value;
	connection_t *connection;
} coap_context_connection_t;


void coap_context_connection_node_delete(struct coap_node_t *node);

/**
 * Create a new context connection.
 * @param context The context to create.
 * @param addr The addr associated with the connection.
 * @param pdu The CoAP PDU associated with the connection.
 * @return 1 if the connection was successfully created or 0 otherwise.
 */
int coap_context_connection_create(coap_context_connection_t **conn);

int coap_context_create(coap_context_t **context);

void coap_context_delete(coap_context_t *context);

/** Reads a CoAP PDUs
 * @param context The context
 */
void coap_context_read(coap_context_t *context);

/**
 * Send a CoAP PDU through the socket.
 * @param client The client object.
 * @param pdu The PDU to send.
 * @param to The destination address.
 */
void coap_context_send(coap_context_t *context,
	coap_pdu_t *pdu);

void coap_context_recover_pdu(coap_context_connection_t *conn,coap_pdu_t *pdu);
#endif /* COAP_CONTEXT_H_ */
