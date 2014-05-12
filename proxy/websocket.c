#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include <math.h>

#include "websocket.h"
#include "openssl/evp.h"
#include "openssl/bio.h"
#include "openssl/buffer.h"

char *base64(const unsigned char *input, int length) {
	BIO *bmem, *b64;
	BUF_MEM *bptr;

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, length);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	char *buff = (char *)malloc(bptr->length);
	memcpy(buff, bptr->data, bptr->length-1);
	buff[bptr->length-1] = 0;

	BIO_free_all(b64);

	return buff;
}

char *interpret_key(const char *key) {
	const unsigned char *p, *algorithm;
	char *output;
	unsigned char key_encode[64];
	unsigned char *md;
	int i;
	size_t olen;
	EVP_MD *m;
	EVP_MD_CTX ctx;

	/*concatenate key with GUID*/
    p = key_encode;

    strcpy(p, key);
    p += strlen(key);

    strcpy(p, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    p += strlen ("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    /*SHA-1 hash*/
    OpenSSL_add_all_digests();

    algorithm = "sha1";

    if(!(m = (EVP_MD *) EVP_get_digestbyname(algorithm)))
    	return 1;

    if(!(md = (unsigned char *) malloc(EVP_MAX_MD_SIZE)))
    	return 1;

    EVP_DigestInit(&ctx, m);
    EVP_DigestUpdate(&ctx, (const void *)key_encode, strlen(key_encode));
    EVP_DigestFinal(&ctx, md, &olen);

    //Base64Encode(md, output, olen);
    output = base64(md, olen);

	return output;
}


static int write_fd(int fd, void *buf, int len) {
    int n = 0;

    while (n < len) {
        n += write(fd, buf+n, len-n);
    }
    return len;
}

static int handshake(int sock, unsigned char *key_token) {
	char response[500];
	char *p, *new_key;
	int n,len;
    char c;

	/* prepare response key */
	new_key = interpret_key(key_token);

	p = response;
	strcpy(p,"HTTP/1.1 101 Switching Protocol\x0d\x0a"
			"Upgrade: WebSocket\x0d\x0a");
	p += strlen("HTTP/1.1 101 Switching Protocol\x0d\x0a"
					  "Upgrade: WebSocket\x0d\x0a");
	strcpy(p,"Connection: Upgrade\x0d\x0a"
		    "Sec-WebSocket-Accept: ");
	p += strlen("Connection: Upgrade\x0d\x0a"
		    "Sec-WebSocket-Accept: ");
	strcpy(p, new_key);
	p += strlen(new_key);
	strcpy(p,   "\x0d\x0a\x0d\x0a");
	p += strlen("\x0d\x0a\x0d\x0a");

    if(!write_fd(sock, (void *)response, p-response))
		return 0;


    return 1;
}

int websocket_send(int sock, char *message) {
    unsigned char buffer[strlen(message)+2];
    unsigned char *b;

    b = buffer;

    *b = 0x81; //FIN 1 - RSV1/2/3 0 - OPCODE 0x1
    b++;

    *b = strlen(message); //payload_len
    b++;

    memcpy(b, message, strlen(message));

    if (!write_fd(sock,buffer,strlen(message)+2)) return 0;

    return 1;
}

int websocket_do_handshake(int sock, char *uri, char *method) {
    int rcount;
    unsigned char head[WEBSOCKET_BUF_SIZE+3];
    int pos, keylen, URI_len = 0;
    int bsum;
    char *key, *host, *origin, *p, *q, *version, *weburi;
    memset(head, 0, sizeof(char)*(WEBSOCKET_BUF_SIZE+2) );

    rcount = read(sock,head,WEBSOCKET_BUF_SIZE);
    pos = 0;

    while (pos <= rcount && !(head[pos]=='\r' && head[pos+1]=='\n' && head[pos+2]=='\r' && head[pos+3]=='\n'))
        pos++;

    if(pos <= rcount) {
        if (head[pos+1] == '\r') {
            pos += 3;			
        } else {
            pos += 2;			
        }
	    bsum = rcount-pos;
	    head[pos] = '\0';

    }

    head[rcount] = '\0';

	if(bsum == 0) {
        /* Empty data */
		return 1;
	}

    p = head;
    printf("URI %s\n", p);


    while (*p) {
        q = p;

        while(*p && *p!='\r' && *p!='\n') *p++;
        *p = '\0';
        p++;
        if (strncmp(q,"Origin: ",strlen("Origin: ")) == 0) {
        	/* accept connection form anywhere */
            origin = &q[strlen("Origin: ")];
        } else if (strncmp(q,"Sec-WebSocket-Key: ",strlen("Sec-WebSocket-Key: ")) == 0) {
            key = &q[strlen("Sec-WebSocket-Key: ")];
        } else if (strncmp(q,"Host: ", strlen("Host: ")) == 0) {
        	host = &q[strlen("Host: ")];
        } else if (strncmp(q,"Sec-WebSocket-Version: ", strlen("Sec-WebSocket-Version: ")) == 0){
        	/* If Version is different from "13" send a HTTP 426 Upgrade Required as response and and a |Sec-WebSocket-Version| header
          field indicating the version(s) the server is capable of understanding*/
        	version = &q[strlen("Sec-WebSocket-Version: ")];
        } else if (strncmp(q,"GET ", strlen("GET ")) == 0){
        	strcpy(method, "GET");
        	/* point the URI Path*/
        	q += strlen("GET /");
        	weburi = q;
            /* Calculate the URI Length */
        	while (*q != ' '){
            	URI_len += 1;
        		q++;
            }
        	strncpy(uri, weburi, URI_len);
        	uri[URI_len] = '\0';
        }
        while (*p && (*p == '\r' || *p == '\n')) p++;
    }

    if(!handshake(sock, key))
    	return 0;

    return 1;
}
