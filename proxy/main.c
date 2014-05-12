#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <db.h>
#include <sqlite3.h>

#include "sockset.h"
#include "connection.h"
#include "resource.h"
#include "fcgi/fcgiapp.h"
#include "../coap/coap_list.h"
#include "../coap/coap_resource.h"
#include "../coap/coap_client.h"
#include "../coap/coap_server.h"
#include "../coap/coap_net.h"
#include "../coap/coap_pdu.h"
#include "../coap/coap_rd.h"

typedef struct {
	sockset_t ss; /* SockeSet contain the list of available
						sockets to use in the selector. */
	DB *cache_db; /* Cache database. */
	char *database_path; /* Resources database */
	int unix_fd; /* FastCGI UNIX listening socket. */
	int coap_fd; /* CoAP UDP socket listening socket. */
	int websocket_fd; /* WebSocket TCP listening socket. */
	int response_timeout; /* Timeout to wait for a response.*/
	coap_list_t *resources; /* List containing the available resources. */
	coap_server_t *server; /* CoAP server */
	coap_client_t *client; /* CoAP client*/
	coap_list_t *client_connections;
} context_t;

context_t *context;

void client_response_received(coap_pdu_t *response, void *data) {
	coap_list_t *connections;
	coap_context_connection_t *conn;
	coap_option_t *token, *uri, *obs;
	coap_list_index_t *li;
	connection_t *connection;
	coap_rd_entry_t *entry;

	coap_pdu_option_get(response, COAP_OPTION_TOKEN, &token);

	if (coap_list_node_get(context->client->context->pending_queue, token->data, strlen(token->data), (void **)&conn)) {
		/* Is an Update? */
		if(coap_pdu_option_get(response, COAP_OPTION_OBSERVE, &obs)){
			coap_pdu_option_get(conn->pdu, COAP_OPTION_URI_PATH, &uri);
			if	(!coap_list_node_get(context->server->rd->entries, uri->data, strlen(uri->data), (void **)&entry)){
				coap_debug_error("Resource not found in context->server->rd->entries\n");
				return;
			}
			for (li = coap_list_first(entry->observers);li; li = coap_list_next(li)) {
				coap_list_this(li, NULL,(void **)&connection);
				connection_response_send(connection, response);
			}
		} else {
			connection_response_send(conn->connection, response);
			if (conn->connection->type == FASTCGI) {
				coap_list_node_delete(context->client->context->pending_queue, token->data, strlen(token->data));
				coap_debug_error("FASTCGI\n");
				} else if (conn->connection->type == WEBSOCKET) {
					coap_debug_error("websocket\n");
				}
			}
		}
}

static int process_fastcgi_request(connection_t **connection)
{
	struct sockaddr_in caddr;
	resource_t *res;
	coap_option_t *token;
	coap_rd_entry_t *entry;
	char *request_path, *query_string, *method;

	/* Create new active connection */
	if (!connection_create(FASTCGI, connection)) {
		coap_debug_error("Cannot receive a socket\n");
		return 0;
	}

    FCGX_InitRequest(FASTCGI_UNIX_REQUEST(*connection), 0, 0);

    if (FCGX_Accept_r(FASTCGI_UNIX_REQUEST(*connection)) < 0) {
		connection_delete(*connection);
	    return 0;
	}

	request_path = FCGX_GetParam("REQUEST_URI", FASTCGI_UNIX_REQUEST(*connection)->envp);
	query_string = FCGX_GetParam("QUERY_STRING", FASTCGI_UNIX_REQUEST(*connection)->envp);
	method = FCGX_GetParam("REQUEST_METHOD", FASTCGI_UNIX_REQUEST(*connection)->envp);

	if (request_path[0] == '/')
		request_path++;

	if (!coap_list_node_get(context->server->rd->entries, request_path, strlen(request_path), (void **)&entry)) {
		coap_debug_error("Couldn't find the resource\n");
		goto error;
	}

	/* create the CoAP Request */
		if (!coap_pdu_create((*connection)->pdu)) {
			coap_debug_error("CoAP PDU cannot be created\n");
			goto error;
		}

		coap_debug_error("setting connection...\n");
		if (!coap_pdu_request((*connection)->pdu, request_path, method, 0)) {
			coap_debug_error("CoAP PDU cannot be created\n");
			goto error;
		}

		/* Define Address */
		(*connection)->pdu->addr = entry->coap_res->server_addr;

		/* Send he CoAP Request to WSN */
		coap_client_send(context->client, (*connection)->pdu);

	//	coap_pdu_option_get((*connection)->coap_request, COAP_OPTION_TOKEN, &token);

		//coap_list_node_insert(context->client->context->pending_queue, token->data, strlen(token->data), *connection);

	return 1;
error:
	FCGX_Finish_r(FASTCGI_UNIX_REQUEST(*connection));
	connection_delete(*connection);
	return 0;
}

