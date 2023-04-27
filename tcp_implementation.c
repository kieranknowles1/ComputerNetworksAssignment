// TODO: Rewrite for TCP
void *lander(void *data)
{
    char *service = (char *)data;

    size_t msgsize = 1000;
    char msgbuf[msgsize];
    int l;
    struct addrinfo *landr;

    const char conditionq[] = "condition:?\n";
    const char stateq[] = "state:?\n";

    if (!getaddr("127.0.1.1", service, &landr))
        fprintf(stderr, "can't get lander address");
    ;
    fprintf(stderr, "%s\n", landr->ai_canonname);

    l = mksocket();

    while (true)
    {
        int m;
        usleep(50000); /* 20Hz = 0.05s = 50ms = 50000us */
        /* poll for condition */
        sendto(l, conditionq, strlen(conditionq), 0, landr->ai_addr,
               landr->ai_addrlen);

        m = recvfrom(l, msgbuf, msgsize, 0, NULL, NULL);
        msgbuf[m] = '\0';

        /* parse condition */
        parsecondition(msgbuf);

        /* poll for state */
        sendto(l, stateq, strlen(stateq), 0, landr->ai_addr,
               landr->ai_addrlen);

        m = recvfrom(l, msgbuf, msgsize, 0, NULL, NULL);
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

        sendto(l, msgbuf, strlen(msgbuf), 0, landr->ai_addr, landr->ai_addrlen);
        m = recvfrom(l, msgbuf, msgsize, 0, NULL, NULL);
        msgbuf[m] = '\0';

        usleep(100000);
    }
}
