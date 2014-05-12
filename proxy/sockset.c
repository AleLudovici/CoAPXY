#include <sys/types.h>

#include "sockset.h"

#define max(i,j) ((i>j) ? (i) : (j))

void sockset_init(sockset_t *ss)
{
    FD_ZERO(&ss->rdfs);
}

void sockset_add(sockset_t *ss, int sd)
{
    FD_SET(sd, &ss->rdfs);
    ss->max = max(ss->max, sd);
}

void sockset_remove(sockset_t *ss,int sd)
{
    int i;
    FD_CLR(sd,&ss->rdfs);
    if(sd == ss->max) {
        for(i=ss->max;i>0;i--){
            if(FD_ISSET(i,&ss->rdfs)) {
                ss->max = i;
                break;
            }
        }        
    }
}
