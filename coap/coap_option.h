#ifndef _COAP_OPTION_H_
#define _COAP_OPTION_H_

#include <netinet/in.h>

#include "coap_general.h"

#define MAX_OPT_DATA 270
typedef struct {
    char code;
    int len;
    char data[MAX_OPT_DATA];
} coap_option_t;

enum {
  COAP_OPTION_CONTENT_TYPE = 1, /* C, uint, 0-2 B */
  COAP_OPTION_MAXAGE = 2, /* E, uint, 0-4 B, 60 Seconds */
  COAP_OPTION_PROXY_URI = 3, /* C, string, 1-270 B */
  COAP_OPTION_ETAG = 4, /* E, opaque, 1-8 B, - */
  COAP_OPTION_URI_HOST = 5, /* C, string, 1-270 B, "" */
  COAP_OPTION_LOCATION_PATH = 6, /* E, String, 1-270 B, - */
  COAP_OPTION_URI_PORT = 7, /* C, uint, 0-2 B, - */
  COAP_OPTION_LOCATION_QUERY = 8, /* E, string, 1-270 B, - */
  COAP_OPTION_URI_PATH = 9, /* C, string, 1-270 B, "" */
  COAP_OPTION_OBSERVE = 10,
  COAP_OPTION_TOKEN = 11, /* C, opaque, 1-8 B, - */
  COAP_OPTION_ACCEPT = 12, /* E, uint 0-2 B */
  COAP_OPTION_IF_MATCH = 13, /* C, opaque 0-8 B*/
  COAP_OPTION_URI_QUERY = 15, /* C, string, 1-270 B*/
  COAP_OPTION_IF_NONE_MATCH = 21, /* C, none 0 B*/
};

/* function definitions */

int coap_option_create(char code, void *value, int len, coap_option_t **opt);
int coap_option_delete(coap_option_t *opt);

#endif
