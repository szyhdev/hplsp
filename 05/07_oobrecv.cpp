#include "common.h"

// #define USE_SIGURG

#define BUF_SIZE 1024

int connfd;

#if defined(USE_SIGURG)

void sigurg_handler(int sig)
{
    printf("SIGURG received\n");
    if (sockatmark(connfd) == 1) {
        char oob_buf[64];
        memset(oob_buf, '\0', sizeof(oob_buf));
        int ret = recv(connfd, &oob_buf, sizeof(oob_buf) - 1, MSG_OOB);
        if (ret > 0) {
            oob_buf[ret] = '\0';
            printf("got %d bytes of oob data '%s'\n", ret, oob_buf);
        }
    }
}

#endif

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
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
    socklen_t client_addrlength = sizeof(client_addr);
    connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlength);
    if (connfd == -1) {
        printf("accept socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

#if defined(USE_SIGURG)

    fcntl(connfd, F_SETOWN, getpid());
    signal(SIGURG, sigurg_handler);

#endif

    char buffer[BUF_SIZE];

    memset(buffer, '\0', BUF_SIZE);
    ret = recv(connfd, buffer, BUF_SIZE - 1, 0);
    printf("got %d bytes of normal data '%s'\n", ret, buffer);

#if !defined(USE_SIGURG)

    memset(buffer, '\0', BUF_SIZE);
    ret = recv(connfd, buffer, BUF_SIZE - 1, MSG_OOB);
    printf("got %d bytes of oob data '%s'\n", ret, buffer);

#endif

    memset(buffer, '\0', BUF_SIZE);
    ret = recv(connfd, buffer, BUF_SIZE - 1, 0);
    printf("got %d bytes of normal data '%s'\n", ret, buffer);

    close(connfd);
    close(sockfd);

    return 0;
}

// gcc -o 07_oobrecv 07_oobrecv.cpp
