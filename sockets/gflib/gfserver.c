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
	
	//Heading of response
	char scheme[8];
	gfstatus_t status;
	size_t filelen;
}

void gfs_abort(gfcontext_t **ctx){
    close((*(*ctx)).cs_fd);
	free(*ctx);
}

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

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
    return -1;
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
    return -1;
}

void gfserver_serve(gfserver_t **gfs){
    // Bind the server socket
    if (bind((*(*gfs)).ss_fd, (struct sockaddr *) &((*(*gfs)).server), sizeof((*(*gfs)).server)) < 0) {
		sprintf(stderr, "Failed to bind.");
        exit(1);
    }

    // Listen on the server socket for up to some maximum pending connections
    if (listen((*(*gfs)).ss_fd, (*(*gfs)).max_pending) < 0) {
		sprintf(stderr, "Failed to listen.");
        exit(1);
    } 

    while (1) { // Continuous listen and accept a new client
		struct gfcontext_t *gfc = (struct gfcontext_t *) malloc(sizeof(struct gfcontext_t));
		socklen_t client_addr_len = sizeof((*gfc).client_addr);
        (*gfc).cs_fd = accept((*(*gfs)).ss_fd, (struct sockaddr *) &((*gfc).client_addr), &client_addr_len);
		// Receive scheme, method, and path of request
		// Try to retrieve path content, and if success/failure update gfstatus_t.
		// Send header of response back to client.
		// If a successful gfstatus_t was in header, send path content.
		
        close((*gfc).cs_fd);
		free(gfc);
    }
}

void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
    (*(gfs)).arg = arg;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
    (*(gfs)).handler = handler;
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
    (*(gfs)).max_pending = max_npending;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    (*(*gfs)).server_addr.sin_port = htons(port);
}


