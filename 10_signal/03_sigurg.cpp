#include "common.h"

#define BUF_SIZE 1024

static int connfd;

void sig_urg_handler(int sig)
{
    int save_errno = errno;

    char buffer[BUF_SIZE];
    memset(buffer, '\0', BUF_SIZE);
    int ret = recv(connfd, buffer, BUF_SIZE - 1, MSG_OOB);
    if (ret == -1) {
        printf("recv socket error: %d\n", errno);
    } else if (ret == 0) {
        printf("connection closed by client\n");
    } else {
        printf("got %d bytes of oob data '%s'\n", ret, buffer);
    }

    errno = save_errno;
}

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
    connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (connfd == -1) {
        printf("accept socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    if (addsig(SIGURG, sig_urg_handler) == -1) {
        printf("addsig SIGURG error\n");
        close(sockfd);
        return -1;
    }

    if (fcntl(connfd, F_SETOWN, getpid()) == -1) {
        printf("fcntl F_SETOWN error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    char buffer[BUF_SIZE];
    while (1) {
        ret = recv(connfd, buffer, BUF_SIZE - 1, 0);
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
            printf("got %d bytes of normal data: '%s'\n", ret, buffer);
        }
    }

    close(connfd);
    close(sockfd);

    return 0;
}

// gcc -o 03_sigurg 03_sigurg.cpp common.cpp
