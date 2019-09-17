
#include "gfclient-student.h"

struct gfcrequest_t {
	int cs_fd;
    struct sockaddr_in server_addr;
    void *headerarg;
    void (*headerfunc)(void*, size_t, void *);
    void *writearg;
    void (*writefunc)(void*, size_t, void *)
    
	// Heading of request
    char scheme[8];
    char method[4];
    char *path;
	
	// Received
	gfstatus_t status;
    size_t filelen;
    size_t bytesreceived;
};
    

void gfc_cleanup(gfcrequest_t **gfr){
	close((*(*gfr)).cs_fd);
    free(*gfr);
}

gfcrequest_t *gfc_create(){
    struct gfcrequest_t *gfr = (struct gfcrequest_t *) malloc(sizeof(struct gfcrequest_t));
    // Default values of request
    (*gfr).scheme = "GETFILE";
    (*gfr).method = "GET";
    
    return gfr;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr){
    // not yet implemented
    return -1;
}

size_t gfc_get_filelen(gfcrequest_t **gfr){
    // not yet implemented
    return -1;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr){
    // not yet implemented
    return -1;
}

void gfc_global_init(){
}

void gfc_global_cleanup(){
}

int gfc_perform(gfcrequest_t **gfr){
    // Connect socket to server
	if (0 > ((*(*gfr)).cs_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		sprintf(stderr, "Failed to create socket");
        return -1;
    }
    if (0 > connect((*(*gfr)).cs_fd, (struct sockaddr *) &((*(*gfr)).server_addr), sizeof((*(*gfr)).server_addr))) {
		sprintf(stderr, "Failed to connect socket");
        close(cs_fd);
        return -1;
    } 
	
    return -1;
}

void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg){
    (*(*gfr)).headerarg = headerarg;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void*, size_t, void *)){
    (*(*gfr)).headerfunc = headerfunc;
}

void gfc_set_path(gfcrequest_t **gfr, const char* path){
    (*(*gfr)).path = path;
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port){
    (*(*gfr)).server_addr.sin_port = htons(port);
}

void gfc_set_server(gfcrequest_t **gfr, const char* server){
    struct hostent *server_host_info = gethostbyname(server);
    unsigned long server_addr_nbo = *(unsigned long *)((*server_host_info).h_addr_list[0]);
    
    bzero(&((*(*gfr)).server_addr), sizeof((*(*gfr)).server_addr));
    (*(*gfr)).server_addr.sin_addr.s_addr = server_addr_nbo;
    (*(*gfr)).server_addr.sin_family = AF_INET;
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg){
    (*(*gfr)).writearg = writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *)){
    (*(*gfr)).writefunc = writefunc;
}

const char* gfc_strstatus(gfstatus_t status){
    const char *strstatus = NULL;

    switch (status) {
        default: {
            strstatus = "UNKNOWN";
        }
        break;

        case GF_INVALID: {
            strstatus = "INVALID";
        }
        break;

        case GF_FILE_NOT_FOUND: {
            strstatus = "FILE_NOT_FOUND";
        }
        break;

        case GF_ERROR: {
            strstatus = "ERROR";
        }
        break;

        case GF_OK: {
            strstatus = "OK";
        }
        break;
        
    }

    return strstatus;
}

