// TCP: Extra parameter for socket type
int getaddr(const char *node, const char *service,
            struct addrinfo **address, int socktype)
{
    struct addrinfo hints = {
        .ai_flags = 0,
        .ai_family = AF_INET,
        .ai_socktype = socktype};

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

// TCP: Extra parameter for socket type
int mksocket(int socktype)
{
    // Create an IPv4 socket for use with UDP
    // Determine the protocol automatically
    int fileDescriptor = socket(AF_INET, socktype, 0);

    if (fileDescriptor == -1)
    {
        perror("Failed to get socket");
        exit(1);
    }

    return fileDescriptor;
}

void *lander(void *data)
{
    char *service = (char *)data;

    size_t msgsize = 1000;
    char msgbuf[msgsize];
    int l;
    struct addrinfo *landr;

    const char conditionq[] = "condition:?\n";
    const char stateq[] = "state:?\n";

    // TCP: Use SOCK_STREAM instead of SOCK_DGRAM
    if (!getaddr("127.0.1.1", service, &landr, SOCK_STREAM))
        fprintf(stderr, "can't get lander address");

    fprintf(stderr, "%s\n", landr->ai_canonname);

    // TCP: Use SOCK_STREAM instead of SOCK_DGRAM
    l = mksocket(SOCK_STREAM);

    // TCP: Need to connect before communicating
    if (connect(l, landr->ai_addr, landr->ai_addrlen) < 0)
    {
        perror("Failed to connect");
        exit(1);
    }

    while (true)
    {
        int m;
        usleep(50000); /* 20Hz = 0.05s = 50ms = 50000us */
        /* poll for condition */
        // TCP: send instead of sendto. Note how the address is not needed
        // As this is stored with the socket
        send(l, conditionq, strlen(conditionq), 0);

        // TCP: recv instead of recvfrom.
        m = recv(l, msgbuf, msgsize, 0);
        msgbuf[m] = '\0';

        /* parse condition */
        parsecondition(msgbuf);

        /* poll for state */
        // TCP: Same changes as poll for condition
        send(l, stateq, strlen(stateq), 0);

        m = recv(l, msgbuf, msgsize, 0);
        msgbuf[m] = '\0';
        parsestate(msgbuf);

        /* format command to send to lander */
        sem_wait(&cmdlock);
        sprintf(msgbuf,
                "command:!\n"
                "main-engine: %f\n"
                "rcs-roll: %f\n",
                landercommand.thrust, landercommand.rotn);
        sem_post(&cmdlock);

        // TCP: Same changes as poll for condition
        send(l, msgbuf, strlen(msgbuf), 0);
        m = recv(l, msgbuf, msgsize, 0);
        msgbuf[m] = '\0';

        usleep(100000);
    }
}

void *dashboard(void *data)
{
    size_t bufsize = 1024;
    char buffer[bufsize];
    int d;
    struct addrinfo *daddr;

    // TCP: Still use SOCK_DGRAM as this is a UDP socket
    if (!getaddr("127.0.1.1", (char *)data, &daddr, SOCK_DGRAM))
        fprintf(stderr, "cant get dash address");
    d = mksocket(SOCK_DGRAM);

    while (true)
    {
        // Format buffer for dashboard
        // Keep landercond locked for a minimal amount of time
        sem_wait(&condlock);
        sprintf(buffer,
                "fuel:%f\n"
                "altitude:%f\n",
                landercond.fuel,
                landercond.altitude);
        sem_post(&condlock);

        // Send formatted buffer using the previously acquired address
        sendto(d, buffer, strlen(buffer), 0, daddr->ai_addr, daddr->ai_addrlen);

        usleep(500000);
    }
}
