#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <ctype.h>

#include "coap_pdu.h"
#include "coap_list.h"
#include "coap_debug.h"
#include "coap_general.h"

void coap_pdu_node_delete(struct coap_node_t *node)
{
	coap_pdu_t *pdu;

	pdu = (coap_pdu_t *)node->data;
	coap_pdu_delete(pdu);
}

static void delete_option_func(coap_node_t *node) {
    coap_option_t *opt;
    opt = (coap_option_t *)node->data;
    free(opt);
}

static int order_option_func(void *data_new, void *data_old) {
	coap_option_t *opt_new, *opt_old;

	opt_new = (coap_option_t *)data_new;
	opt_old = (coap_option_t *)data_old;

    if (opt_new->code == opt_old->code) {
        return 0;
    } else if (opt_new->code < opt_old->code) {
        return -1;
    }
    return 1;
}

int coap_pdu_create(coap_pdu_t **pdu) {

    if ((*pdu = (coap_pdu_t *)malloc(sizeof(coap_pdu_t))) == NULL) {
    	coap_debug_error("malloc\n");
        return 0;
    }

    memset(*pdu, 0, sizeof(coap_pdu_t));
    
    /* Assign current timestamp */
    (*pdu)->timestamp = time(NULL);
    coap_list_create(delete_option_func, order_option_func, &((*pdu)->opt_list));

    return 1;    
}

int coap_pdu_request(coap_pdu_t *pdu, char *path, char *method, int observe) {
	coap_pdu_clean(pdu);

	/* Fill The Header */
	pdu->hdr.version = COAP_DEFAULT_VERSION;
	pdu->hdr.type = COAP_MESSAGE_CON;

	/* Insert the method code */
	if (strcmp (method, "GET")) {
		pdu->hdr.code = COAP_REQUEST_GET;
	} else if (strcmp (method, "POST")) {
		pdu->hdr.code = COAP_REQUEST_POST;
	} else if (strcmp (method, "PUT")) {
		pdu->hdr.code = COAP_REQUEST_PUT;
	} else if (strcmp (method, "DELETE")) {
		pdu->hdr.code = COAP_REQUEST_DELETE;
	} else {
		/* No Method */
		coap_debug_error("no method\n");
		return 0;
	}

	/* INSERT URI_PATH*/
	coap_pdu_option_insert(pdu, COAP_OPTION_URI_PATH, (void *)path, strlen(path));

	/* Insert the observe option */
	if (observe) {
		coap_pdu_option_insert(pdu, COAP_OPTION_OBSERVE, "0", sizeof(char));
	}

	return 1;
}

int coap_pdu_ack(coap_pdu_t *pdu, char code, unsigned int message_id, void *token, int token_len)
{
	coap_pdu_clean(pdu);

    pdu->hdr.version = COAP_DEFAULT_VERSION;
    pdu->hdr.type = COAP_MESSAGE_ACK;
    pdu->hdr.code = code;
    pdu->hdr.id = message_id;

    coap_pdu_option_insert(pdu,COAP_OPTION_TOKEN,
    	token,token_len);

    return 1;
}

int coap_pdu_packet_write(coap_pdu_t *pdu, char *buffer,  unsigned int *packet_len)
{
    
    char *pointer = buffer;

    unsigned int opt, opt_delta = 0;
    coap_option_t *option;
    coap_list_index_t *li;

    if (!buffer) {
        return 0;
    }

    if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
    *buffer = (((pdu->hdr.version) << 6) | ((pdu->hdr.type) << 4) | (pdu->hdr.optcnt & 0x0F));
    buffer++;


    if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
    *buffer = pdu->hdr.code;
    buffer++;

    if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
    *((unsigned short *)buffer) = (unsigned short)pdu->hdr.id;
    buffer += 2;

    opt = 0;

    for (li = coap_list_first(pdu->opt_list);li;li = coap_list_next(li)) {
    	coap_list_this(li,NULL,(void **)&option);

    	opt_delta = option->code - opt;
        opt = option->code;

        if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
        *buffer = (opt_delta & 0x0F) << 4;

        if (option->len > 15) {
        	if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
            *buffer |= 0x0F;
            buffer++;
            /* extended option */
            if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
            *buffer = option->len - 15;
        } else {
        	if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
            *buffer |= option->len & 0x0F;
        }
        buffer++;

        if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
        memcpy(buffer,option->data,option->len);
        buffer += option->len;
    }
    if (pdu->payload_len > 0) {
        if (buffer - pointer > COAP_MAX_PAYLOAD_SIZE) return 0;
    	memcpy(buffer,pdu->payload,pdu->payload_len);
    }
    *packet_len = buffer - pointer + pdu->payload_len;
    return 1;
}