static int process_websocket_request(connection_t **connection){
	int sock, obs = 0;
	struct sockaddr_in caddr;
	int len = sizeof(caddr);
	resource_t *res;
	coap_rd_entry_t *entry;
	char *method;
	char *uri;
	coap_option_t *token;
	char str[INET6_ADDRSTRLEN];
	coap_list_index_t *li_rd, *li_res;
	coap_context_connection_t *conn;

	if ((sock = accept(context->websocket_fd, (struct sockaddr*) &caddr, &len)) < 0) {
		coap_debug_error("Cannot receive a socket\n");
	    return 0;
	}

	if (!connection_create(WEBSOCKET,  connection)){
		coap_debug_error("Cannot establish a WebSocket connection\n");
		connection_delete(*connection);
		return 0;
	}

	uri = (char *) malloc(10);
	method = (char *) malloc(5);

	if (!websocket_do_handshake(sock, uri, method)) {
		coap_debug_error("Bad Handshake\n");
		goto error;
	}

	WEBSOCKET_SOCKET(*connection) = sock;

	if (!coap_context_connection_create(&conn)) {
		return 0;
	}

	conn->connection = *connection;

	/* Check the presence of the observe request */
	if (coap_observe_request(uri)){
		printf("observe\n");
		obs = 1;
		uri = &uri[strlen("observe/")];
	}

	/* Search for entry in RD */
	if	(!coap_list_node_get(context->server->rd->entries, uri, strlen(uri), (void **)&entry)){
		coap_debug_error("Resource not found in context->server->rd->entries\n");
		goto error;
	}

	/* ADD THE OBSERVER TO THE LIST IN THE RD_ENTRY */
	if(obs && (entry->observers->first == NULL)){

		coap_list_node_insert(entry->observers, ((int *)(*connection)->data), sizeof(int),*connection);
		conn->is_observe = 1;

		/* create the CoAP Request */
			if (!coap_pdu_create(&conn->pdu)) {
				coap_debug_error("CoAP PDU cannot be created\n");
				goto error;
			}

			if (!coap_pdu_request(conn->pdu, uri, &method, obs)) {
				coap_debug_error("CoAP PDU cannot be created\n");
				goto error;
			}

		/* Define Address */
		conn->pdu->addr = entry->coap_res->server_addr;

		/* Send the CoAP Request to WSN */
		coap_client_send(context->client, conn->pdu);
		coap_pdu_option_get(conn->pdu, COAP_OPTION_TOKEN, &token);
		coap_debug_error("code %X len %X data %s\n", token->code, token->len, token->data);
		coap_list_node_insert(context->client->context->pending_queue, token->data, strlen(token->data), conn);
		return 1;
	} else {
		coap_list_node_insert(entry->observers, ((int *)(*connection)->data), sizeof(int),*connection);
		return 1;
	}

	error:
		connection_delete(*connection);
		close(sock);
		free (uri);
		free (method);
		return 0;
}


