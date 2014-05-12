#ifndef _coap_SOCKSET_H
#define _coap_SOCKSET_H
#include <sys/types.h>

typedef struct {
    fd_set rdfs;
    struct timeval tv;
    int max;
} sockset_t;

void sockset_init(sockset_t *ss);
void sockset_add(sockset_t *ss,int sd);
void sockset_remove(sockset_t *ss,int sd);

#endif
