#ifndef _COAP_CACHE_H_
#define _COAP_CACHE_H_

#include <db.h>
#include "coap_pdu.h"
#include <netinet/in.h>

/* Data to store into the database */
typedef struct {
    time_t timestamp;
    char packet[COAP_MAX_PDU_SIZE];
    int packet_len;
    struct sockaddr_in6 addr;
} coap_cache_t;

/**
 * Creates a special MD5 checksum given a CoAP PDU. It removes ETAG, MAXAGE and TOKEN
 * options and the message ID. This way, two different CoAP PDUs can be compared
 * using the returned digest value. Two PDUs will be considered equal from the cache point
 * of view if all PDU contains the same fields except the ones mentioned above. Comparing
 * this checksum is a way for determining if two PDUs are equal
 * @param pdu CoAP pdu.
 * @param digest The MD5 sum of the CoAP PDU.
 */
void coap_hash(coap_pdu_t *pdu, char *digest);
/**
 * Creates a new Oracle Berkeley DB object.
 * @param database_path System path where to store the cache database.
 * @param database The resulting Oracle Berkeley DB database object.
 */
int coap_cache_create(char *database_path, DB **database);
/**
 * Stores a new CoAP PDU in the cache database.
 * @param database The Oracle Berkeley DB object representing the cache database.
 * @param request The CoAP PDU to use as cache record index.
 * @param response The CoAP PDU response which is going to be cached.
 * @return 1 if a cache record was successfully stored and 0 otherwise.
 */
int coap_cache_add(DB *database, coap_pdu_t *request, coap_pdu_t *response);
/**
 * Gets an existing CoAP cache record from the cache database.
 * @param database The Oracle Berkeley DB database object representing the cache database.
 * @param request The CoAP PDU request from which the cache was created.
 * @param pdu The CoAP PDU response stored in the cache in case some response was found.
 * @return 1 if a cache record was found and 0 otherwise.
 */
int coap_cache_get(DB *database, coap_pdu_t *request, coap_pdu_t **pdu);

/**
 * Revalidates a stored CoAP cache record updating its freshness. This
 * function will be called when the server receives a 2.03 Valid response code.
 * @param database The Oracle Berkeley DB database object to search on.
 * @param request The request CoAP PDU that sent the validation request to the server.
 * @param pdu The CoAP PDU containing the E-tag sent from the server.
 * @return 1 if a cache record was found and 0 otherwise.
 */
int coap_cache_revalidate(DB *database, coap_pdu_t *request, coap_pdu_t **response);

/**
 * Removes an existing CoAP cache record from the cache database.
 * @param database The Oracle Berkeley DB database object.
 * @param pdu The CoAP PDU which is the cache record index.
 * @return 1 if the cache record was successfully removed and 0 otherwise.
 */
int coap_cache_delete(DB *database, coap_pdu_t *pdu);

#endif
