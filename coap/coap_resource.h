#ifndef COAP_RESORCE_H_
#define COAP_RESORCE_H_

#include "coap_list.h"
#include "coap_pdu.h"

struct coap_resource_t {
	char *path; /* The URI accessible from the Web browser. */
	char *name; /* Descriptive name for the resource. */
	char *rt; /* Resource Identifier (from CoRE Link format). */
	char *ifd; /* Interface descriptor (from CoRE Link format). */
	int sz; /* Maximum size estimate (from CoRE Link format). */
	struct sockaddr_in6 server_addr; /* Server address. */
	unsigned int is_observe; /* Indicates if the resource is accessed using a subscription */
	coap_list_t *observers; /* This list is initialized if is_observe=1 */
	void (*handler)(coap_pdu_t *);
};

typedef struct coap_resource_t coap_resource_t;

/**
 * Adds the given subscription object to the observer list.
 * @param resource The resource created.
 * @param uri The CoAP URI path to be copied.
 * @param name The descriptive name for the resource.
 * @param is_observe If is accessed via subscription (1) or not (0).
 * @return 1 if the resource is created and 0 if not.
*/
int coap_resource_create(char *path, char *name, char *rt, char *ifd, int sz,
		int is_observe, void (*handler)(coap_pdu_t *),
		coap_resource_t **resource);

int coap_resource_add_handler(coap_resource_t *resource,
		void (*handler)(coap_pdu_t *));
/**
 * Deletes a resources from memory freeing all its elements.
 * @param resource The resource to be deleted.
 * @return 1 if the resource can be deleted and 0 if not.
 */
int coap_resource_delete(coap_resource_t *resource);

/**
 * Prints resource contents using the standard output.
 * @param resource The resource to print.
 */
void coap_resource_print(coap_resource_t *resource);


/**
 * Gets the resources representation in CoRE Link format.
 * @param resources The resources list.
 * @param data String with the CoRE Link format representation.
 * @param len CoRE Link format string representation length.
 * @return 1 if can be converted and 0 if not.
 */
int coap_resource_link_format(coap_resource_t *resource, char *data, int rd, int *len);

int coap_resource_link_format_all(coap_list_t *resources, char *data, int *len);

/**
 * Updates the given resource list with the data string passed with the CoRE
 * Link Format.
 * @param data String with resources in CoRE link format.
 * @param len String's length.
 * @return 1 if the list was correctly updated and 0 if not.
 */
int coap_resource_update(coap_list_t *resources, char *data, int len);

void coap_resource_node_delete(coap_node_t *node);

#endif /* COAP_RESORCE_H_ */
