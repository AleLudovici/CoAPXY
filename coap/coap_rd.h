#ifndef COAP_RD_H_
#define COAP_RD_H_

#include "coap_resource.h"
#include "coap_list.h"

#define COAP_RD_DEFAULT_LIFETIME 86400
#define COAP_RD_BASE_DIR "rd"

typedef struct {
	char *name;
	int id;
	int lt;
	char *rt;
	coap_resource_t *coap_res;
	coap_list_t *observers;
} coap_rd_entry_t;

typedef struct {
	coap_list_t *entries;
} coap_rd_t;

void coap_rd_node_delete(struct coap_node_t *node);

int coap_rd_entry_create(char *name, coap_rd_entry_t **entry);

void coap_rd_entry_delete(coap_rd_entry_t *entry);

int coap_rd_create(coap_rd_t **rd);

void coap_rd_delete(coap_rd_t *rd);

int coap_rd_entry_add(coap_rd_t *rd, coap_rd_entry_t *entry);

void coap_rd_print(coap_rd_t *rd);

void coap_rd_get(coap_resource_t *res, coap_rd_t *rd);


#endif /* COAP_RD_H_ */
