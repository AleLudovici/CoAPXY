#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "coap_resource.h"
#include "../proxy/resource.h"

void aux_resource_delete(struct coap_node_t *node)
{
	resource_t *res;

	res = (resource_t *)node;
	coap_list_delete(res->connections);
	coap_client_delete(res->client);
	free(res);
}

int coap_resource_create(char *path, char *name,char *rt, char *ifd, int sz,
		int is_subscription,
		void (*handler)(coap_pdu_t *),
		coap_resource_t **resource)
	{

	if (!path || !name || ((*resource = (coap_resource_t *) malloc (sizeof(coap_resource_t))) == NULL)) {
		return 0;
	}

	memset(*resource,0,sizeof(coap_resource_t));

	(*resource)->sz = sz;

	/* Is a subscription? */
	(*resource)->handler = handler;
	(*resource)->is_subscription = is_subscription;

	/* If is a subscription, create the list*/
	if(is_subscription){
	    if (!coap_list_create(aux_resource_delete, NULL, &(*resource)->observers)) {
	    	coap_debug_error("Cannot create observers list\n");
	    	return 0;
	    }
	    fprintf(stderr,"Resource added as observable\n");
	}

	/* Reserve space for the URI field and the name and copy the desired values. */
	if (((*resource)->path = (char *) malloc (sizeof(char)*(strlen(path)+1))) != NULL) {
		sprintf((*resource)->path, "%s", path);
	} else {
		free(*resource);
		return 0;
	}

	if (((*resource)->name = (char *) malloc (sizeof(char)*(strlen(name)+1))) != NULL) {
		sprintf((*resource)->name, "%s", name);
	} else {
		free((*resource)->path);
		free(*resource);
		return 0;
	}

	if (rt && ((*resource)->rt = (char *) malloc (sizeof(char)*(strlen(rt)+1))) != NULL) {
		sprintf((*resource)->rt,"%s", rt);
	} else if (rt) {
		free((*resource)->path);
		free((*resource)->name);
		free(*resource);
		return 0;
	}

	if (ifd && ((*resource)->ifd = (char *) malloc (sizeof(char)*(strlen(ifd)+1))) != NULL) {
		sprintf((*resource)->ifd,"%s", ifd);
	} else if (ifd) {
		free((*resource)->path);
		free((*resource)->name);
		free((*resource)->rt);
		free(*resource);
		return 0;
	}
	return 1;

}

int coap_resource_delete(coap_resource_t *resource)
{
	free(resource->path);
	free(resource->name);
	free(resource->rt);
	free(resource->ifd);
	free(resource);
	return 1;
}

void coap_resource_print(coap_resource_t *resource)
{
	char str[INET6_ADDRSTRLEN];

	fprintf(stdout, "---RESOURCE: ---\n");
	fprintf(stdout, "Name: %s\n", resource->name);
	fprintf(stdout, "rt: %s\n", resource->rt);
	fprintf(stdout, "if: %s\n", resource->ifd);
	fprintf(stdout, "sz: %d\n", resource->sz);
	fprintf(stdout, "Subscription: %s\n", (resource->is_subscription) ? "Yes" : "No");

	inet_ntop(AF_INET6, &(resource->server_addr.sin6_addr), str, INET6_ADDRSTRLEN);
	fprintf(stdout, "%s\n", str); // prints "2001:db8:8714:3a90::12"
}

void coap_resource_node_delete(coap_node_t *node)
{
	coap_resource_t *res;

	res = (coap_resource_t *)node;

	coap_resource_delete(res);

}


int coap_resource_link_format(coap_resource_t *resource, char *data, int rd, int *len)
{
	char host[INET6_ADDRSTRLEN];

	if (rd) {
	    inet_ntop(AF_INET6, &(resource->server_addr.sin6_addr), host, INET6_ADDRSTRLEN);
		sprintf(data, "%s</coap://[%s]:%d/%s>", data, host, ntohs(resource->server_addr.sin6_port), resource->path);
	} else {
		sprintf(data, "%s</%s>", data, resource->path);
	}

	if (resource->rt)
		sprintf(data, "%s;rt=\"%s\"", data, resource->rt);
	if(resource->ifd)
		sprintf(data, "%s;if=\"%s\"", data, resource->ifd);
	if(resource->sz)
		sprintf(data, "%s;sz=%d", data, resource->sz);

	*len = strlen(data);
	return 1;
}
int coap_resource_update(coap_list_t *resources, char *data, int len)
{
	coap_resource_t *resource;
	char buffer[len];
	char *p, *item;
	char *path, *rt = NULL, *ifd = NULL, *sz = NULL;

	memcpy(buffer, data, len);
	p = buffer;

	while (p - buffer <= len) {
		while (*p != '<' && p - buffer <= len) p++;
		if (*p != '<' || p - buffer == len) return 1;
		p++;
		if (*p != '/' || p - buffer == len) return 1;

		p++;
		path = p;

		while(*p != '>' && p - buffer <= len) p++;

		if (*p == '>' && p - buffer < len) *p = '\0';
		else return 0;

		p++;

		item = p;
		
		rt = p;
		while (strncmp(p, ";rt=", 4) != 0) p++;
		if (strncmp(p, ";rt=", 4) == 0) {
			p += 4;
			rt = p;
			while(*p != ';' && *p != ',' && p - buffer < len) p++;
			*p = '\0';
		}
		ifd = item;
		p = item;
		while (strncmp(p, ";if=", 4) != 0 && *p != ',' && p - buffer < len) p++;
		if (strncmp(p, ";if=", 4) == 0) {
			p += 4;
			ifd = p;
			while(*p != ';' && *p != ',' && p - buffer < len) p++;
			*p = '\0';
		} else p = rt;
		sz = item;
		p = item;
		while (strncmp(p, ";sz=", 4) != 0 && *p != ',' && p - buffer < len) p++;
		if (strncmp(p, ";mz=", 4) == 0) {
			p += 4;
			sz = p;
			while(*p != ';' && *p != ',' && p - buffer < len) p++;
			*p = '\0';
		} else if (ifd) p = ifd;
		else p = rt;

		if (!coap_list_node_get(resources, path, strlen(path), (void **)&resource)) {

			if (!coap_resource_create(path, path, rt, ifd, atoi(sz), 0, NULL,&resource))
				return 0;

			if (!coap_list_node_insert(resources, path,	strlen(path), resource))
				return 0;
		} else {
			sprintf(resource->rt, "%s",rt);
			sprintf(resource->ifd, "%s",ifd);
			resource->sz = atoi(sz);
		}
	}
	return 1;
}
