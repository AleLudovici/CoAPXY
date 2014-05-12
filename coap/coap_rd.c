#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coap_rd.h"

void coap_rd_entry_node_delete(struct coap_node_t *node)
{
	coap_rd_entry_t *entry;

	entry = (coap_rd_entry_t *)node->data;

	coap_rd_entry_delete(entry);
}

int coap_rd_entry_create(char *name, coap_rd_entry_t **entry)
{
	if (((*entry) = (coap_rd_entry_t *) malloc (sizeof(coap_rd_entry_t))) == NULL) {
		return 0;
	}

	if (((*entry)->name = (char *) malloc (strlen(name))) == NULL) {
		free(*entry);
		return 0;
	}

	sprintf((*entry)->name, "%s", name);

	if (!coap_list_create(coap_resource_node_delete, NULL, &(*entry)->observers)) {
		free((*entry)->name);
		free(*entry);
		return 0;
	}

	(*entry)->lt = COAP_RD_DEFAULT_LIFETIME;

	return 1;
}

void coap_rd_entry_delete(coap_rd_entry_t *entry)
{
	free(entry->name);
	coap_list_delete(entry->observers);
	free(entry);
}

int coap_rd_create(coap_rd_t **rd)
{
	if ((*rd = (coap_rd_t *) malloc (sizeof(coap_rd_t))) == NULL) {
		return 0;
	}
	if (!coap_list_create(coap_rd_entry_node_delete, NULL, &(*rd)->entries)) {
		free(*rd);
		return 0;
	}
	return 1;
}

void coap_rd_delete(coap_rd_t *rd)
{
	coap_list_delete(rd->entries);
	free(rd);
}

int coap_rd_entry_add(coap_rd_t *rd, coap_rd_entry_t *entry)
{
	return coap_list_node_insert(rd->entries, entry->name, strlen(entry->name), entry);
}

void coap_rd_print(coap_rd_t *rd)
{
	coap_list_index_t *li_rd, *li_res;
	coap_rd_entry_t *entry;
	coap_resource_t *res;

	for (li_rd = coap_list_first(rd->entries);li_rd; li_rd = coap_list_next(li_rd)) {
		coap_list_this(li_rd, NULL,(void **) &entry);

		fprintf(stderr,"ENTRY: %s\n", entry->name);
		fprintf(stderr,"RESOURCES: \n");
		for (li_res = coap_list_first(entry->observers);li_res; li_res = coap_list_next(li_res)) {
			coap_list_this(li_res, NULL,(void **) &res);

			fprintf(stderr,"\t");
			coap_resource_print(res);

		}
	}
}