static int rebuild_proxy_resources() {
	resource_t *res;
	int rc;
	sqlite3_stmt *stmt;
	sqlite3 *db;

	rc = sqlite3_open(context->database_path,&db);

	if(rc){
	    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	    sqlite3_close(db);
	    return 0;
	}

	if (sqlite3_prepare(
			db,            /* Database handle */
	        "SELECT rm.path, rd.name, r.path as rpath FROM resource_mapping as rm JOIN resource as r ON r.id = rm.resource_id"
	        " JOIN resource_directory as rd ON r.resource_directory_id = rd.id",       /* SQL statement, UTF-8 encoded */
	        strlen("SELECT rm.path, rd.name, r.path as rpath FROM resource_mapping as rm JOIN resource as r ON r.id = rm.resource_id"
	    	        " JOIN resource_directory as rd ON r.resource_directory_id = rd.id")+1, /* Maximum length of zSql in bytes. */
	        &stmt,  /* OUT: Statement handle */
	        NULL     /* OUT: Pointer to unused portion of zSql */
    ) < 0);

	while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	    coap_rd_entry_t *entry;
	    coap_resource_t *resource;
	    resource_t *res;
		char *rd_name, *path, *mapping_path;

		rd_name = (char *)sqlite3_column_text (stmt, 1);
	    path = (char *)sqlite3_column_text (stmt, 2);
	    mapping_path = (char *)sqlite3_column_text (stmt, 0);

		if (coap_list_node_get(context->server->rd->entries, rd_name,strlen(rd_name),
			(void **)&entry)) {
			if (coap_list_node_get(entry->observers, path, strlen(path),(void **)&resource)) {

				if (!coap_list_node_get(context->resources, mapping_path, strlen(mapping_path),(void **)&res)
					|| res->connections->first == NULL) {
					resource_create(mapping_path, resource, &res);
					coap_list_node_insert(context->resources, mapping_path, strlen(mapping_path), res);
					sockset_add(&context->ss, res->client->context->socket);
				}
			}
	    }
	}
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}

static void serve() {
	int rc;
	fd_set lset;
	struct timeval timeout;
	connection_t *connection;
	coap_list_index_t *li;
	resource_t *res;

	lset = context->ss.rdfs;

	while(1) {
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		do {
			lset = context->ss.rdfs;
			rc = select(context->ss.max+1, &lset, NULL, NULL, &timeout);
		} while(rc < 0 && errno == EINTR);

		if (FD_ISSET(context->client->context->socket, &lset)) {
			coap_debug_error("client\n");
			coap_client_read(context->client);
			coap_client_dispatch(context->client);
		} else {
			coap_client_clean_pending(context->client);
		}

		if (FD_ISSET(context->server->context->socket, &lset)) {
			coap_debug_error("server\n");
			coap_server_read(context->server);
			coap_server_dispatch(context->server);
		} else {
		   	coap_server_clean_pending(context->server);
		}

		/* New connection from the web server */
		if (FD_ISSET(0, &lset)) {
#ifdef NDEBUG
			fprintf(stderr,"NEW FASTCGI CONNECTION\n");
#endif
			process_fastcgi_request(&connection);
		}
		/* New connection from Websocket server*/
		if (FD_ISSET(context->websocket_fd, &lset)) {
			process_websocket_request(&connection);
#ifdef NDEBUG
			fprintf(stderr,"NEW WEBSOCKET CONNECTION\n");
#endif
		}
	}
}


static void context_delete(struct coap_node_t *node) {

	context_t *context;

	context = (context_t *)node->data;

	socket_init(context->ss);


		//DB *cache_db; /* Cache database. */

		//sqlite3 *resources_db; /* Resources database */

	close(context->unix_fd);
	close(context->coap_fd);
	close(context->websocket_fd);

	coap_server_delete(context->server);
}

void handler_resource(coap_pdu_t *request) {
	request->hdr.type = COAP_MESSAGE_ACK;
	request->hdr.code = COAP_RESPONSE_205;
	sprintf(request->payload, "%s", "HOLA");
	request->payload_len = 4;
	coap_pdu_option_unset(request, COAP_OPTION_URI_PATH);
	coap_pdu_print(request);
}

