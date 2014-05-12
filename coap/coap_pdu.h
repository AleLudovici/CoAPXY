#ifndef _COAP_PDU_H_
#define _COAP_PDU_H_

#include <netinet/in.h>

#include "coap_general.h"
#include "coap_option.h"
#include "coap_list.h"


/* CoAP message types */
enum {
	COAP_MESSAGE_CON = 0, /* confirmable message (requires ACK/RST) */
	COAP_MESSAGE_NON = 1, /* non-confirmable message (one-shot message) */
	COAP_MESSAGE_ACK = 2, /* used to acknowledge confirmable messages */
	COAP_MESSAGE_RST = 3 /* indicates error in received messages */
};
/* CoAP pdu methods */

enum {
	COAP_REQUEST_GET = 1,
	COAP_REQUEST_POST = 2,
	COAP_REQUEST_PUT = 3,
	COAP_REQUEST_DELETE = 4
};

/* CoAP result codes (HTTP-Code / 100 * 40 + HTTP-Code % 100) */

enum {
	COAP_RESPONSE_201 = 65, /* Created */
	COAP_RESPONSE_202 = 66, /* Deleted */
	COAP_RESPONSE_203 = 67, /* Valid */
	COAP_RESPONSE_204 = 68, /* Changed */
	COAP_RESPONSE_205 = 69, /* Content */
	COAP_RESPONSE_400 = 128, /* Bad Request */
	COAP_RESPONSE_401 = 129, /* Unauthorized */
	COAP_RESPONSE_402 = 130, /* Bad Option */
	COAP_RESPONSE_403 = 131, /* Forbidden */
	COAP_RESPONSE_404 = 132, /* Not Found */
	COAP_RESPONSE_405 = 133, /* Method Not Allowed */
	COAP_RESPONSE_413 = 141, /* Request Entity Too Large */
	COAP_RESPONSE_415 = 143, /* Unsupported Media Type */
	COAP_RESPONSE_500 = 160, /* Internal Server Error */
	COAP_RESPONSE_501 = 161, /* Not Implemented */
	COAP_RESPONSE_502 = 162, /* Bad Gateway */
	COAP_RESPONSE_503 = 163, /* Service Unavailable */
	COAP_RESPONSE_504 = 164, /* Gateway Timeout */
	COAP_RESPONSE_505 = 165  /* Proxying Not Supported */
};

typedef struct {
    char version; /* Protocol version */   
    unsigned char type;	/* Message type */
    unsigned char code;	/* Pdu method (value 1--10) or response code (value 40-255) */
    unsigned char optcnt; /* Number of option */
    unsigned short id; /* Transaction id */
} coap_hdr_t;

/** PDU definition **/

typedef struct {
    coap_hdr_t hdr; /* Header */
    coap_list_t *opt_list; /* Option list */
    char payload[COAP_MAX_PAYLOAD_SIZE];
    unsigned int payload_len;
    struct sockaddr_in6 addr; /* address. */
    time_t timestamp; /* Timestamp used to check the MAXAGE option */
} coap_pdu_t;

/* function definitions */

int coap_pdu_create(coap_pdu_t **pdu);
int coap_pdu_delete(coap_pdu_t *pdu);

void coap_pdu_node_delete(struct coap_node_t *node);

int coap_pdu_clean(coap_pdu_t *pdu);
int coap_pdu_copy(coap_pdu_t *old, coap_pdu_t *new);

int coap_pdu_option_insert(coap_pdu_t *pdu, char code, char *value, unsigned int len);
int coap_pdu_option_delete(coap_pdu_t *pdu);
int coap_pdu_option_get(coap_pdu_t *pdu, char code, coap_option_t **opt);
int coap_pdu_option_unset(coap_pdu_t *pdu, char code);

void coap_pdu_clean_common_options(coap_pdu_t *pdu);

int coap_pdu_packet_write(coap_pdu_t *pdu, char *buffer,  unsigned int *packet_len);
int coap_pdu_packet_read(char *buffer, unsigned int packet_len, coap_pdu_t *pdu);

void coap_pdu_print(coap_pdu_t *pdu);


int coap_pdu_parse_uri(char *coap_uri, struct sockaddr_in6 *addr,
	char **uri_path, char **uri_query);

int coap_pdu_parse_query(char *query, char *variable, int len, char **value);

int coap_pdu_request(coap_pdu_t *pdu, char *path, char *method, int observe);
int coap_pdu_ack(coap_pdu_t *pdu, char code, unsigned int message_id, void *token, int token_len);

#endif
