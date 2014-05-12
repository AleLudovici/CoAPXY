#ifndef _COAP_GENERAL_H_
#define _COAP_GENERAL_H_

#define RESPONSE_TIMEOUT 2
#define MAX_RETRANSMIT 4


#define COAP_DEFAULT_RESPONSE_TIMEOUT  5 /* response timeout in seconds */
#define COAP_DEFAULT_MAX_RETRANSMIT    5 /* max number of retransmissions */
#define COAP_DEFAULT_PORT          5683 /* CoAP default UDP port */
#define COAP_DEFAULT_MAX_AGE          120 /* default maximum object lifetime in seconds */
#define COAP_MAX_PDU_SIZE           1400
#define COAP_MAX_PAYLOAD_SIZE           1400
#define COAP_DEFAULT_VERSION           1 /* version of CoAP supported */
#define COAP_DEFAULT_SCHEME        "coap" /* the default scheme for CoAP URIs */
#define COAP_DEFAULT_URI_WELLKNOWN ".well-known/core" /* well-known resources URI */


/* CoAP media type encoding */

#define COAP_MEDIATYPE_TEXT_PLAIN                     0 /* text/plain (UTF-8) */
#define COAP_MEDIATYPE_LINK_FORMAT                    40
#define COAP_MEDIATYPE_XML                            41
#define COAP_MEDIATYPE_OCTET_STREAM                   42
#define COAP_MEDIATYPE_EXI                            47
#define COAP_MEDIATYPE_JSON                           50

#endif
