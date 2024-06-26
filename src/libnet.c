/* Network and UDP handling library
 * KV5002
 *
 * Dr Alun Moon
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Dummy header to silence false positive errors in VS code
// All content is disabled with #ifndef and -D in Makefile
#include "dummy.h"

int getaddr(const char *node, const char *service,
            struct addrinfo **address)
{
    struct addrinfo hints = {
        .ai_flags = 0,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM
    };

    if (node)
        hints.ai_flags = AI_ADDRCONFIG;
    else
        hints.ai_flags = AI_PASSIVE;
    hints.ai_flags |= AI_CANONNAME;

    // Fill address with the found server address
    int status = getaddrinfo(node, service, &hints, address);

    // Log to stderr as stdout is used by the console. Can be redirected with 2>
    fprintf(stderr, "Connected with status %d\n", status);

    return status == 0;
}

int mksocket(void)
{
    // Create an IPv4 socket for use with UDP
    // Determine the protocol automatically
    int fileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);

    if (fileDescriptor == -1) {
        perror("Failed to get socket");
        exit(1);
    }

    return fileDescriptor;
}

int bindsocket(int sfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int err = bind(sfd, addr, addrlen);
    if (err == -1) {
        fprintf(stderr, "error binding socket: %s\n", strerror(errno));
        return false;
    }
    return true;
}

char uri[80];
char *addrtouri(struct sockaddr *addr)
{
    struct sockaddr_in *a = (struct sockaddr_in *) addr;
    sprintf(uri, "%s:%d", inet_ntoa(a->sin_addr), ntohs(a->sin_port));
    return uri;
}

typedef size_t (*handler_t)(char *, size_t,
                            char *, size_t, struct sockaddr_in *);

int server(int srvrsock, handler_t handlemsg)
{
    const size_t buffsize = 4096;       /* 4k */
    char message[buffsize], reply[buffsize];
    size_t msgsize, replysize;
    struct sockaddr clientaddr;
    socklen_t addrlen = sizeof(clientaddr);

    while (true) {
        msgsize = recvfrom(srvrsock,    /* server socket listening on */
                           message,     /* buffer to put message */
                           buffsize,    /* size of receiving buffer */
                           0,   /* flags */
                           &clientaddr, /* fill in with address of client */
                           &addrlen     /* number of bytes filled in */
            );
        replysize = handlemsg(message,  /* incoming message */
                              msgsize,  /* incoming message size */
                              reply,    /* buffer for reply */
                              buffsize, /* size of outgoing buffer */
                              (struct sockaddr_in *) &clientaddr);
        if (replysize)
            sendto(srvrsock,    /* server socket to use */
                   reply,       /* outgoing message to send */
                   replysize,   /* size of message */
                   0,           /* flags */
                   &clientaddr, /* address to send to */
                   addrlen      /* size of address structure */
                );
    }

}

int cleanupsock;

void finished(int signal)
{
    exit(0);
}

void cleanup(void)
{
    close(cleanupsock);
}
