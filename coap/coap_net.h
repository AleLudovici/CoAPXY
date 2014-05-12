#ifndef _COAP_NET_H_
#define _COAP_NET_H_

#include <netinet/in.h>
#include "coap_pdu.h"

#define NET_BUF_SIZE 400

int coap_net_address_create(struct sockaddr_in6 *addr, char *hostname, int port);
int coap_net_udp_create();
int coap_net_udp_bind(int port);
int coap_net_tcp_connect(char *hostname,int port);
int coap_net_tcp_bind(int port);

#endif
