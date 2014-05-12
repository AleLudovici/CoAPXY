#ifndef COAP_CLIENT_H_
#define COAP_CLIENT_H_

#include "coap_context.h"
#include "coap_pdu.h"
#include "coap_cache.h"

typedef struct {
	coap_context_t *context; /* A CoAP context */
	void (*response_received)(coap_pdu_t *, void *); /* Function called each time a new response
												   is received and matched with a request. */
	void (*pending_removed)(char *, void *);
	DB *cache_db; /* Cache database. */
	void *data; /* Extra data */
} coap_client_t ;

/**
 * Create a new client object.
 * @param data Extra data to add.
 * @param client The client object returned.
 * @param handler The response handler to call when a new response id received.
 * @return 1 if the connection was successfully created or 0 otherwise.
 */
int coap_client_create(DB *cache_db,
	void *data,
	void (*response_received)(coap_pdu_t *, void *),
	void (*request_removed)(char *, void *), coap_client_t **client);

void coap_client_delete(coap_client_t *client);

/**
 * Waits for an incoming CoAP response to arrive and reads it.
 * @param client The client object used to send CoAP requests.
 */
void coap_client_read(coap_client_t *client);

/**
 * Send a CoAP PDU through the socket.
 * @param client The client object.
 * @param pdu request The CoAP request to be sent.
 * @param to The destination address where to send the request.
 * @param subscription If the request is to establish a subscription.
 */
int coap_client_send(coap_client_t *client,
		coap_pdu_t *request);

/**
 * Matches received responses with sent requests and calls the response
 * handler to pass the CoAP response to upper layers.
 * @param client The client object used to send CoAP requests.
 */
void coap_client_dispatch(coap_client_t *client);

void coap_client_clean_pending(coap_client_t *client);

#endif /* COAP_CLIENT_H_ */
