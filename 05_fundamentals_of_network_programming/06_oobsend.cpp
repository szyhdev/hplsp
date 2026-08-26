#include "common.h"

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
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

    int ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        printf("connect socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    const char *oob_data = "abc";
    const char *normal_data = "123";
    ret = send(sockfd, normal_data, strlen(normal_data), 0);
    printf("sent %d bytes of normal data '%s'\n", ret, normal_data);
    ret = send(sockfd, oob_data, strlen(oob_data), MSG_OOB);
    printf("sent %d bytes of oob data '%s'\n", ret, oob_data);
    ret = send(sockfd, normal_data, strlen(normal_data), 0);
    printf("sent %d bytes of normal data '%s'\n", ret, normal_data);

    close(sockfd);

    return 0;
}

// gcc -o 06_oobsend 06_oobsend.cpp
