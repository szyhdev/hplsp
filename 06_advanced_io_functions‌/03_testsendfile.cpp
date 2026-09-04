#include "common.h"

int main(int argc, char *argv[])
{
    if (argc <= 3) {
        printf("usage: %s ip_address port_number filename\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *file_name = argv[3];

    int filefd = open(file_name, O_RDONLY);
    if (filefd == -1) {
        printf("open file error: %d\n", errno);
        return -1;
    }

    struct stat stat_buf;
    int ret = fstat(filefd, &stat_buf);
    if (ret == -1) {
        printf("fstat file error: %d\n", errno);
        close(filefd);
        return -1;
    }

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    if (setreuseaddr(sockfd) == -1) {
        close(sockfd);
        close(filefd);
        return -1;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        printf("bind socket error: %d\n", errno);
        close(sockfd);
        close(filefd);
        return -1;
    }

    ret = listen(sockfd, 5);
    if (ret == -1) {
        printf("listen socket error: %d\n", errno);
        close(sockfd);
        close(filefd);
        return -1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (connfd == -1) {
        printf("accept socket error: %d\n", errno);
        close(sockfd);
        close(filefd);
        return -1;
    }

    ret = sendfile(connfd, filefd, NULL, stat_buf.st_size);
    if (ret == -1) {
        printf("sendfile error: %d\n", errno);
        close(connfd);
        close(sockfd);
        close(filefd);
        return -1;
    }
    printf("%s sent (size = %ld)\n", file_name, stat_buf.st_size);

    close(connfd);
    close(sockfd);
    close(filefd);

    return 0;
}

// gcc -o 03_testsendfile 03_testsendfile.cpp common.cpp
