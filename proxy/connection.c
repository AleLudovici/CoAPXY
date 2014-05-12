#include <stdio.h>
#include <stdlib.h>
#include "fcgi/fcgiapp.h"
#include "connection.h"
#include "websocket.h"

int connection_create(int type, connection_t **connection) {

	if ((*connection = (connection_t *)malloc(sizeof(connection_t)))
		== NULL) {

		coap_debug_error("Error creating a new connection\n");
		return 0;
	}

	switch (type) {
		case FASTCGI:
			/* Reserve space for the FCGX_Request struct */
			if (((*connection)->data = (FCGX_Request *) malloc (sizeof(FCGX_Request)))
			   	== NULL) {

			   	coap_debug_error("FCGX_Request cannot be created\n");
			   	free(connection);
			    return 0;
			}
			break;
		case WEBSOCKET:
			if (((*connection)->data = (int *) malloc (sizeof(int)))
			   	== NULL) {

			   	coap_debug_error("WebSocket integer cannot be created\n");
			   	free(connection);
			    return 0;
			}


		case COAP:
			break;
		default:
			coap_debug_error("Wrong connection type provided\n");
			return 0;
	}

	(*connection)->type = type;

	return 1;
}

int connection_delete(connection_t *connection)
{
	switch (connection->type) {
		case FASTCGI:
			free(connection->data);
			break;
		case WEBSOCKET:
			close(*(((int *)connection->data)));
			break;
		case COAP:
			close(*(((int *)connection->data)));
			break;
		default:
			coap_debug_error("delete: Bad connection provided\n");
			return 0;
	}
	free(connection);
	return 1;
}

void connection_node_delete(struct coap_node_t *node)
{
	connection_t *conn;

	conn = (connection_t *)node->data;
	connection_delete(conn);
}

void connection_print(connection_t *connection)
{
	char string[10];

	switch (connection->type) {
		case FASTCGI:
			sprintf(string,"%s","Fast CGI");
			break;
		case WEBSOCKET:
			sprintf(string,"%s","WebSocket");
			break;
		case COAP:
			sprintf(string,"%s","CoAP");
			break;
		default:
			sprintf(string,"%s","Unknown");
			break;
	}
	fprintf(stdout, "\t---CONNECTION: ---\n");
	fprintf(stdout, "\tType: %s\n", string);
}

int connection_response_send(connection_t *connection, coap_pdu_t *response) {
	int http_code;

	switch (connection->type) {
		case FASTCGI:
			switch(response->hdr.code) {
				case COAP_RESPONSE_201:
					http_code = 201;
					break;
				case COAP_RESPONSE_202:
					http_code = 204;
					break;
				case COAP_RESPONSE_203:
					http_code = 304;
					break;
				case COAP_RESPONSE_204:
					http_code = 204;
					break;
				case COAP_RESPONSE_205:
					http_code = 200;
					break;
				case COAP_RESPONSE_400:
					http_code = 400;
					break;
				case COAP_RESPONSE_401:
					http_code = 400;
					break;
				case COAP_RESPONSE_402:
					http_code = 400;
					break;
				case COAP_RESPONSE_403:
					http_code = 403;
					break;
				case COAP_RESPONSE_404:
					http_code = 404;
					break;
				case COAP_RESPONSE_405:
					http_code = 405;
					break;
				case COAP_RESPONSE_413:
					http_code = 413;
					break;
				case COAP_RESPONSE_415:
					http_code = 415;
					break;
				case COAP_RESPONSE_500:
					http_code = 500;
					break;
				case COAP_RESPONSE_501:
					http_code = 501;
					break;
				case COAP_RESPONSE_502:
					http_code = 502;
					break;
				case COAP_RESPONSE_503:
					http_code = 503;
					break;
				case COAP_RESPONSE_504:
					http_code = 504;
					break;
				case COAP_RESPONSE_505:
					http_code = 400;
					break;
				default:
					http_code = 400;
					break;
			}

			FCGX_FPrintF(FASTCGI_UNIX_REQUEST(connection)->out,"Status: %d\r\n"
			        "Content-type: text/html\r\n\r\n"
			        "%s\r\n",http_code,response->payload);
			FCGX_Finish_r(FASTCGI_UNIX_REQUEST(connection));
			return 1;
		case WEBSOCKET:
			return websocket_send(WEBSOCKET_SOCKET(connection),response->payload);
		case COAP:
			return 1;
		default:
			coap_debug_error("Bad connection provided. Connection deleted.\n");
			return 0;
	}
	return 0;
}
