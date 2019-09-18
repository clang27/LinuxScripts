#include "gfserver-student.h"

struct gfserver_t {
    int ss_fd;
    int flag;
    int max_pending;
    struct sockaddr_in server_addr;
    void* arg;
    gfh_error_t (*handler)(gfcontext_t **, const char *, void*);
};

struct gfcontext_t {
    int cs_fd;
    struct sockaddr_in client_addr;
	char request_header[256];
}

/* 
 * This function must be the first one called as part of 
 * setting up a server.  It returns a gfserver_t handle which should be
 * passed into all subsequent library calls of the form gfserver_*.  It
 * is not needed for the gfs_* call which are intended to be called from
 * the handler callback.
 */
gfserver_t* gfserver_create(){
    struct gfserver_t *gfs = (struct gfserver_t *) malloc(sizeof(struct gfserver_t));
    (*gfs).flag = 1;
    (*gfs).ss_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if ((*gfs).ss_fd < 0) {
        sprintf(stderr, "Failed to create socket");
        exit(1);
    }
    
    bzero(&((*gfs).server_addr), sizeof((*gfs).server_addr));
    (*gfs).server_addr.sin_family = AF_INET;
    (*gfs).server_addr.sin_addr.s_addr = INADDR_ANY;
    setsockopt((*gfs).ss_fd, SOL_SOCKET, SO_REUSEADDR, &((*gfs).flag), sizeof((*gfs).flag));
    
    return gfs;
}

/*
 * Sends size bytes starting at the pointer data to the client 
 * This function should only be called from within a callback registered 
 * with gfserver_set_handler.  It returns once the data has been
 * sent.
 */
ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
    return send((*(*ctx)).cs_fd, data, len, 0); 
}

/*
 * Sends to the client the Getfile header containing the appropriate 
 * status and file length for the given inputs.  This function should
 * only be called from within a callback registered gfserver_set_handler.
 */
ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
	char buffer[256];
	sprintf(buffer, "GETFILE %d %d \r\n\r\n", status, file_len);
	
	return send((*(*ctx)).cs_fd, buffer, 256, 0); 
}

/*
 * Starts the server.  Does not return.
 */
void gfserver_serve(gfserver_t **gfs){
    // Bind the server socket
    if (bind((*(*gfs)).ss_fd, (struct sockaddr *) &((*(*gfs)).server), sizeof((*(*gfs)).server)) < 0) {
		fprintf(stderr, "Failed to bind.");
        exit(1);
    }

    // Listen on the server socket for up to some maximum pending connections
    if (listen((*(*gfs)).ss_fd, (*(*gfs)).max_pending) < 0) {
		fprintf(stderr, "Failed to listen.");
        exit(1);
    } 

    while (1) { // Continuous listen and accept a new client
		struct gfcontext_t *gfc = (struct gfcontext_t *) malloc(sizeof(struct gfcontext_t));
		socklen_t client_addr_len = sizeof((*gfc).client_addr);
        (*gfc).cs_fd = accept((*(*gfs)).ss_fd, (struct sockaddr *) &((*gfc).client_addr), &client_addr_len);		

		if (0 > recv((*(*gfr)).cs_fd, (*gfc).request_header, 256, 0)) {
			fprintf(stderr, "Failed to receive request header");
			exit(1);
		}

		gfstatus_t status = GF_OK;
		size_t file_len = 1024;
		if (gfs_sendheader(&gfc, status, file_len) < 0) {
			fprintf(stderr, "Failed to send header.");
			exit(1);
		}
		
		if (status == GF_OK) {
			size_t message_len = 2048;
			if (gfs_send(&gfc, data, message_len) < 0) {
				fprintf(stderr, "Failed to send data");
				exit(1);
			}
		}
		
        close((*gfc).cs_fd);
		free(gfc);
    }
}

/*
 * Sets the third argument for calls to the handler callback.
 */
void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
    (*(gfs)).arg = arg;
}

/*
 * Sets the handler callback, a function that will be called for each each
 * request.  As arguments, this function receives:
 * - a gfcontext_t handle which it must pass into the gfs_* functions that 
 * 	 it calls as it handles the response.
 * - the requested path
 * - the pointer specified in the gfserver_set_handlerarg option.
 * The handler should only return a negative value to signal an error.
 */
void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
    (*(gfs)).handler = handler;
}

/*
 * Sets the maximum number of pending connections which the server
 * will tolerate before rejecting connection requests.
 */
void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
    (*(gfs)).max_pending = max_npending;
}

/*
 * Sets the port at which the server will listen for connections.
 */
void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    (*(*gfs)).server_addr.sin_port = htons(port);
}

/*
 * Aborts the connection to the client associated with the input
 * gfcontext_t.
 */
void gfs_abort(gfcontext_t **ctx){
    close((*(*ctx)).cs_fd);
	free(*ctx);
}
