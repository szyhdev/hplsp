#include "common.h"

int main(int argc, char* argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    int ret = bind(sockfd, (struct sockaddr *)&address, sizeof(address));
    if (ret == -1) {
        printf("bind socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    ret = listen(sockfd, 5);
    if (ret == -1) {
        printf("listen socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    printf("sleep for 20s\n");
    sleep(20);
    printf("wake up\n");

    struct sockaddr_in client;
    socklen_t client_addrlen = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr *)&client, &client_addrlen);
    if (connfd < 0) {
        printf("accept error: %d\n", errno);
    } else {
        char remote[INET_ADDRSTRLEN];
        printf("connected with ip: %s and port: %d\n",
                inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN),
                ntohs(client.sin_port));
        close(connfd);
    }

    close(sockfd);

    return 0;
}

// gcc -o 05_testaccept 05_testaccept.cpp
