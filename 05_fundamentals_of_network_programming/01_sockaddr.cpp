#include "common.h"

const char *get_family(struct sockaddr_storage *storage)
{
    switch (storage->ss_family) {
        case AF_INET:
            return "IPv4";
        case AF_INET6:
            return "IPv6";
        case AF_UNIX:
            return "Unix Domain";
        default:
            return "unknown";
    }
}

int bind_sockaddr(struct sockaddr_storage *storage,
        struct sockaddr *addr, socklen_t addr_len)
{
    memcpy(storage, addr, addr_len);

    int sockfd = socket(storage->ss_family, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("create socket error: %d\n", errno);
        return -1;
    }

    struct sockaddr *generic_addr = reinterpret_cast<struct sockaddr *>(storage);
    int ret = bind(sockfd, generic_addr, addr_len);
    if (ret == -1) {
        printf("bind socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main()
{
    // generic address: sockaddr_storage
    struct sockaddr_storage storage;

    // IPv4 address: sockaddr_in
    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr_in.sin_addr);

    int sockfd = bind_sockaddr(&storage, (struct sockaddr *)&addr_in, sizeof(addr_in));
    if (sockfd != -1) {
        printf("%s: successfully bind addr_in\n", get_family(&storage));
        close(sockfd);
    }

    // IPv6 address: sockaddr_in6
    struct sockaddr_in6 addr_in6;
    memset(&addr_in6, 0, sizeof(addr_in6));
    addr_in6.sin6_family = AF_INET6;
    addr_in6.sin6_port = htons(9090);
    inet_pton(AF_INET6, "::1", &addr_in6.sin6_addr);

    sockfd = bind_sockaddr(&storage, (struct sockaddr *)&addr_in6, sizeof(addr_in6));
    if (sockfd != -1) {
        printf("%s: successfully bind addr_in6\n", get_family(&storage));
        close(sockfd);
    }

    // Unix domain address: sockaddr_un
    struct sockaddr_un addr_un;
    memset(&addr_un, 0, sizeof(addr_un));
    addr_un.sun_family = AF_UNIX;
    strncpy(addr_un.sun_path, "/tmp/01_sockaddr.sock", sizeof(addr_un.sun_path) - 1);

    sockfd = bind_sockaddr(&storage, (struct sockaddr *)&addr_un, sizeof(addr_un));
    if (sockfd != -1) {
        printf("%s: successfully bind addr_un\n", get_family(&storage));
        close(sockfd);
        unlink("/tmp/01_sockaddr.sock");
    }

    return 0;
}
