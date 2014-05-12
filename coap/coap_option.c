#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "coap_option.h"

int coap_option_create(char code, void *value, int len, coap_option_t **opt) {

    if ((*opt = (coap_option_t *)malloc(sizeof(coap_option_t))) == NULL) {
        return 0;
    }
    memset(*opt,0,sizeof(coap_option_t));

    (*opt)->code = code;
    memcpy((*opt)->data,value,len);
    (*opt)->len = len;

    return 1;
}

int coap_option_delete(coap_option_t *opt) {
    free(opt);
    return 1;
}
