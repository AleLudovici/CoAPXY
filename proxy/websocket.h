#ifndef _coap_WEBSOCKET_H_
#define _coap_WEBSOCKET_H_

#define WEBSOCKET_BUF_SIZE 400

int websocket_do_handshake(int sock, char *uri, char *method);
int websocket_send(int sock, char *message);

#endif
