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
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#define BUFSIZE 4098

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferserver [options]\n"                           \
    "options:\n"                                             \
    "  -f                  Filename (Default: 6200.txt)\n" \
    "  -h                  Show this help message\n"         \
    "  -p                  Port (Default: 19121)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}};

int main(int argc, char **argv)
{
    int option_char;
    int portno = 19121;             /* port to listen on */
    char *filename = "6200.txt"; /* file to transfer */

    setbuf(stdout, NULL); // disable buffering

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:hf:x", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        case 'f': // file to transfer
            filename = optarg;
            break;
        }
    }


    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    
    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    /********************/
    /* Socket Code Here */
    /********************/
    
    int ss_fd, cs_fd; // Store the socket system call return values

    FILE *fp;
    ssize_t data_amount_sent;
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
    close(ss_fd);
    return 0;
}