static int update_rd(int action, coap_rd_entry_t *entry, coap_resource_t *res){
	int rc;
	sqlite3_stmt *stmt;
	sqlite3 *db;
	int entry_id;

	char *error;

	/* Initialize the resource database */
	if (sqlite3_open(context->database_path, &db) < 0) {
	    coap_debug_error("Can't open database: %s\n", sqlite3_errmsg(db));
	    sqlite3_close(db);
	    return 0;
	}

	/* Check if the entry exists. Create it if not. */
	if (sqlite3_prepare(
		db,
		"SELECT id FROM resource_directory WHERE name = ?",
		strlen("SELECT * FROM resource_directory WHERE name = ?")+1,
		&stmt,
		NULL
	) < 0) return 0;

	sqlite3_bind_text (stmt, 1, entry->name, strlen(entry->name), SQLITE_STATIC);

	rc = sqlite3_step(stmt);

	if (rc == SQLITE_ROW) {
		entry_id = sqlite3_column_int (stmt, 0);
		fprintf(stderr,"ENTRy FOUND: %d\n", entry_id);
		sqlite3_finalize(stmt);
	} else {
		if (sqlite3_prepare(
			db,
			"INSERT INTO resource_directory (name) VALUES (?)",
			strlen("INSERT INTO resource_directory (name) VALUES (?)")+1,
			&stmt,
			NULL
		) < 0) return 0;
		sqlite3_bind_text (stmt, 1, entry->name, strlen(entry->name), SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		entry_id =  sqlite3_last_insert_rowid(db);
	}

	switch (action) {
		case RD_UPDATE:
			if (sqlite3_prepare(
				db,
				"UPDATE resources SET path = ?, host = ?, port = ?, rt = ?, if = ?, sz = ?, subscription = ? WHERE name = ?",
				strlen("UPDATE resources SET path = ?, host = ?, port = ?, rt = ?, if = ?, sz = ?, subscription = ? WHERE name = ?")+1,
				&stmt,
				NULL
			) < 0) return 0;

			fprintf(stderr,"error1: %s\n", sqlite3_errmsg(db));
			(char *)sqlite3_bind_text (stmt, 1, res->path, 4, SQLITE_STATIC);
			(char *)sqlite3_bind_text (stmt, 2, "localhost", 9, SQLITE_STATIC);
			sqlite3_bind_int (stmt, 3, 8000);
			(char *)sqlite3_bind_text (stmt, 4, res->rt, 0, SQLITE_STATIC);
			(char *)sqlite3_bind_text (stmt, 5, res->ifd, 0, SQLITE_STATIC);
			sqlite3_bind_int (stmt, 6, res->sz);
			sqlite3_bind_int (stmt, 7, res->is_observe);
			(char *)sqlite3_bind_text (stmt, 7, res->name, 0, SQLITE_STATIC);
			break;
		case RD_INSERT:
			if (sqlite3_prepare(
				db,
				"INSERT INTO resource (resource_directory_id, path, host, port, rt, if, sz, subscription) VALUES (?,?,?,?,?,?,?,?)",
				strlen("INSERT INTO resource (resource_directory_id, path, host, port, rt, if, sz, subscription) VALUES (?,?,?,?,?,?,?,?)")+1,
				&stmt,
				NULL
			) < 0) return 0;


			sqlite3_bind_int (stmt, 1, entry_id);
			sqlite3_bind_text (stmt, 2, res->path, 4, SQLITE_STATIC);
			sqlite3_bind_text (stmt, 3, "localhost", 9, SQLITE_STATIC);
			sqlite3_bind_int (stmt, 4, 8000);
			sqlite3_bind_text (stmt, 5, res->rt, 0, SQLITE_STATIC);
			sqlite3_bind_text (stmt, 6, res->ifd, 0, SQLITE_STATIC);
			sqlite3_bind_int (stmt, 7, res->sz);
			sqlite3_bind_int (stmt, 8, res->is_observe);

			break;
		case RD_DELETE:
			if (sqlite3_prepare(
				db,
				"DELETE FROM resources WHERE name = ?",
				strlen("DELETE FROM resources WHERE name = ?")+1,
				&stmt,
				NULL
			) < 0) return 0;

			sqlite3_bind_text (stmt, 1, res->name, 0, SQLITE_STATIC);
			break;
		default:
			sqlite3_close(db);
			return 0;
	}

	sqlite3_step(stmt);
	fprintf(stderr,"%s\n", sqlite3_errmsg(db));
	sqlite3_finalize(stmt);

	sqlite3_close(db);
    return 0;
}

static int get_resource_directory() {
    int rc;
    sqlite3_stmt *stmt;
    sqlite3 *db;

    rc = sqlite3_open(context->database_path,&db);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    if (sqlite3_prepare(
        db,
        "SELECT r.path, r.name, r.host, r.port, r.rt, r.if, r.sz, r.subscription, rd.name as dirname FROM resource as r JOIN resource_directory as rd ON r.resource_directory_id = rd.id",
        strlen("SELECT r.path, r.name, r.host, r.port, r.rt, r.if, r.sz, r.subscription, rd.name as dirname FROM resource as r JOIN resource_directory as rd ON r.resource_directory_id = rd.id")+1,
        &stmt,
        NULL
    ) < 0) return 0;

    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    	coap_resource_t *resource;
        coap_rd_entry_t *entry;

    	// Create resource
    	if (!coap_resource_create((char *)sqlite3_column_text(stmt, 0),
    			(char *)sqlite3_column_text(stmt, 1),
    			(char *)sqlite3_column_text(stmt, 4),
    			(char *)sqlite3_column_text(stmt, 5),
    			sqlite3_column_int(stmt, 6),
    			sqlite3_column_int(stmt, 7),
    			NULL,
    			&resource
    	)) return 0;


    	//obtain socketadd_in6 struct
    	if (!coap_net_address_create(&resource->server_addr, (char *)sqlite3_column_text(stmt, 2), sqlite3_column_int(stmt, 3))) return 0;

    	//Create entry
    	if (!coap_rd_entry_create((char *)sqlite3_column_text(stmt, 8), &entry)) return 0;


    	coap_list_node_insert(entry->coap_res, resource->path, strlen(resource->path), resource);
    	coap_rd_entry_add(context->server->rd, entry);

    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}


static int context_create(char *cache_database_path, char *resource_database_path, char *fastcgi_socket, context_t **context) {
    coap_resource_t *res;

    /* Create context */
    if (((*context) = (context_t *) malloc (sizeof(context_t))) == NULL) {
    	coap_debug_error("Cannot create context\n");
    	return 0;
    }

	(*context)->database_path = resource_database_path;
    (*context)->response_timeout = RESPONSE_TIMEOUT;

    /* Init socket */
	sockset_init(&(*context)->ss);

	/* Create connections list */
    if (!coap_list_create(resource_node_delete, NULL, &(*context)->resources)) {
    	coap_debug_error("Cannot create connections list\n");
    	return 0;
    }

    /* Create CoAP server */
  	if (!coap_server_create(8007, 0, &(*context)->server))
  		coap_debug_error("server not created");

    if (!coap_resource_create("prox",  "Prox", "", "", 0, 0, handler_resource, &res))
    	coap_debug_error("resource not created\n");

   	if (!coap_server_resource_add((*context)->server, res))
   		coap_debug_error("resource not added\n");

   	/* Create CoAP Client */
    if (!coap_client_create(NULL, NULL, client_response_received, NULL, &(*context)->client)) {
        coap_debug_error("Error creating the client.\n");
       	return 0;
       }

    sockset_add(&(*context)->ss, (*context)->client->context->socket);

    if (!coap_list_create(connection_node_delete, NULL, &(*context)->client_connections)) {
    	return 0;
    }

    // Update RD callback
   	(*context)->server->rd_update_callback = update_rd;

   	if (!get_resource_directory()) exit(1);

   	rebuild_proxy_resources();

   	/* Add CoAP server to sockset */
   	sockset_add(&(*context)->ss, (*context)->server->context->socket);
   	sockset_add(&(*context)->ss, (*context)->server->client->context->socket);

   	/* Create and init UNIX socket for FastCGI */
	(*context)->unix_fd = OS_CreateLocalIpcFd(fastcgi_socket, 50);
	close(0);
	dup((*context)->unix_fd);
	close((*context)->unix_fd);
	FCGX_Init();

	/* Add to context */
	sockset_add(&(*context)->ss, 0);

	/* Create a listening TCP socket (WebSocket server) */
	if (((*context)->websocket_fd = coap_net_tcp_bind(9000)) < 0) {
		coap_debug_error("Error binding Websocket server on port 9000\n");
		return 0;
	}
	/* Add WebSocket to sockset */
	sockset_add(&(*context)->ss, (*context)->websocket_fd);

	/* Initialize the cache database */
	if (!coap_cache_create(cache_database_path, &(*context)->cache_db)) {
		coap_debug_error("Cannot create the cache DB: %s\n", cache_database_path);
	    return 0;
	}

    return 1;
}

static void usage( const char *program, const char *version) {
  const char *p;

  p = strrchr( program, '/' );
  if ( p )
    program = ++p;

  fprintf( stderr, "%s v%s -- CoAP daemon\n"
	   "(c) 2011 Pol Moreno <pol.moreno@entel.upc.edu>\n\n"
	   "usage: %s [-c cache database_path] [-r resource database path] [-f fastcgi_socket_path]\n\n"
	   "\t-c file\t\tCache database path.\n"
	   "\t-r file\t\tResource database path.\n"
	   "\t-f file\t\tFastCGI UNIX socket path.\n",
	   program, version, program );
}

int main(int argc, char *argv[])
{
	int opt;
	coap_resource_t *res;
	char *cache_database_path = NULL, *resource_database_path = NULL, *fastcgi_socket = NULL;

	while ((opt = getopt(argc, argv, "c:r:f:")) != -1) {
		switch (opt) {
	    	case 'c' :
	    		cache_database_path = optarg;
	    		break;
	    	case 'r' :
	    		 resource_database_path = optarg;
	    		 break;
	    	case 'f':
	    		fastcgi_socket = optarg;
	    		break;
	    	case 'n':

	    	default:
	    		usage( argv[0], "1.0" );
	    		exit( 1 );
	    }
	}

	if (!cache_database_path) {
		if ((cache_database_path = (char *)malloc(sizeof(char)*(strlen("/tmp/coap.db")+1))) == NULL) {
			coap_debug_error("Database path cannot be created.\n");
			return 1;
		}
		sprintf(cache_database_path,"%s", "/tmp/coap.db");
	}

	if (!resource_database_path) {
		if ((resource_database_path = (char *)malloc(sizeof(char)*(strlen("/tmp/coap_proxy.sqlite")+1))) == NULL) {
			coap_debug_error("Database path cannot be created.\n");
			return 1;
		}
		sprintf(resource_database_path,"%s", "/tmp/coap_proxy.sqlite");
	}

	if (!fastcgi_socket) {
		if ((fastcgi_socket = (char *)malloc(sizeof(char)*(strlen("/tmp/coap_fastcgi")+1))) == NULL) {
			coap_debug_error("FastCGI socket path cannot be created.\n");
			return 1;
		}
		sprintf(fastcgi_socket,"%s","/tmp/coap_fastcgi");
	}

	/* Create context */
	if (!context_create(cache_database_path, resource_database_path, fastcgi_socket, &context))
		return 1;

	serve();

	return 0;
}
