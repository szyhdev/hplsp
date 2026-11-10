#include "common.h"

int main(int argc, char *argv[])
{
    if (argc < 5) {
        printf("usage: %s ip_address port_number recvdata_size recvbuf_size\n",
                basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int recvdata_size = atoi(argv[3]);
    int recvbuf_size = atoi(argv[4]);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    int old_recvbuf_size = get_recvbuf_size(sockfd);
    if (old_recvbuf_size == -1) {
        close(sockfd);
        return -1;
    }

    int ret = set_recvbuf_size(sockfd, recvbuf_size);
    if (ret == -1) {
        close(sockfd);
        return -1;
    }

    int cur_recvbuf_size = get_recvbuf_size(sockfd);
    if (cur_recvbuf_size == -1) {
        close(sockfd);
        return -1;
    }
    printf("recv buffer size: %d, %d, %d\n", old_recvbuf_size,
            recvbuf_size, cur_recvbuf_size);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

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

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (connfd == -1) {
        printf("accept socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    char *buffer = new char[recvdata_size];
    while (1) {
        int ret = recv(connfd, buffer, recvdata_size - 1, 0);
        if (ret == -1) {
            printf("recv socket error: %d\n", errno);
            delete [] buffer;
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

    delete [] buffer;
    close(connfd);
    close(sockfd);

    return 0;
}

// g++ -o 11_set_recv_buffer 11_set_recv_buffer.cpp common.cpp
