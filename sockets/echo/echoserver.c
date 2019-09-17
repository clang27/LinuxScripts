#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>


#define BUFSIZE 16

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  echoserver [options]\n"                                                    \
"options:\n"                                                                  \
"  -p                  Port (Default: 19121)\n"                                \
"  -m                  Maximum pending connections (default: 1)\n"            \
"  -h                  Show this help message\n"                              \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"port",          required_argument,      NULL,           'p'},
  {"maxnpending",   required_argument,      NULL,           'm'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,             0}
};


int main(int argc, char **argv) {
    int option_char;
    int portno = 19121; /* port to listen on */
    int maxnpending = 1;
  
    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:m:hx", gLongOptions, NULL)) != -1) {
        switch (option_char) {
            case 'p': // listen-port
                portno = atoi(optarg);
                break;                                        
              default:
                fprintf(stderr, "%s ", USAGE);
                exit(1);
              case 'm': // server
                maxnpending = atoi(optarg);
                break; 
              case 'h': // help
                fprintf(stdout, "%s ", USAGE);
                exit(0);
                break;
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    if (maxnpending < 1) {
        fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__, maxnpending);
        exit(1);
    }
    
    /********************/
    /* Socket Code Here */
    /********************/
    
    int ss_fd, cs_fd; // Store the socket system call return values
    int flag = 1;
    char buffer[BUFSIZE];

    //Internet addresses for both socket nodes
    struct sockaddr_in server, client;
    socklen_t client_addr_len = sizeof(client); 

    if ((ss_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        exit(1);
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(portno);
    setsockopt(ss_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    
    // Bind the server socket
    if (bind(ss_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        exit(1);
    }

    // Listen on the server socket for up to some maximum pending connections
    if (listen(ss_fd, maxnpending) < 0) {
        exit(1);
    } 

    while (1) { // Continuous listen
        // Accept a new client
        cs_fd = accept(ss_fd, (struct sockaddr *) &client, &client_addr_len);
         
        // Read echo from the client
        bzero(buffer, BUFSIZE);
        if(0 > read(cs_fd, buffer, BUFSIZE)) {
            close(cs_fd);
            exit(1);
        } else {
            printf("%s", buffer);
        }
        // Echo back to the client
        write(cs_fd, buffer, strlen(buffer));
        
        close(cs_fd);
    }
    close(ss_fd);
    return 0;
}
