#ifndef _COAP_LIST_H_
#define _COAP_LIST_H_

struct coap_node_t {
    struct coap_node_t *next;
    char *key;
    void *data;
};

struct coap_list_index_t {
    struct coap_list_t *list;
    struct coap_node_t *this, *next;
};

struct coap_list_t {
    struct coap_node_t *first;
    struct coap_list_index_t  iterator;  /* For apr_hash_first(NULL, ...) */
    void (*delete_func)(struct coap_node_t *);
    int (*order_func)(void *, void *);
};


typedef struct coap_list_t coap_list_t;
typedef struct coap_node_t coap_node_t;
typedef struct coap_list_index_t coap_list_index_t;

/* function definitions */
int coap_list_create(void (*delete_func)(struct coap_node_t *),
		int (*order_func)(void *, void *), coap_list_t **list);
int coap_list_node_insert(coap_list_t *list, void *key, int len, void *data);


int coap_list_node_get(coap_list_t *list, void *key, int len, void **data);
int coap_list_node_create(void *key, int len, void *data, coap_node_t **node);
int coap_list_node_delete(coap_list_t *list, void *key, int len);
int coap_list_clean(coap_list_t *list);
int coap_list_delete(coap_list_t *list);

coap_list_index_t *coap_list_first(coap_list_t *list);
coap_list_index_t *coap_list_next(coap_list_index_t *li);
void coap_list_this(coap_list_index_t *li, const void **key,void **data);


#endif
