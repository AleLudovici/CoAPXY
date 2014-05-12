#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "../coap/coap_pdu.h"
#include "../coap/coap_general.h"
#include "../coap/coap_context.h"

int sock;
coap_pdu_t *request;

struct sockaddr_in6 server_addr;
coap_context_t *client;

int n = 1; //Number of requests sent. Default 1.
int c = 1; //Concurrency level. Default 1.
char filename[100] = "results.txt";

FILE *file;

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

static int parse_cmd_uri(struct sockaddr_in6 *server_addr, coap_pdu_t **pdu, char *uri)
{
	char *p,*port_tmp, *host, *path, *query_string;
	int len = strlen(uri);
	int port;

	/* URI must start with coap:// */
	if (strncmp(uri, "coap://", 7) != 0) {
		return 0;
	}
	p = &uri[7];
	/* Next is the IPv6 address */
	if (*p == '[') {
		host = &uri[8];
		while (p - uri < len && *p != ']')
			p++;
	}
	if (*p == ']') {
		*p = '\0';
	} else {
		return 0;
	}

	/* Look for port */
	if (p - uri + 2 < len && *(++p) == ':') {
		port_tmp = ++p;
		while (p - uri < len && isdigit(*p) && *p != '/')
			p++;
		if (*p == '/') {
			*p = '\0';
		}
		port = atoi(port_tmp);
		*p = '/';
	} else {
		port = COAP_DEFAULT_PORT;
	}
	/* Look for URI */
	if (p - uri + 1 < len && *p == '/') {
		*p = '\0';
		path = ++p;
		while (p - uri < len && *p != '?')
			p++;
	}
	/* Look for QUERY STRING */
	if (p - uri + 1 < len && *p == '?') {
		*p = '\0';
		query_string = ++p;
		while (p - uri < len)
			p++;
	}
	fprintf(stdout, "HOST: %s, PORT: %d, PATH: %s, QUERY: %s\n", host, port, path, query_string);
	if (!coap_net_address_create(server_addr, host, port)) 
		return 0;
	
	/* Create CoAP PDU */
	if (!coap_pdu_create(pdu)) 
		return 0;
	
	if (!coap_pdu_request(*pdu, path, "GET", 0)) 
		return 0;
	
	(*pdu)->addr = *server_addr;

	coap_pdu_print(*pdu);
	return 1;
}

static void *send_and_receive(void *a)
{
	char buffer[COAP_MAX_PDU_SIZE];
	int len;
	coap_pdu_t *response;
	int j;

	struct timeval tv;
	double ini_time, end_time;

	static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

	if (!coap_pdu_create(&response)) {
		fprintf(stderr,"Cannot create the response.\n");
		exit(1);
	}

	for (j = 0; j < n; j++) {
		timerclear(&tv);
		gettimeofday(&tv, 0);
		ini_time = tv.tv_sec+(tv.tv_usec/1000000.0);
		coap_client_send(client, request);

		timerclear(&tv);
		gettimeofday(&tv, 0);
		end_time = tv.tv_sec+(tv.tv_usec/1000000.0);

		coap_client_read(client);
		coap_client_dispatch(client);

		pthread_mutex_lock(&file_mutex);
		fprintf(file,"%lf \t %d \t %d \t %lf\n", end_time - ini_time, len, response->payload_len, response->payload_len / (end_time - ini_time)/1000);
		pthread_mutex_unlock(&file_mutex);
	}
	return(0);
}

static void handler(coap_pdu_t *response, void *data)
{
	coap_pdu_print(response);
}

int main (int argc, char *argv[])
{
	int opt;
	int i;
	pthread_t *id;


	if (argc < 2) {
		usage(argv[0],"1.0");
		return 1;
	}
	while ((opt = getopt(argc, argv, "n:f:c:")) != -1) {
		switch (opt) {
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
	if (!parse_cmd_uri(&server_addr, &request, argv[optind])) {
		fprintf(stderr,"Wrong URI format provided.\n");
	}


	coap_client_create(NULL, NULL,handler, NULL, &client);


	if ((id = (pthread_t *) malloc (sizeof(pthread_t)*c)) == NULL) {
		fprintf(stderr,"Unable to create Pthread list.\n");
		return 0;
	}

	if ((file = fopen(filename, "w+")) < 0) {
		fprintf(stderr,"Unable to open the results file.\n");
		return 0;
	}
	fprintf(file,"Latency \t Bytes received \t Payload bytes \t Throughtput\n");


	for (i = 0; i < c; i++) {
		pthread_create(&id[i], NULL, send_and_receive, (void*)i);
	}

	for(i = 0; i < c; i++) {
		pthread_join(id[i], NULL);
	}

	fclose(file);
	return 0;
}
