#include "common.h"

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    if (setreuseaddr(sockfd) == -1) {
        close(sockfd);
        return -1;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
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

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (connfd == -1) {
        printf("accept socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    int oobinline = 1;
    ret = setsockopt(connfd, SOL_SOCKET, SO_OOBINLINE, &oobinline, sizeof(oobinline));
    if (ret == -1) {
        printf("setsocketopt error: %d\n", errno);
        close(connfd);
        close(sockfd);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        bool is_obbdata;
        int atmark = sockatmark(connfd);
        if (atmark == -1) {
            printf("sockatmark error: %d\n", errno);
            break;
        } else if (atmark == 1) {
            is_obbdata = true;
        } else {
            assert(atmark == 0);
            is_obbdata = false;
        }

        ret = recv(connfd, buffer, BUFFER_SIZE - 1, 0);
        if (ret == -1) {
            printf("recv socket error: %d\n", errno);
            close(connfd);
            close(sockfd);
            return -1;
        } else if (ret == 0) {
            printf("connection closed by client\n");
            break;
        }

        buffer[ret] = '\0';

        if (is_obbdata) {
            char oob_data = buffer[0];
            printf("got 1 byte of oob data: '%c'\n", oob_data);
            if (ret > 1) {
                printf("got %d byte of normal data: '%s'\n", ret - 1, buffer + 1);
            }
        } else {
            printf("got %d bytes of normal data: '%s'\n", ret, buffer);
        }
    }

    close(connfd);
    close(sockfd);

    return 0;
}

// gcc -o 07_oobrecv_atmark 07_oobrecv_atmark.cpp
