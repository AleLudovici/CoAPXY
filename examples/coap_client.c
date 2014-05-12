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
#include <sys/time.h>

#include "../coap/coap_net.h"
#include "../coap/coap_debug.h"
#include "../coap/coap_pdu.h"
#include "../coap/coap_client.h"
#include "../coap/coap_resource.h"

int finish = 0;
int s;
int n = 1; //Number of requests sent. Default 1.
int c = 1; //Concurrency level. Default 1.
char filename[100] = "results.txt";
int c_client = 0;

FILE *file;

struct sockaddr_in6 server_addr;
coap_pdu_t *request;

coap_client_t *client;

typedef struct {
	pthread_t id;
	coap_client_t *client;
	struct timeval tv;
} thread_client_t;

thread_client_t *clients;

void response_received(coap_pdu_t *response, void *data)
{
	coap_pdu_print(response);
}


static void usage( const char *program, const char *version) {
  const char *p;

  p = strrchr( program, '/' );
  if ( p )
    program = ++p;

  fprintf( stdout, "%s v%s -- CoAP client\n"
	   "(c) 2011 Pol Moreno <pol.moreno@entel.upc.edu>\n\n"
	   "usage: %s [-n requests] [-f results] URI\n\n",
	   program, version, program);
}

int main(int argc, char *argv[]) {
    int opt;
    char *uri_path, *uri_query;
    fd_set lset, lsetcopy;
    coap_client_t *client;
    int max, rc;
    int i;

    if (argc < 2) {
    	usage(argv[0],"1.0");
    	return 1;
    }
    while ((opt = getopt(argc, argv, "s:n:f:c:")) != -1) {
    	switch (opt) {
    		case 's':
    			s = atoi(optarg);
    			break;
    		case 'n':
    			n = atoi(optarg);
    			break;
    		case 'f':
    			sprintf(filename,"%s",optarg);
    			break;
    		case 'c':
    			c = atoi(optarg);
    			break;
        	default:
        		usage( argv[0], "1.0" );
        		exit(1);
        }
    }
   	if (optind >= argc) {
   		usage(argv[0],"1.0");
   	}


   	coap_pdu_create(&request);

    request->hdr.code = COAP_REQUEST_GET;


    coap_pdu_option_insert(request, COAP_OPTION_PROXY_URI, "coap://[::1]:8005/test", strlen("coap://[::1]:8005/test"));

   	if (!coap_pdu_parse_uri(argv[optind], &server_addr, &uri_path, &uri_query)) {
   		fprintf(stderr,"Wrong URI format provided.\n");
   	}

    request->addr = server_addr;

    coap_pdu_option_insert(request, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));
    coap_pdu_option_insert(request, COAP_OPTION_URI_QUERY, uri_query, strlen(uri_query));

    /*sprintf(tmp, "%d", COAP_MEDIATYPE_LINK_FORMAT);
    coap_pdu_option_insert(request, COAP_OPTION_CONTENT_TYPE, tmp, strlen(tmp));
    //coap_pdu_option_insert(request, COAP_OPTION_URI_QUERY, "n=pol2", strlen("n=pol2"));
    sprintf(request->payload,"%s", "</fd>;rt=Press");
    request->payload_len = strlen(request->payload);*/
   	if (!coap_client_create(NULL, NULL, response_received, NULL, &client)) {
    	coap_debug_error("Error creating the client.\n");
    	return 0;
    }

   	coap_client_send(client, request, 0);
    max = client->context->socket;
    FD_ZERO(&lset);
    FD_SET(client->context->socket, &lset);


    lsetcopy = lset;
    while (!finish) {
        lset = lsetcopy;
        do {
            rc = select(max + 1, &lset, NULL, NULL, NULL);
        } while(rc < 0 && errno == EINTR);

        if(rc == 0) {
            continue;
        }
        if (FD_ISSET(client->context->socket,&lset)) {
        	coap_client_read(client);
        	coap_client_dispatch(client);
        } else {
        	coap_client_clean_pending(client);
        }
        finish = 1;
    }
    return 0;
}
