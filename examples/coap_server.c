#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "../coap/coap_net.h"
#include "../coap/coap_debug.h"
#include "../coap/coap_pdu.h"
#include "../coap/coap_server.h"
#include "../coap/coap_resource.h"


void handler_resource(coap_pdu_t *request)
{
	request->hdr.type = COAP_MESSAGE_ACK;
	request->hdr.code = COAP_RESPONSE_205;
	sprintf(request->payload, "%s", "HOLA");
	request->payload_len = 4;
	coap_pdu_option_unset(request, COAP_OPTION_URI_PATH);
	coap_pdu_print(request);


}

int main() {
	coap_server_t *server;
    fd_set lset, lsetcopy;
    int rc, max;
    coap_resource_t *res;
    struct timeval timeout;



    /* Create CoAP server */
    if (!coap_resource_create("test",  "Test", "", "", 0, 0, handler_resource, &res))
    	return 1;
    if (!coap_server_create(8005,0, &server))
    	return 1;

    coap_server_resource_add(server, res);

    max = server->context->socket;
    FD_ZERO(&lset);
    FD_SET(server->context->socket, &lset);

    lsetcopy = lset;

    while (1) {
        lset = lsetcopy;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        do {
            rc = select(max + 1, &lset, NULL, NULL, &timeout);
        } while(rc < 0 && errno == EINTR);

        if(rc == 0) {
//        	coap_pdu_t *pdu;
//
//        	coap_pdu_create(&pdu);
//
//        	pdu->hdr.type = COAP_MESSAGE_CON;
//        	pdu->hdr.code = COAP_RESPONSE_205;
//        	sprintf(pdu->payload, "%s", "HOLA MUNDO");
//        	pdu->payload_len = strlen("HOLA MUNDO");
//
//        	coap_server_notify_observers(server, res, pdu);
//
//        	coap_pdu_delete(pdu);

            continue;
        }
        if (FD_ISSET(server->context->socket,&lset)) {
        	coap_server_read(server);
        	coap_server_dispatch(server);
        } else {
        	coap_server_clean_pending(server);
        }
    }
    return 0;
}
