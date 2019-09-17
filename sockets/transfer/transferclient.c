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

#define BUFSIZE 4098

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferclient [options]\n"                           \
    "options:\n"                                             \
    "  -s                  Server (Default: localhost)\n"    \
    "  -p                  Port (Default: 19121)\n"           \
    "  -o                  Output file (Default cs6200.txt)\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    FILE *fp;
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 19121;
    char *filename = "cs6200.txt";

    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:o:hx", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'o': // filename
            filename = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /********************/
    /* Socket Code Here */
    /********************/
    
    int cs_fd; // Store the socket system call return values
    ssize_t data_amount_received;
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
    
    // Create file to write to
    fp = fopen(filename, "w");
    char mode[] = "0644";
    int permissions = strtol(mode, 0, 8);
    chmod(filename, permissions);

    // Process file from server
    while ((0 < (data_amount_received = recv(cs_fd, buffer, BUFSIZE, 0)))){
        fwrite(buffer, sizeof(char), data_amount_received, fp);
    }
    
    // Close the socket and file
    fclose(fp);
    close(cs_fd);
    return 0;
}
