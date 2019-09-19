
#include "gfclient-student.h"

struct gfcrequest_t {
	int cs_fd;
    const char *server;
	unsigned short port;
	
    void *headerarg;
    void (*headerfunc)(void*, size_t, void *);
    void *writearg;
    void (*writefunc)(void*, size_t, void *);
    
	// Heading of request
    char *scheme;
    char *method;
    const char *path;
	
	char response[RESPONSE_BUFSIZE];
};

gfcrequest_t *gfc_create(){
    struct gfcrequest_t *gfr = (struct gfcrequest_t *) malloc(sizeof(struct gfcrequest_t));
    // Default values of request
    (*gfr).scheme = "GETFILE";
    (*gfr).method = "GET";
    
    return gfr;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr){	
	char str_status[32];
	gfstatus_t status;
	
	int i;
	int j = 0;
	unsigned short gap_counter = 0;
	int space_flag = 0;
	char *h = (*(*gfr)).response;
	
	for (i=0; i<strlen(h); i++) {
		if (h[i] == ' ') {
			//Saw a space, continue
			space_flag = 1;
		}
		else {
			//Saw a character after seeing a space
			if (space_flag){
				//One more gap to track
				gap_counter++;
				space_flag = 0;
				//After one gap, we are at the status portion of header so mark char number of header
				if (gap_counter == 1){
					j = i;
				}
			}
			
			//From that char point, copy over to the beginning elements of the buffer
			if (gap_counter == 1) {
				str_status[i-j] = h[i];
				//Peek at next char, and if it is a space or end of header, then the status string is complete
				if (h[i+1] == '\r' || h[i+1] == ' ') {
					str_status[i-j+1] = '\0';
				}
			}
		}
	}
        
	if (strcmp(str_status, "OK") == 0) {
    	status = GF_OK;
	}
	else if (strcmp(str_status, "INVALID") == 0) {
    	status = GF_INVALID;
	}
	else if (strcmp(str_status, "FILE_NOT_FOUND") == 0) {
    	status = GF_FILE_NOT_FOUND;
	}
	else if (strcmp(str_status, "ERROR") == 0) {
    	status = GF_ERROR;
	}

    return status;
}

size_t gfc_get_filelen(gfcrequest_t **gfr){
	char str_filelen[32];
	
	int i;
	int j = 0;
	unsigned short gap_counter = 0;
	int space_flag = 0;
	char *h = (*(*gfr)).response;
	
	for (i=0; i<strlen(h); i++) {
		if (h[i] == ' ') {
			//Saw a space, continue
			space_flag = 1;
			continue;
		} 
		else {
			if (space_flag){
				//Didn't see a space, so that was one more gap to track
				gap_counter++;
				space_flag = 0;
				//After two gaps, we are at the file length portion of header so mark char number of header
				if (gap_counter == 2){
					j = i;
				}
			}
			//From that char point, copy over to the beginning elements of the buffer
			if (gap_counter == 2) {
				str_filelen[i-j] = h[i];
				//Peek at next char, and if it is a space or end of header, then the file length string is complete
				if (h[i+1] == '\r' || h[i+1] == ' ') {
					str_filelen[i-j+1] = '\0';
				}
			}
		}
	}
	
	//Convert buffer to size_t
	size_t filelen = atoi(str_filelen);
    return filelen;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr){
	int i;
	char *h = (*(*gfr)).response;
	size_t bytes_received = 0;
	unsigned short found_end_of_header = 0;
	
	for (i=0; i<strlen(h); i++) {
		if (h[i] == '\r' && h[i++] == '\n' && h[i++] == '\r' && h[i++] == '\n') {
			found_end_of_header = 1;
		} 
		else if (found_end_of_header && h[i] != 0) {
			bytes_received++;
		}
	}
	
    return bytes_received;
}

int gfc_perform(gfcrequest_t **gfr){
	struct sockaddr_in server_addr;
	struct hostent *server_host_info = gethostbyname((*(*gfr)).server);
    unsigned long server_addr_nbo = *(unsigned long *)((*server_host_info).h_addr_list[0]);
    
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_addr.s_addr = server_addr_nbo;
    server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((*(*gfr)).port);
	
    // Connect socket to server
	if (0 > ((*(*gfr)).cs_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		fprintf(stderr, "Failed to create socket\n");
		fflush(stderr);
        return -1;
    }
    if (0 > connect((*(*gfr)).cs_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) {
		fprintf(stderr, "Failed to connect socket\n");
		fflush(stderr);
        close((*(*gfr)).cs_fd);
        return -1;
    } 
	
	//Send request
	char request[MAX_REQUEST_LEN] = "";
	strcat(request, (*(*gfr)).scheme); strcat(request, " ");	
	strcat(request, (*(*gfr)).method); strcat(request, " ");
	strcat(request, (*(*gfr)).path); strcat(request, " \r\n\r\n");
	
	send((*(*gfr)).cs_fd, request, MAX_REQUEST_LEN, 0);
	//Receive header response
	bzero(&((*(*gfr)).response), RESPONSE_BUFSIZE);	
	recv((*(*gfr)).cs_fd, (*(*gfr)).response, MAX_REQUEST_LEN, 0);	
	
	//Receive body response and callback
	if (gfc_get_status(gfr) == GF_OK) {
		size_t len = gfc_get_filelen(gfr);
		size_t amount_received = 0;	
		size_t bytes_received = 0;
		while ((bytes_received = gfc_get_bytesreceived(gfr)) < len) {
    		amount_received = recv((*(*gfr)).cs_fd, (*(*gfr)).response, RESPONSE_BUFSIZE, 0);
			fprintf(stdout, "Received %ld bytes \n", bytes_received);
			fflush(stdout);
			(*(*(*gfr)).writefunc)((*(*gfr)).response, amount_received, (*(*gfr)).writearg);
		}
		return 0;
	}
	else if (gfc_get_status(gfr) == GF_FILE_NOT_FOUND || gfc_get_status(gfr) == GF_ERROR) {
		return 0;
	}
    
    return -1;
}

/*
 * Sets the third argument for all calls to the registered header callback.
 */
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
    (*(*gfr)).port = port;
}

void gfc_set_server(gfcrequest_t **gfr, const char* server){
	(*(*gfr)).server = server;
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

void gfc_cleanup(gfcrequest_t **gfr){
	close((*(*gfr)).cs_fd);
    free(*gfr);
}

void gfc_global_init(){
}

void gfc_global_cleanup(){
}
