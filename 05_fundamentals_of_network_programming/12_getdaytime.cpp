#include "common.h"

// #define USE_GETADDRINFO

#define BUFFER_SIZE 256

int main(int argc, char *argv[])
{
    if (argc <= 1) {
    printf("usage: %s host_name\n", basename(argv[0]));
    return 1;
    }
    const char *host = argv[1];

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;

#if !defined(USE_GETADDRINFO)
    struct hostent *hostinfo = gethostbyname(host);
    if (hostinfo == NULL) {
        printf("gethostbyname error for host \"%s\": %d\n", host, h_errno);
        return -1;
    }

    struct servent *servinfo = getservbyname("daytime", "tcp");
    if (servinfo == NULL) {
        printf("getservbyname error for service \"daytime\": %d\n", h_errno);
        return -1;
    }
    printf("daytime port is %d\n", ntohs(servinfo->s_port));

    addr.sin_addr = *((struct in_addr *)*hostinfo->h_addr_list);
    addr.sin_port = servinfo->s_port;
#else
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int errcode = getaddrinfo(host, "daytime", &hints, &res);
    if (errcode != 0) {
        printf("getaddrinfo error for host \"%s\": %d\n", host, errcode);
        return -1;
    }

    printf("daytime port is %d\n", ntohs(((struct sockaddr_in *)res->ai_addr)->sin_port));

    memcpy(&addr, (struct sockaddr_in *)res->ai_addr, sizeof(addr));
    addr.sin_port = ((struct sockaddr_in *)res->ai_addr)->sin_port;
    freeaddrinfo(res);
#endif

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    int ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        printf("connect socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    ret = read(sockfd, buffer, sizeof(buffer));
    if (ret == -1) {
        printf("read socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    buffer[ret] = '\0';
    trim_crlf(buffer, ret);
    printf("%s\n", buffer);

    close(sockfd);

    return 0;
}

// gcc -o 12_getdaytime 12_getdaytime.cpp common.cpp
// $ ./12_getdaytime time.nist.gov