int coap_pdu_packet_read(char *buffer, unsigned int packet_len, coap_pdu_t *pdu)
{
    int ret;

    char *p, *limit;

    unsigned int cnt;
    unsigned int optcnt;
    
    unsigned int opt_len;
    unsigned int opt_code = 0;

      /* pdu must be initialized */
    if (!pdu || packet_len > COAP_MAX_PDU_SIZE) {
        return 0;
    }

    limit = (char *)((unsigned int)buffer + packet_len);

    /* delete option list */
    coap_pdu_clean(pdu);

    /* Check if the buffer contains the header */
    if (packet_len < sizeof(pdu->hdr)) {

    	return 0;
    }


    /* Parse CoAP header */
    if (p > limit) return 0;
    p = buffer;
    pdu->hdr.version = (*((char *)p) & 0xC0 ) >> 6;
    pdu->hdr.type = (*((char *)p) & 0x30 ) >> 4;
    optcnt = *((char *)p) & 0x0F;
    p++;
    if (p > limit) return 0;
    pdu->hdr.code = *((char *)p);
    p++;
    if (p > limit) return 0;
    pdu->hdr.id = *((unsigned short *)p);
    p += 2;
    pdu->hdr.optcnt = 0;

    /* Parse CoAP options */
    for (cnt = 0; cnt < optcnt; cnt++) {
        /* add delta */
    	if (p > limit) return 0;
        opt_code += (*((char *)p) & 0xF0) >> 4;
        /* calculate option length */
        if (buffer > limit) return 0;
        opt_len = *((char *)p) & 0x0F;

        if (opt_len == 15) {
            p++;
            if (p > limit) return 0;
            /* extended length */
            opt_len = 15 + *((char *)p);
        }
        p++;
        /* get option */
        if (p > limit) return 0;
        ret = coap_pdu_option_insert(pdu,opt_code,(char *)p,opt_len);
        if (ret != 1) {
            return 0;
        }
        p += opt_len;
    }

    if (p > limit) return 0;
    pdu->payload_len = packet_len - ((char *)p - buffer);
    if (pdu->payload_len > packet_len) {
    	return 0;
    }
    if (p > limit) return 0;
    memcpy(pdu->payload, p, pdu->payload_len);

    /* Assign current time stamp */
    pdu->timestamp = time(NULL);

    return 1;
}


int coap_pdu_option_insert(coap_pdu_t *pdu, char code, char *value, unsigned int len) {
    int ret;
    coap_option_t *opt;

    ret = coap_option_create(code, value, len, &opt);

    if (ret != 1) {
        return 0;
    }
    coap_pdu_option_unset(pdu, code);

    if (!coap_list_node_insert(pdu->opt_list,(void *)&code,sizeof(char),(void *)opt))
    	return 0;

    pdu->hdr.optcnt++;
    return 1;
}

int coap_pdu_delete(coap_pdu_t *pdu) {
    coap_pdu_clean(pdu);
    free(pdu);
    return 1;
}

int coap_pdu_clean(coap_pdu_t *pdu) {
    coap_option_t *opt;
    coap_list_index_t *li;
    if (pdu->opt_list != NULL) {
		for (li = coap_list_first(pdu->opt_list);li;li = coap_list_next(li)) {
			coap_list_this(li,NULL,(void **)&opt);
			coap_list_node_delete(pdu->opt_list, (void *)&opt->code, sizeof(char));
		}
    }
    memset(&pdu->hdr, 0, sizeof(coap_hdr_t));
    memset(pdu->payload, 0, COAP_MAX_PDU_SIZE);
    pdu->payload_len = 0;
    return 1;
}

int coap_pdu_copy(coap_pdu_t *old, coap_pdu_t *new) {
    coap_option_t *opt;
    coap_list_index_t *li;


    memcpy(new, old, sizeof(old));
    new->hdr.optcnt = 0;
    if (old->opt_list != NULL) {
		for (li = coap_list_first(old->opt_list);li;li = coap_list_next(li)) {
			coap_list_this(li,NULL,(void **)&opt);
			coap_pdu_option_insert(new, opt->code, opt->data, opt->len);
		}
    }
    sprintf(new->payload, old->payload, old->payload_len);
    new->addr = old->addr;
    return 1;
}



int coap_pdu_option_get(coap_pdu_t *pdu, char code, coap_option_t **opt) {
    return coap_list_node_get(pdu->opt_list, (void *)&code, sizeof(char), (void **)opt);
}

int coap_pdu_option_unset(coap_pdu_t *pdu, char code) {
	if (!coap_list_node_delete(pdu->opt_list, (void *)&code, sizeof(char))) {
		return 0;
	}
	if (pdu->hdr.optcnt > 0)
		pdu->hdr.optcnt--;
	return 1;
}

