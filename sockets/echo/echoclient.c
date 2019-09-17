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

/* A buffer large enough to contain the longest allowed string */
#define BUFSIZE 16

#define USAGE                                                                       \
    "usage:\n"                                                                      \
    "  echoclient [options]\n"                                                      \
    "options:\n"                                                                    \
    "  -s                  Server (Default: localhost)\n"                           \
    "  -p                  Port (Default: 19121)\n"                                  \
    "  -m                  Message to send to server (Default: \"Hello world.\")\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

/* Main ========================================================= */
int main(int argc, char **argv) {
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 19121;
    char *message = "Hello World!!";

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:m:hx", gLongOptions, NULL)) != -1) {
        switch (option_char) {
		case 's': // server
		    hostname = optarg;
		    break;
		case 'p': // listen-port
		    portno = atoi(optarg);
		    break;
		default:
		    fprintf(stderr, "%s", USAGE);
		    exit(1);
		case 'm': // message
		    message = optarg;
		    break;
		case 'h': // help
		    fprintf(stdout, "%s", USAGE);
		    exit(0);
		    break;
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == message) {
        fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == hostname) {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    /********************/
    /* Socket Code Here */
    /********************/
    
    int cs_fd; // Store the socket system call return values
    struct sockaddr_in server;
    char buffer[BUFSIZE];
    struct hostent *server_host_info = gethostbyname(hostname);
    unsigned long server_addr_nbo = *(unsigned long *)(server_host_info->h_addr_list[0]);

    if (0 > (cs_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        exit(1);
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(portno);
    server.sin_addr.s_addr = server_addr_nbo;

    // Connect socket to server
    if (0 > connect(cs_fd, (struct sockaddr *) &server, sizeof(server))) {
        close(cs_fd);
        exit(1);
    } 

    // Send echo message
    if (0 > send(cs_fd, message, strlen(message), 0)) {
        close(cs_fd);
        exit(1);
    }

    // Process response from server
    bzero(buffer, BUFSIZE);
    if(0 > read(cs_fd, buffer, BUFSIZE)) {
        close(cs_fd);
        exit(1);
    } else {
        printf("%s", buffer);
    }
    
    // Close the socket
    close(cs_fd);
    return 0;
}
