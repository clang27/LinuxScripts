
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
	
	char response_header[256];
	char response_body[2048];
};

/* 
 * This function must be the first one called as part of 
 * request.  It returns a gfcrequest_t handle which should be
 * passed into all subsequent library calls pertaining to
 * this requeest.
 */
gfcrequest_t *gfc_create(){
    struct gfcrequest_t *gfr = (struct gfcrequest_t *) malloc(sizeof(struct gfcrequest_t));
    // Default values of request
    (*gfr).scheme = "GETFILE";
    (*gfr).method = "GET";
    
    return gfr;
}

/*
 * Returns the status of the response.
 */
gfstatus_t gfc_get_status(gfcrequest_t **gfr){	
	char str_status[4];
	int int_status;
	gfstatus_t status;
	
	int i, j;
	unsigned short gap_counter = 0;
	int space_flag = 0;
	char *h = &((*(*gfr)).response_header);
	
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
	
	//Convert buffer to size_t
	sprintf(int_status, "%d", str_status);
	
	switch (int_status) {
        default: {
            status = GF_ERROR;
        }
        break;
        case 600: {
            status = GF_INVALID;
        }
        break;
        case 400: {
            status = GF_FILE_NOT_FOUND;
        }
        break;
        case 500: {
            status = GF_ERROR;
        }
        break;
        case 200: {
            status = GF_OK;
        }
        break;     
    }
	
    return status;
}

/*
 * Returns the length of the file as indicated by the response header.
 * Value is not specified if the response status is not OK.
 */
size_t gfc_get_filelen(gfcrequest_t **gfr){
	char str_filelen[32];
	size_t filelen;
	
	int i, j;
	unsigned short gap_counter = 0;
	int space_flag = 0;
	char *h = &((*(*gfr)).response_header);
	
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
	sprintf(filelen, "%d", str_filelen);
    return filelen;
}

/*
 * Returns actual number of bytes received before the connection is closed.
 * This may be distinct from the result of gfc_get_filelen when the response 
 * status is OK but the connection is reset before the transfer is completed.
 */
size_t gfc_get_bytesreceived(gfcrequest_t **gfr){
    return recv((*(*gfr)).cs_fd, (*(*gfr)).response_body, 2048, 0);
}

/*
 * Performs the transfer as described in the options.  Returns a value of 0
 * if the communication is successful, including the case where the server
 * returns a response with a FILE_NOT_FOUND or ERROR response.  If the 
 * communication is not successful (e.g. the connection is closed before
 * transfer is complete or an invalid header is returned), then a negative 
 * integer will be returned.
 */
int gfc_perform(gfcrequest_t **gfr){
    // Connect socket to server
	if (0 > ((*(*gfr)).cs_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		fprintf(stderr, "Failed to create socket");
        return -1;
    }
    if (0 > connect((*(*gfr)).cs_fd, (struct sockaddr *) &((*(*gfr)).server_addr), sizeof((*(*gfr)).server_addr))) {
		fprintf(stderr, "Failed to connect socket");
        close(cs_fd);
        return -1;
    } 
	//Send request
	char request[256] = "";
	strcat(request, (*(*gfr)).scheme);
	strcat(request, " ");
	strcat(request, (*(*gfr)).method);
	strcat(request, " ");
	strcat(request, (*(*gfr)).path);
	strcat(request, " \r\n\r\n");
	send((*(*gfr)).cs_fd, request, 256, 0);
	
	//Receive header response
	recv((*(*gfr)).cs_fd, (*(*gfr)).response_header, 256, 0);
	
	//Receive body response and callback
	if (gfc_get_status(&gfr) == GF_OK) {
		size_t len = gfc_get_filelen(&gfr);
		size_t amount_received = 0;
		size_t total_amount_received = 0;
		while (total_amount_received < len) {
			amount_received = gfc_get_bytesreceived(&gfr);
			total_amount_received += amount_received;
			(*(*(*gfr)).writefunc)((*(*gfr)).response_body, amount_received, (*(*gfr)).writearg);
		}
		return 0;
	}
	else if (gfc_get_status(&gfr) == GF_FILE_NOT_FOUND || gfc_get_status(&gfr) == GF_ERROR) {
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

/*
 * Sets the callback for received header.  The registered callback
 * will receive a pointer the header of the response, the length 
 * of the header response as it's second argument (don't assume that
 * this is null-terminated), and the pointer registered with 
 * gfc_set_headerarg (or NULL if not specified) as the third argument.
 *
 * You may assume that the callback will only be called once and will
 * contain the full header.
 */
void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void*, size_t, void *)){
    (*(*gfr)).headerfunc = headerfunc;
}

/*
 * Sets the path of the file that will be requested.
 */
void gfc_set_path(gfcrequest_t **gfr, const char* path){
    (*(*gfr)).path = path;
}

/*
 * Sets the port over which the request will be made.
 */
void gfc_set_port(gfcrequest_t **gfr, unsigned short port){
    (*(*gfr)).server_addr.sin_port = htons(port);
}

/*
 * Sets the server to which the request will be sent.
 */
void gfc_set_server(gfcrequest_t **gfr, const char* server){
    struct hostent *server_host_info = gethostbyname(server);
    unsigned long server_addr_nbo = *(unsigned long *)((*server_host_info).h_addr_list[0]);
    
    bzero(&((*(*gfr)).server_addr), sizeof((*(*gfr)).server_addr));
    (*(*gfr)).server_addr.sin_addr.s_addr = server_addr_nbo;
    (*(*gfr)).server_addr.sin_family = AF_INET;
}

/*
 * Sets the third argument for all calls to the registered write callback.
 */
void gfc_set_writearg(gfcrequest_t **gfr, void *writearg){
    (*(*gfr)).writearg = writearg;
}

/*
 * Sets the callback for received chunks of the body.  The registered 
 * callback will receive a pointer the chunk, the length of the chunk
 * as it's second argument (don't assume that this is null-terminated),
 * and the pointer registered with gfc_set_writearg (or NULL if not 
 * specified) as the third argument.
 *
 * The callback may be called multiple times in a single request.  The 
 * gfclient library does not store the entire contents of the requested file
 * but rather calls this callback each time that it receives a chunk of data
 * from the server.
 */
void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *)){
    (*(*gfr)).writefunc = writefunc;
}

/*
 * Returns the string associated with the input status
 */
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

/*
 * Frees memory associated with the request.  
 */
void gfc_cleanup(gfcrequest_t **gfr){
	close((*(*gfr)).cs_fd);
    free(*gfr);
}
/*
 * Sets up any global data structures needed for the library.
 * Warning: this function may not be thread-safe.
 */
void gfc_global_init(){
}
/*
 * Cleans up any global data structures needed for the library.
 * Warning: this function may not be thread-safe.
 */
void gfc_global_cleanup(){
}
