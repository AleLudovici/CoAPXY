#ifndef COAP_SERVER_H_
#define COAP_SERVER_H_

#include "coap_resource.h"
#include "coap_client.h"
#include "coap_context.h"
#include "coap_rd.h"
#include "coap_pdu.h"

enum {
	RD_INSERT,
	RD_UPDATE,
	RD_DELETE
};

typedef struct {
	coap_resource_t *resource;
	coap_list_t *observers;
	unsigned int observe;
	time_t last_timestamp;
} coap_server_observe_t;

typedef struct {
	coap_context_t *context; /* A CoAP context */
	coap_list_t *resources; /* Server resources */
	coap_list_t *observe; /* Server CoAP observe relationship */
	coap_list_t *proxy; /* Proxy requests */
	coap_rd_t *rd; /* Resource discovery */
	int (*rd_update_callback)(int, coap_rd_entry_t *, coap_resource_t *); /* Resource update */
	void (*subscription_new_callback)(coap_resource_t *,coap_pdu_t *); /* Callback for new subcriptions */
	void (*subscription_refresh_callback)(coap_resource_t *,coap_pdu_t *); /* Callback for refreshing subscriptions */
	void (*subscription_delete_callback)(coap_resource_t *,coap_pdu_t *, int); /* Callback for deleting subcriptions */
	coap_client_t *client;
} coap_server_t ;

int coap_observe_request (char *uri);

/**
 * Create a new server object.
 * @param server The server object returned.
 * @param port The port to listen.
 * @return 1 if the connection was successfully created or 0 otherwise.
 */
int coap_server_create(int port, int cache, coap_server_t **server);

void coap_server_delete(coap_server_t *server);

/**
 * Waits for an incoming CoAP requests to arrive and reads them.
 * @param server The server object.
 */
void coap_server_read(coap_server_t *server);

/**
 * Send a CoAP response through the socket.
 * @param server The server object.
 * @param response request The CoAP response to be sent.
 * @param to The destination address where to send the response.
 */
void coap_server_request_send(coap_server_t *server, coap_pdu_t *pdu);

void coap_server_response_send(coap_server_t *server, coap_pdu_t *pdu);

/**
 * Processed all incoming requests.
 * @param server The server object.
 */
void coap_server_dispatch(coap_server_t *server);

void proxy_dispatch(coap_server_t *server);


/**
 * Adds a new resource to the server.
 * @param server The server object.
 * @param resource The resource to be added.
 * @return 1 if the resource was successfully added or 0 otherwise.
 */
int coap_server_resource_add(coap_server_t *server, coap_resource_t *resource);

/**
 * Notifies all observes that with a new response
 * @param server The server object.
 * @param res The updated resource.
 * @param response The CoAP response to be sent.
 */
void coap_server_notify_observers(coap_server_t *server, coap_resource_t *res, coap_pdu_t *response);

void coap_server_clean_pending(coap_server_t *server);

int coap_server_rd_update(coap_server_t *server, coap_rd_entry_t *rd, coap_pdu_t *request);


void bci_resource_handler(coap_pdu_t *request);

#endif /* COAP_SERVER_H_ */
