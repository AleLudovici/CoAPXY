/*
 * coap_connection.h
 *
 *  Contains function definitions and structs to manipulate connections.
 */

#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "../coap/coap_pdu.h"
#include "../coap/coap_resource.h"


enum {
	WEBSOCKET,
	COAP,
	FASTCGI,
};

typedef struct {
	int type;
	char *path; /* Connection PATH */
	void *data; /* Optional data */
	coap_pdu_t *pdu;
} connection_t;

#define FASTCGI_UNIX_REQUEST(connection) ((FCGX_Request *)((connection)->data))
#define WEBSOCKET_SOCKET(connection) (*((int *)((connection)->data)))
#define COAP_ORIGIN_ADDR(connection) ((struct sockaddr_in6 *)((connection)->data))
#define COAP_SOCKET(connection) (*((int *)((connection)->data+sizeof(struct sockaddr_in6))))

/**
 * Creates a new connection given its type. The resulting connection will contain a
 * fasctcgi_connection_t or socket_connection_type struct depending on value of the
 * parameter type.
 * @param connection The connection created.
 * @param type The type of the connection to be created.
 * @return 1 if the connection is successfully deleted or 0 if not.
 */
int connection_create(int type, connection_t **connection);

/**
 * Deletes a connection node.
 */
void connection_node_delete(struct coap_node_t *node);

/**
 * Deletes an existing connection and frees memory.
 * @param connection The connection to be deleted.
 * @return 1 if the connection is successfully deleted or 0 if not.
 */
int connection_delete(connection_t *connection);

/**
 * Prints the connection content using the standard output.
 * @param connection The connection to be shown.
 */
void connection_print(connection_t *connection);

/**
 * Sends the CoAP requests associated with the connection.
 * @param connection The connection containing the CoAP request.
 * @param res The resource to send the request to.
 * @return 1 if the request is successfully send or 0 if not.
 */
int connection_request_send(connection_t *connection, coap_resource_t *resource);

/**
 * Sends a CoAP response to the origin socket that began the connections.
 * @param connection The origin connection.
 * @param response The CoAP PDU to be sent.
 * @return 1 if the response is successfully send or 0 if not.
 */
int connection_response_send(connection_t *connection, coap_pdu_t *response);

#endif /* COAP_CONNECTION_H_ */