void coap_pdu_clean_common_options(coap_pdu_t *pdu)
{
  coap_pdu_option_unset(pdu, COAP_OPTION_PROXY_URI);
  coap_pdu_option_unset(pdu, COAP_OPTION_ETAG);
  coap_pdu_option_unset(pdu, COAP_OPTION_LOCATION_PATH);
  coap_pdu_option_unset(pdu, COAP_OPTION_URI_PATH);
  coap_pdu_option_unset(pdu, COAP_OPTION_URI_QUERY);
  coap_pdu_option_unset(pdu, COAP_OPTION_IF_MATCH);
  coap_pdu_option_unset(pdu, COAP_OPTION_IF_NONE_MATCH);
  coap_pdu_option_unset(pdu, COAP_OPTION_ACCEPT);
  coap_pdu_option_unset(pdu, COAP_OPTION_URI_PORT);
}

void coap_pdu_print(coap_pdu_t *pdu) {
	coap_option_t *opt;
	coap_list_index_t *li;
	char host[INET6_ADDRSTRLEN];

    fprintf(stderr,"Version: %d\n",pdu->hdr.version);
    fprintf(stderr,"Type: %d\n",pdu->hdr.type);
    fprintf(stderr,"Code: %d\n",pdu->hdr.code);
    fprintf(stderr,"Option count: %d\n",pdu->hdr.optcnt);
    fprintf(stderr,"Id: %u\n",pdu->hdr.id);
/*
    fprintf(stderr,"expiration timestamp: %d\n",(int)pdu->timestamp+COAP_DEFAULT_MAX_AGE);
    fprintf(stderr,"today timestamp: %d\n",time(NULL));
*/
    inet_ntop(AF_INET6, &(pdu->addr.sin6_addr), host, INET6_ADDRSTRLEN);
    fprintf(stderr,"address: %s\n",host);

    fprintf(stderr,"port: %d\n",pdu->addr.sin6_port);

	for (li = coap_list_first(pdu->opt_list);li;li = coap_list_next(li)) {
		coap_list_this(li,NULL,(void **)&opt);
		fprintf(stderr,"Option: %u: %s %d\n", opt->code, opt->data, opt->len);
	}

	if (pdu->payload)
		fprintf(stderr,"PAYLOAD: %s\n", pdu->payload);

}

int coap_pdu_parse_uri(char *coap_uri, struct sockaddr_in6 *addr,
	char **uri_path, char **uri_query)
{
	char *p, *uri_host, *port_tmp;
	int len = strlen(coap_uri);
	int uri_port;

	/* URI must start with coap:// */
	if (strncmp(coap_uri, "coap://", 7) != 0) {
		return 0;
	}
	p = &coap_uri[7];

	/* Next is the IPv6 address */
	if (*p == '[') {
		uri_host = &coap_uri[8];
		while (p - coap_uri < len && *p != ']')
			p++;
	}

	if (*p == ']') {
		*p = '\0';
	} else {
		return 0;
	}

	/* Look for port */
	if (p - coap_uri + 2 < len && *(++p) == ':') {
		port_tmp = ++p;
		while (p - coap_uri < len && isdigit(*p) && *p != '/')
			p++;
		if (*p == '/') {
			*p = '\0';
		}
		uri_port = atoi(port_tmp);
		*p = '/';
	} else {
		uri_port = COAP_DEFAULT_PORT;
	}


	/* Look for URI */
	if (p - coap_uri + 1 < len && *p == '/') {
		*p = '\0';
		*uri_path = ++p;
		while (p - coap_uri < len && *p != '?')
			p++;
	}
	/* Look for QUERY STRING */
	if (p - coap_uri + 1 < len && *p == '?') {
		*p = '\0';
		*uri_query = ++p;
		while (p - coap_uri < len)
			p++;
	}

	if (!coap_net_address_create(addr, uri_host, uri_port)) {
		return 0;
	}
	return 1;
}


int coap_pdu_parse_query(char *query, char *variable, int len, char **value) {
	char *p, *var;
	p = query;

	while((p - query) < len) {
		var = p;
		while((p - query) < len && *p != '=' && *p != '\0') p++;
		*p = '\0';

		if ((p - query) < len) {
			p++;
		} else {
			return 0;
		}
		*value = p;

		while((p - query) < len && *p != '&' && *p != '\0') p++;
		*p = '\0';
		if (strcmp(var, variable) == 0) {
			return 1;
		}
		if ((p - query) < len) p++;
	}
	*value = NULL;
	return 0;
}
