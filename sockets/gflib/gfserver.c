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
    gfstatus_t status;
    char scheme[8];
    char method[4];
    size_t filelen;
}

void gfs_abort(gfcontext_t **ctx){
    close((*(*ctx)).cs_fd);
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
    // To-do: Change this to gfcontext stuff
    // Bind the server socket
    if (bind(ss_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        exit(1);
    }

    // Listen on the server socket for up to some maximum pending connections
    if (listen(ss_fd, 1) < 0) {
        exit(1);
    } 

    while (1) { // Continuous listen
        // Accept a new client
        cs_fd = accept(ss_fd, (struct sockaddr *) &client, &client_addr_len);
         
        // Sending file data
        fp = fopen(filename, "r");
        while((data_amount_sent = fread(buffer, 1, BUFSIZE, fp)) > 0) {
            send(cs_fd, buffer, data_amount_sent, 0);
        }

        fclose(fp);
        close(cs_fd);
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


