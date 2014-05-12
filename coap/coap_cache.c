#include <stdlib.h>
#include <string.h>
#include <db.h>

#include "coap_md5.h"
#include "coap_cache.h"
#include "coap_pdu.h"

void coap_hash(coap_pdu_t *pdu,char *digest)
{
    MD5_CTX mdContext;
    coap_option_t *opt;
    coap_list_index_t *li;

    
    coap_MD5Init (&mdContext);   

    /* Add version */
    coap_MD5Update(&mdContext,(void *)&pdu->hdr.version, sizeof(char));
    /* Add message type */
    coap_MD5Update(&mdContext,(void *)&pdu->hdr.type, sizeof(char));
    /* Add message code */
    coap_MD5Update(&mdContext,(void *)&pdu->hdr.code, sizeof(char));


    for (li = coap_list_first(pdu->opt_list);li;li = coap_list_next(li)) {
     	coap_list_this(li,NULL,(void **)&opt);
     		switch(opt->code) {
     	    	// skip known options
     	        case COAP_OPTION_MAXAGE :
     	        case COAP_OPTION_ETAG :
     	        case COAP_OPTION_TOKEN :
     	        	break;
     	        default:
     	            coap_MD5Update(&mdContext,(void *)&opt->code, sizeof(char));
     	            coap_MD5Update(&mdContext,(void *)&opt->data, opt->len);
     	            break;
     	    }
    }
    coap_MD5Final (&mdContext);
    memcpy(digest,mdContext.digest,16); 
}

int coap_cache_create(char *database_path, DB **database)
{

    int ret;
    uint32_t flags;
  
    ret = db_create(database,NULL,0);
    if (ret != 0) {
        return 0;
    }   
    flags = DB_CREATE;
  
    ret = (*database)->open(*database,
                           NULL,
                           database_path,
                           NULL,
                           DB_BTREE,
                           flags,0);
    if (ret != 0) {
        // destroy database
        return 0;
    }
    return 1;
}

int coap_cache_add(DB *database, coap_pdu_t *request, coap_pdu_t *response)
{
    DBT key,data;
    int ret;
    
    coap_cache_t cache;
    unsigned int packet_len;    
  
    char index[16];


    memset(&key,0,sizeof(key));
    memset(&data,0,sizeof(data));
    memset(&cache,0,sizeof(coap_cache_t));

    coap_hash(request, index);

    key.data = index;
    key.size = 16; /* md5 size */

    coap_pdu_packet_write(response, cache.packet,&packet_len);

    cache.timestamp = response->timestamp;
    cache.packet_len = packet_len;
    cache.addr = request->addr;


    data.data = (void *)&cache;
    data.size = sizeof(cache);
  
    ret = database->put(database,
                            NULL,
                            &key,
                            &data,
                            DB_NOOVERWRITE);
    if (ret == DB_KEYEXIST) {
    	return 0;
    }
    database->sync(database,0);
#ifdef NDEBUG
	fprintf(stderr,"Cache added\n");
#endif
    return 1;
}

int coap_cache_get(DB *database, coap_pdu_t *request, coap_pdu_t **pdu)
{
    DBT key, data;
    int ret;
    coap_cache_t cache;
    unsigned int packet_len;
    /* MAXAGE option of the stored packet */
    int maxage;
    coap_option_t *opt;
    /* Temporary pdu timestamp */
    time_t ttimestamp;
    char index[16];

	coap_hash(request, index);

    memset(&key,0,sizeof(key));
    memset(&data,0,sizeof(data));
    memset(&cache,0,sizeof(coap_cache_t));

    key.data = index;
    key.size = 16; /* md5 size */

    data.data = (void *)&cache;
    data.ulen = sizeof(cache); /* maximum available space into packet */


    ret = database->get(database,NULL,&key,&data,0);
    if (ret == DB_NOTFOUND) {
    	return 0;
    }

    /* Get stored packet timestamp */
    ttimestamp = ((coap_cache_t *)data.data)->timestamp;
    /* Read the stored packet into a coap_pdu_t structure */

    coap_pdu_create(pdu);
    coap_pdu_packet_read(((coap_cache_t *)data.data)->packet,((coap_cache_t *)data.data)->packet_len, *pdu);

    (*pdu)->timestamp = ttimestamp;
    (*pdu)->addr = ((coap_cache_t *)data.data)->addr;

    return 1;
}

int coap_cache_revalidate(DB *database, coap_pdu_t *request, coap_pdu_t **response)
{
	char digest[16];
	coap_pdu_t *pdu;

	coap_hash(request, digest);
	if (coap_cache_get(database, request, &pdu)) {
		int maxage;
		coap_option_t *opt;
		coap_option_t *etag;

		if (!coap_pdu_option_get(pdu,COAP_OPTION_ETAG,&opt)
			|| !coap_pdu_option_get(request,COAP_OPTION_ETAG,&etag)
			|| strcmp(etag->data, opt->data) != 0) {

			return 0;
		}

		/* Update time stamp */
		pdu->timestamp = time(NULL);

		/* Get Max Age from received response */
		if (!coap_pdu_option_get(*response,COAP_OPTION_MAXAGE,&opt)) {
			/* MAXAGE not found. Use default value. */
			maxage = COAP_DEFAULT_MAX_AGE;
		} else {
			maxage = atoi(opt->data);
		}

		if (!coap_pdu_option_get(pdu,COAP_OPTION_MAXAGE,&opt)) {
			char maxage_opt[8];
			sprintf(maxage_opt, "%d", maxage);
			coap_pdu_option_insert(pdu, COAP_OPTION_MAXAGE, maxage_opt, strlen(maxage_opt));
		}

		memcpy(opt->data, &maxage, sizeof(int));
		opt->len = sizeof(int);
		/* Clean received response */
		coap_pdu_clean(*response);
		/* Return the stored response */
		response = &pdu;

		return 1;
	}
	return 0;
}

int coap_cache_delete(DB *database, coap_pdu_t *pdu)
{
    DBT key;
	char digest[16];

	coap_hash(pdu, digest);
    memset(&key,0,sizeof(key));  

    key.data = digest;
    key.size = 16; /* md5 size */
    return database->del(database,NULL,&key,0);
}
