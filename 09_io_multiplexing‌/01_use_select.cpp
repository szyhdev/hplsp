#include "common.h"

#define BUFFER_SIZE 1024

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

    std::vector<int> connfds;
    connfds.reserve(10);

    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    fd_set exception_fds;
    bool quit = false;
    while (1) {
        // prepare for select
        FD_ZERO(&read_fds);
        FD_ZERO(&exception_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(sockfd, &exception_fds);
        int maxfd = sockfd;
        for (auto connfd : connfds) {
            FD_SET(connfd, &read_fds);
            FD_SET(connfd, &exception_fds);
            if (connfd > sockfd) {
                maxfd = connfd;
            }
        }

        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        // select
        ret = select(maxfd + 1, &read_fds, NULL, &exception_fds, &timeout);
        if (ret == -1) {
            int err = errno;
            if (err == EINTR) {
                printf("select socket interrupted: %d\n", err);
                continue;
            } else {
                printf("select socket error: %d\n", err);
                break;
            }
        } else if (ret == 0) {
            printf("select timeout\n");
            continue;
        }
        printf("selected: %d\n", ret);

        if (FD_ISSET(sockfd, &read_fds)) {
            // accept
            struct sockaddr_in client_addr;
            socklen_t client_addrlen = sizeof(client_addr);
            int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
            if (connfd == -1) {
                int err = errno;
                if (errno == EINTR) {
                    printf("accept socket interrupted: %d\n", err);
                } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                    printf("accept socket error: %d (EAGAIN/EWOULDBLOCK)\n", err);
                } else if (err == ECONNABORTED) {
                    printf("accept socket error: %d (ECONNABORTED)\n", err);
                } else {
                    printf("accept socket error: %d\n", errno);
                    quit = true;
                    break;
                }
            } else {
                connfds.push_back(connfd);
                char remote_addr[INET_ADDRSTRLEN];
                printf("connected with ip: %s and port: %d (total = %ld)\n",
                    inet_ntop(AF_INET, &client_addr.sin_addr, remote_addr, INET_ADDRSTRLEN),
                    ntohs(client_addr.sin_port), connfds.size());
            }
        }

        for (auto itr = connfds.begin(); itr != connfds.end(); ) {
            int connfd = *itr;

            // urgent data
            if (FD_ISSET(connfd, &exception_fds)) {
                ret = recv(connfd, buffer, sizeof(buffer) - 1, MSG_OOB);
                if (ret == -1) {
                    int err = errno;
                    if (errno == EINTR) {
                        printf("recv socket interrupted (OOB): %d\n", err);
                    } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                        printf("recv socket error (OOB): %d (EAGAIN/EWOULDBLOCK)\n", err);
                    } else {
                        printf("recv socket error (OOB): %d\n", err);
                        close(connfd);
                        itr = connfds.erase(itr);
                        continue;
                    }
                } else if (ret == 0) {
                    close(connfd);
                    itr = connfds.erase(itr);
					continue;
                } else {
                    buffer[ret] = '\0';
                    printf("got %d bytes of OOB data: %s\n", ret, buffer);
                }
            }

            // normal data
            if (FD_ISSET(connfd, &read_fds)) {
                ret = recv(connfd, buffer, sizeof(buffer) - 1, 0);
                if (ret == -1) {
                    int err = errno;
                    if (errno == EINTR) {
                        printf("recv socket interrupted (normal): %d\n", err);
                    } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                        printf("recv socket error (normal): %d (EAGAIN/EWOULDBLOCK)\n", err);
                    } else {
                        printf("recv socket error (normal): %d\n", err);
                        close(connfd);
                        itr = connfds.erase(itr);
                        continue;
                    }
                } else if (ret == 0) {
                    close(connfd);
                    itr = connfds.erase(itr);
                    printf("connection closed by client (total = %ld)\n", connfds.size());
                    continue;
                } else {
                    buffer[ret] = '\0';
                    ret = trim_crlf(buffer, ret);
                    printf("got %d bytes of normal data: %s\n", ret, buffer);
                    quit = (strncmp(buffer, "quit", 4) == 0);
                    if (quit) {
                        close(connfd);
                        itr = connfds.erase(itr);
                        break;
                    }
                }
            }

            itr++;
        }

        if (quit) {
            printf("quit as requested by client (total = %ld)\n", connfds.size());
            break;
        }
    }

    printf("close %ld connfds...\n", connfds.size());
    for (auto connfd : connfds) {
        close(connfd);
    }
    close(sockfd);

    return 0;
}

// g++ -o 01_use_select 01_use_select.cpp common.cpp -std=c++20
