#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>

#include "coap_net.h"
#include "coap_pdu.h"



int coap_net_address_create(struct sockaddr_in6 *addr, char *hostname, int port)
{
	struct hostent *hp;

	hp = gethostbyname2(hostname, AF_INET6);
	if (hp == NULL)
		return 0;
   /* struct addrinfo hints;
    struct addrinfo *res;

    memset ((char *)&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET6;
    
    if(getaddrinfo(hostname, "", &hints, &res) != 0) {
        return 0;
    }*/

    memset(addr, 0, sizeof addr );
 	memcpy((char *)&addr->sin6_addr, hp->h_addr, hp->h_length);
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons( port );

    return 1;
}

int coap_net_udp_bind(int port) {
    int netd;
    struct sockaddr_in6 addr;


    if((netd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;

    if(bind(netd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return -1;
    }

    return netd;
}

int coap_net_udp_create()
{
	int netd;


	    if((netd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
	        return -1;
	    }

	    /*memset(&addr, 0, sizeof(addr));
	    addr.sin6_family = AF_INET6;
	    addr.sin6_port = htons(10000);
	    addr.sin6_addr = in6addr_any;

	    if(bind(netd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
	            return -1;
	        }*/

	    return netd;
}

int coap_net_tcp_bind(int port) {
    int netd;
    int opt = 1;
    struct sockaddr_in addr;

    if((netd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

	setsockopt(netd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl (INADDR_ANY);

    if(bind(netd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return -1;
    }

    listen(netd,10);

    return netd;
}

int coap_net_tcp_connect(char *hostname, int port) {
    int netd;
    struct hostent  *hp;
    struct sockaddr_in addr;

    if((hp = gethostbyname(hostname)) == NULL) {
        return -1;
    }

    if((netd = socket(AF_INET, SOCK_STREAM, 0))<0) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = hp->h_addrtype;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr.s_addr, hp->h_addr, hp->h_length);

    if(connect(netd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return -1;
    }

    return netd;
}
