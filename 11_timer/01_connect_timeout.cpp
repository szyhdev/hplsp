#include "common.h"

int timeout_connect(const char *ip, int port, int time)
{
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

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags & O_NONBLOCK) {
        printf("socket in non-blocking mode\n");
    } else {
        printf("socket in blocking mode\n");
    }

    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    if (ret == -1) {
        printf("set socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        if (errno == ETIMEDOUT) {
            printf("connection timeout\n");
        } else if (errno == EINPROGRESS) {
            printf("connection in progress (function call timeout)\n");
        } else if (errno == ECONNREFUSED) {
            printf("connection refused\n");
        } else if (errno == EINTR) {
            printf("connection interrupted\n");
        } else {
            printf("connect socket error: %d\n", errno);
        }
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return -1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = timeout_connect(ip, port, 10);
    if (sockfd == -1) {
        return -1;
    }

    close(sockfd);

    return 0;
}

// gcc 01_connect_timeout.cpp -o 01_connect_timeout
