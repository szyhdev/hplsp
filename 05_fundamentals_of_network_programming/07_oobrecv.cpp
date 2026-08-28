#include "common.h"

// #define USE_SIGURG

#define BUFFER_SIZE 1024

int connfd;

#if defined(USE_SIGURG)

void sigurg_handler(int sig)
{
    char oob_data;
    int ret = recv(connfd, &oob_data, sizeof(oob_data), MSG_OOB);
    if (ret == -1) {
        const char *msg = "recv socket error\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    } else if (ret == 0) {
        const char *msg = "connection closed by client\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    } else {
        const char *msg1 = "got 1 bytes of oob data: '";
        const char *msg2 = "'\n";
        write(STDOUT_FILENO, msg1, strlen(msg1));
        write(STDOUT_FILENO, &oob_data, sizeof(oob_data));
        write(STDOUT_FILENO, msg2, strlen(msg2));
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

#if defined(USE_SIGURG)
    signal(SIGURG, sigurg_handler);
#endif

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

#if defined(USE_SIGURG)
    fcntl(connfd, F_SETOWN, getpid());
#endif

    char buffer[BUFFER_SIZE];

    memset(buffer, '\0', BUFFER_SIZE);
    ret = recv(connfd, buffer, BUFFER_SIZE - 1, 0);
    printf("got %d bytes of normal data: '%s'\n", ret, buffer);

#if !defined(USE_SIGURG)
    memset(buffer, '\0', BUFFER_SIZE);
    ret = recv(connfd, buffer, BUFFER_SIZE - 1, MSG_OOB);
    printf("got %d bytes of oob data: '%s'\n", ret, buffer);
#endif

    memset(buffer, '\0', BUFFER_SIZE);
    ret = recv(connfd, buffer, BUFFER_SIZE - 1, 0);
    printf("got %d bytes of normal data: '%s'\n", ret, buffer);

    memset(buffer, '\0', BUFFER_SIZE);
    ret = recv(connfd, buffer, BUFFER_SIZE - 1, 0);
    printf("got %d bytes of normal data: '%s'\n", ret, buffer);

    close(connfd);
    close(sockfd);

    return 0;
}

// gcc -o 07_oobrecv 07_oobrecv.cpp
