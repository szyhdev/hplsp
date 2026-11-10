#include "common.h"

int main(int argc, char *argv[])
{
    if (argc < 5) {
        printf("usage: %s ip_address port_number senddata_size sendbuf_size\n",
                basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int senddata_size = atoi(argv[3]);
    int sendbuf_size = atoi(argv[4]);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    int old_sendbuf_size = get_sendbuf_size(sockfd);
    if (old_sendbuf_size == -1) {
        close(sockfd);
        return -1;
    }

    int ret = set_sendbuf_size(sockfd, sendbuf_size);
    if (ret == -1) {
        close(sockfd);
        return -1;
    }

    int cur_sendbuf_size = get_sendbuf_size(sockfd);
    if (cur_sendbuf_size == -1) {
        close(sockfd);
        return -1;
    }
    printf("send buffer size: %d, %d, %d\n", old_sendbuf_size,
            sendbuf_size, cur_sendbuf_size);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        printf("connect socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    char *buffer = new char[senddata_size];
    memset(buffer, 'a', senddata_size);
    ret = send(sockfd, buffer, senddata_size, 0);
    if (ret == -1) {
        printf("send socket error: %d\n", errno);
        delete [] buffer;
        close(sockfd);
        return -1;
    }
    printf("sent %d bytes of data: '%s'\n", ret, buffer);

    delete [] buffer;
    close(sockfd);

    return 0;
}

// g++ -o 10_set_send_buffer 10_set_send_buffer.cpp common.cpp
