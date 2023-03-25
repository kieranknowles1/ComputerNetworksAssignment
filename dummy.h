#ifndef REMOVE_DUMMY_HEADER
// Dummy header to silence false positive errors in VS code
// All content is disabled with #ifndef and -D in Makefile

typedef unsigned int socklen_t;

struct addrinfo
{
    int ai_flags;             /* Input flags.  */
    int ai_family;            /* Protocol family for socket.  */
    int ai_socktype;          /* Socket type.  */
    int ai_protocol;          /* Protocol for socket.  */
    socklen_t ai_addrlen;     /* Length of socket address.  */
    struct sockaddr *ai_addr; /* Socket address for socket.  */
    char *ai_canonname;       /* Canonical name for service location.  */
    struct addrinfo *ai_next; /* Pointer to next in list.  */
};

#define AI_ADDRCONFIG 0x0020	/* Use configuration of this host to choose */
#define AI_PASSIVE 0x0001       /* Socket address is intended for `bind'.  */

extern int getaddrinfo(const char *__restrict __name,
                       const char *__restrict __service,
                       const struct addrinfo *__restrict __req,
                       struct addrinfo **__restrict __pai);

#endif
