#include "gfserver-student.h"

struct gfserver_t {
    int ss_fd;
	unsigned short port;
    int max_pending;
    
    void* arg;
    gfh_error_t (*handler)(gfcontext_t **, const char *, void*);
};

struct gfcontext_t {
    int cs_fd;
	gfstatus_t status;
	char request[MAX_REQUEST_LEN];
};

gfserver_t *gfserver_create(){    
    return (struct gfserver_t *) malloc(sizeof(struct gfserver_t));
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
    fprintf(stdout, "Sending %ld amount of data.\n", len);
    return send((*(*ctx)).cs_fd, data, len, 0); 
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
	char buffer[MAX_REQUEST_LEN];
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
	
	sprintf(buffer, "GETFILE %s %ld\r\n\r\n", strstatus, file_len);
	fprintf(stdout, "Sending header: GETFILE %s %ld\\r\\n\\r\\n\n", strstatus, file_len);
	fflush(stdout);
	
	return send((*(*ctx)).cs_fd, buffer, MAX_REQUEST_LEN, 0); 
}

void gfserver_serve(gfserver_t **gfs){
	struct sockaddr_in server_addr;
	int flag = 1;
	(*(*gfs)).ss_fd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons((*(*gfs)).port);
    setsockopt((*(*gfs)).ss_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	
    // Bind the server socket
    if (bind((*(*gfs)).ss_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		fprintf(stderr, "Failed to bind.\n");
        exit(1);
    }

    // Listen on the server socket for up to some maximum pending connections
    if (listen((*(*gfs)).ss_fd, (*(*gfs)).max_pending) < 0) {
		fprintf(stderr, "Failed to listen.\n");
        exit(1);
    } 

	struct gfcontext_t *gfc = (struct gfcontext_t *) malloc(sizeof(struct gfcontext_t));
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
    char scheme[8];
	char method[4];
    char path[128];
    int i, space_flag, j, gap_counter;
	    
    while (1) { // Continuous listen and accept a new client
		// Reinitialize variables each iteration of loop
		(*gfc).cs_fd = 0;
		bzero(&client_addr, sizeof(client_addr));
		bzero((*gfc).request, MAX_REQUEST_LEN);
		bzero(scheme, 8); bzero(method, 4); bzero(path, 128);
		space_flag = 0; j=0; gap_counter=0; 
		
        (*gfc).cs_fd = accept((*(*gfs)).ss_fd, (struct sockaddr *) &client_addr, &client_addr_len);     		

		if (0 > recv((*gfc).cs_fd, (*gfc).request, MAX_REQUEST_LEN, 0)) {
			fprintf(stderr, "Failed to receive request header.\n");
			exit(1);
		}
		
	    //Same thing as in gfclient, but for all three words in header
        for (i=0; i<strlen((*gfc).request); i++) {
		    if ((*gfc).request[i] == ' ') {
			    space_flag = 1;
			    continue;
		    } 
		    else {
			    if (space_flag){
				    gap_counter++;
				    space_flag = 0;
				    if (gap_counter == 0 || gap_counter == 1 || gap_counter == 2){
					    j = i;
				    }
			    }
			    if (gap_counter == 0) {
				    scheme[i-j] = (*gfc).request[i];
				    if ((*gfc).request[i+1] == '\r' || (*gfc).request[i+1] == ' ') {
					    scheme[i-j+1] = '\0';
				    }
			    }
			    else if (gap_counter == 1) {
				    method[i-j] = (*gfc).request[i];
				    if ((*gfc).request[i+1] == '\r' || (*gfc).request[i+1] == ' ') {
					    method[i-j+1] = '\0';
				    }
			    }
			    else if (gap_counter == 2) {
				    path[i-j] = (*gfc).request[i];
				    if ((*gfc).request[i+1] == '\r' || (*gfc).request[i+1] == ' ') {
					    path[i-j+1] = '\0';
				    }
			    }
		    }
		}
		
		printf("%s", (*gfc).request);
		for (i=0; i<strlen(path); i++) {
		    if (path[i] == ' ' || path[i] == '\\' || path[i] == '!' || path[i] == '\n' || path[i] == '\r') {
                (*gfc).status = GF_INVALID;
		    }
		}
        if (strcmp(scheme, "GETFILE") == 0 && strcmp(method, "GET") == 0 && strlen(path) > 0) {
            (*(*gfs)).handler(&gfc, path, (*(*gfs)).arg);
            
        } 
        else if (!(strcmp(scheme, "GETFILE") == 0 && strcmp(method, "GET") == 0)) {
            (*gfc).status = GF_INVALID;
        }
        else if ((*gfc).status != GF_INVALID) {
            (*gfc).status = GF_FILE_NOT_FOUND;
        }
        
		gfs_sendheader(&gfc, (*gfc).status, 0);
        close((*gfc).cs_fd);
    }
	free(gfc);
    close((*(*gfs)).ss_fd);
}

void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
    (*(*gfs)).arg = arg;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
    (*(*gfs)).handler = handler;
}


void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
    (*(*gfs)).max_pending = max_npending;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    (*(*gfs)).port = port;
}

void gfs_abort(gfcontext_t **ctx){
    fprintf(stdout, "Aborting!");
    fflush(stdout);
    gfs_sendheader(ctx, GF_ERROR, 0);
    close((*(*ctx)).cs_fd);
	free(*ctx);
}
