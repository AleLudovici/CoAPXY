#ifndef RESOURCE_H_
#define RESOURCE_H_

#include "../coap/coap_resource.h"
#include "../coap/coap_client.h"
#include "../coap/coap_list.h"

typedef struct {
	char *path;
	coap_resource_t *resource;
	coap_list_t *connections;
	coap_client_t *client;
} resource_t;

int resource_create(char *name, coap_resource_t *resource, resource_t **res);
void resource_delete(resource_t *resource);
void resource_node_delete(struct coap_node_t *node);

#endif /* RESOURCE_H_ */
