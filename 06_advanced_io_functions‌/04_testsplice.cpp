#include "common.h"

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

    int pipefd[2];
    ret = pipe(pipefd);
    if (ret == -1) {
        printf("create pipe error: %d\n", errno);
        close(connfd);
        close(sockfd);
        return -1;
    }

    ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    if (ret == -1) {
        printf("splice error: %d\n", errno);
        close(connfd);
        close(sockfd);
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    printf("splice read %d bytes from connfd to pipefd[1]\n", ret);

    ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    if (ret == -1) {
        printf("splice error: %d\n", errno);
        close(connfd);
        close(sockfd);
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    printf("splice read %d bytes from pipefd[0] to connfd\n", ret);

    close(connfd);
    close(sockfd);

    return 0;
}

// gcc -o 04_testsplice 04_testsplice.cpp common.cpp
