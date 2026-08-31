#include "common.h"

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc <= 3) {
        printf("usage: %s ip_address port_number recv_buffer_size\n",
                basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    int recvbuf = 0;
    int recvbuf_len = sizeof(recvbuf);
    int ret = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf,
            (socklen_t *)&recvbuf_len);
    if (ret == -1) {
        printf("getsockopt error: %d\n", errno);
        close(sockfd);
        return -1;
    }
    printf("recv buffer size (old): %d\n", recvbuf);

    recvbuf = atoi(argv[3]);
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf,
            sizeof(recvbuf));
    if (ret == -1) {
        printf("setsockopt error: %d\n", errno);
        close(sockfd);
        return -1;
    }
    printf("recv buffer size (set): %d\n", recvbuf);

    recvbuf_len = sizeof(recvbuf);
    ret = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf,
            (socklen_t *)&recvbuf_len);
    if (ret == -1) {
        printf("getsockopt error: %d\n", errno);
        close(sockfd);
        return -1;
    }
    printf("recv buffer size (new): %d\n", recvbuf);

    ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
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

    struct sockaddr_in client;
    socklen_t client_addr = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr *)&client, &client_addr);
    if (connfd == -1) {
        printf("accept socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        int ret = recv(connfd, buffer, BUFFER_SIZE - 1, 0);
        if (ret == -1) {
            printf("recv socket error: %d\n", errno);
            close(connfd);
            close(sockfd);
            return -1;
        } else if (ret == 0) {
            printf("connection closed by client\n");
            break;
        } else {
            buffer[ret] = '\0';
            printf("got %d bytes of data: '%s'\n", ret, buffer);
        }
    }

    close(connfd);
    close(sockfd);

    return 0;
}

// gcc -o 11_set_recv_buffer 11_set_recv_buffer.cpp
