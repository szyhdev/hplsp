#include "common.h"

#define BUFFER_SIZE 512

int main(int argc, char *argv[])
{
    if (argc <= 3) {
        printf("usage: %s ip_address port_number send_buffer_size\n",
                basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    int sendbuf = 0;
    int sendbuf_len = sizeof(sendbuf);
    int ret = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf,
            (socklen_t *)&sendbuf_len);
    if (ret == -1) {
        printf("getsockopt error: %d\n", errno);
        close(sockfd);
        return -1;
    }
    printf("send buffer size (old): %d\n", sendbuf);

    sendbuf = atoi(argv[3]);
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf,
            sizeof(sendbuf));
    if (ret == -1) {
        printf("setsockopt error: %d\n", errno);
        close(sockfd);
        return -1;
    }
    printf("send buffer size (set): %d\n", sendbuf);

    sendbuf_len = sizeof(sendbuf);
    ret = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf,
            (socklen_t *)&sendbuf_len);
    if (ret == -1) {
        printf("getsockopt error: %d\n", errno);
        close(sockfd);
        return -1;
    }
    printf("send buffer size (new): %d\n", sendbuf);

    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        printf("connect socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 'a', BUFFER_SIZE);
    ret = send(sockfd, buffer, BUFFER_SIZE, 0);
    if (ret == -1) {
        printf("send socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }
    printf("sent %d bytes of data: '%s'\n", ret, buffer);

    close(sockfd);

    return 0;
}

// gcc -o 10_set_send_buffer 10_set_send_buffer.cpp
