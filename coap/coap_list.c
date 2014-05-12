/* MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

#include "coap_list.h"
#include "coap_md5.h"
#include "coap_option.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int coap_list_create(void (*delete_func)(struct coap_node_t *),
		int (*order_func)(void *, void *), coap_list_t **list) {
	if((*list = (coap_list_t *) malloc (sizeof(coap_list_t))) == NULL) {
        return 0;
    }
    memset(*list,0,sizeof(coap_list_t));
    (*list)->delete_func = delete_func;
    (*list)->order_func = order_func;
    return 1;
}

int coap_list_node_insert(coap_list_t *list, void *key, int len, void *data) {
    coap_node_t *node,*q,*p;    

    if(!coap_list_node_create(key, len, data, &node)) {
        return 0;
    }

    /* set queue head if empty */
    if (!list->first) {
        list->first = node;
        return 1;
    }

    q = list->first;
    if (list->order_func && list->order_func( node->data, q->data ) < 0 ) {
    	node->next = q;
        list->first = node;
        return 1;
    }

    /* insert at the end */
    do {
        p = q;
        q = q->next;
    } while (q && list->order_func && list->order_func( node->data, q->data ) >= 0);

    /* insert new item */
    node->next = q;
    p->next = node;

    return 1;
}



int coap_list_node_get(coap_list_t *list, void *key, int len, void **data) {
    coap_node_t *p;
    char *search_key, *printer;
    int i;

    if (!list || !list->first) {
        return 0;    
    }

    search_key = key;

     for (p = list->first; p != NULL; p = p->next) {

    	 if (strncmp(p->key, search_key, len) == 0) {

    		 if (!p->data) {
					return 0;
				}
				if (data != NULL){
					*data = p->data;
					return 1;
				}
    	 }
     }
     return 0;
}

int coap_list_node_create(void *key, int len,void *data, coap_node_t **node) {

    if ((*node = (coap_node_t *)malloc(sizeof(coap_node_t))) == NULL) {
        return 0;
    }
    (*node)->next = NULL;
    (*node)->data = data;
    (*node)->key = (char *)malloc(sizeof(char)*(strlen(key)+1));

	strcpy((*node)->key,key);

	return 1;
}

int coap_list_node_delete(coap_list_t *list, void *key, int len) {
    coap_node_t *p,*q;
    char *search_key;

    if (!list || !list->first) {
        return 0;
    }

	search_key=key;

    q = list->first;
    if (strncmp(q->key,search_key,len) == 0) {
        p = list->first;
        list->first = list->first->next;
        if (list->delete_func)
            list->delete_func(p);
        free(p->key);
        free(p);
        return 1;
    }

    for (p = q->next; p != NULL; p = p->next) {
        if (strncmp(p->key,search_key,len) == 0) {
            q->next = p->next;
            if (list->delete_func)
                list->delete_func(p);
            free(p->key);
            free(p);
            return 1;
        }
        q = p;
    }

    return 0;
}


int coap_list_clean(coap_list_t *list) {
	coap_node_t *p,*q;
    for (p = list->first; p != NULL;) {

        q = p;
        p = p->next;
        if (list->delete_func)
        	list->delete_func(q->data);
        free(q->key);
        free(q);
    }
    list->first = NULL;
    return 1;
}

int coap_list_delete(coap_list_t *list) {
	coap_node_t *p,*q;
    for (p = list->first; p != NULL;) {

        q = p;
        p = p->next;
        if (list->delete_func)
        	list->delete_func(q->data);
        free(q->key);
        free(q);
    }
    free(list);
    return 1;
}

coap_list_index_t *coap_list_next(coap_list_index_t *li)
{
    li->this = li->next;
    if (!li->this) {
        return NULL;
    }
    li->next = li->this->next;
    return li;
}

coap_list_index_t *coap_list_first(coap_list_t *list)
{
    coap_list_index_t *li;

    if (list->first != NULL) {    
        li = &list->iterator;
        li->list = list;
        li->this = list->first;
        li->next = list->first->next;
        return li;
    }
    return NULL;
}

void coap_list_this(coap_list_index_t *li,
                                const void **key,
                                void **data)
{
	if (key)  *key  = li->this->key;
    if (data)  *data  = (void *)li->this->data;
}

