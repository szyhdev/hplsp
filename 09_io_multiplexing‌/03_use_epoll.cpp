#include "common.h"

#define BUFFER_SIZE 1024
#define MAX_EVENT_NUMBER 1024

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

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    if (epollfd == -1) {
        printf("create epoll error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN;
    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);
    if (ret == -1) {
        printf("epoll_ctl ADD error: %d\n", errno);
        close(epollfd);
        close(sockfd);
        return -1;
    }

    std::vector<int> connfds;
    connfds.reserve(10);

    char buffer[BUFFER_SIZE];
    bool quit = false;
    while (1) {
        // epoll_wait
        ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (ret == -1) {
            int err = errno;
            if (err == EINTR) {
                printf("epoll_wait socket interrupted: %d\n", err);
                continue;
            } else {
                printf("epoll_wait socket error: %d\n", err);
                break;
            }
        }
        printf("waited: %d\n", ret);

        for (int i = 0; i < ret; i++) {
            int eventfd = events[i].data.fd;
            if (eventfd == sockfd) {
                // accept
                struct sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
                if (connfd == -1) {
                    int err = errno;
                    if (err == EINTR) {
                        printf("accept socket interrupted: %d\n", err);
                    } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                        printf("accept socket error: %d (EAGAIN/EWOULDBLOCK)\n", err);
                    } else if (err == ECONNABORTED) {
                        printf("accept socket error: %d (ECONNABORTED)\n", err);
                    } else {
                        printf("accept socket error: %d\n", err);
                        quit = true;
                        break;
                    }
                } else {
                    addfd(epollfd, connfd, &connfds, false, true, false);
                    char remote_addr[INET_ADDRSTRLEN];
                    printf("connected with ip: %s and port: %d (total = %ld)\n",
                            inet_ntop(AF_INET, &client_addr.sin_addr, remote_addr, INET_ADDRSTRLEN),
                            ntohs(client_addr.sin_port), connfds.size());
                }
            } else {
                // urgent data
                if (events[i].events & EPOLLPRI) {
                    int ret = recv(eventfd, buffer, sizeof(buffer) - 1, MSG_OOB);
                    if (ret == -1) {
                        int err = errno;
                        if (err == EINTR) {
                            printf("recv socket interrupted (OOB): %d\n", err);
                        } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                            printf("recv socket error (OOB): %d (EAGAIN/EWOULDBLOCK)\n", err);
                        } else {
                            printf("recv socket error (OOB): %d\n", err);
                            removefd(epollfd, eventfd, &connfds);
                            continue;
                        }
                    } else if (ret == 0) {
                        removefd(epollfd, eventfd, &connfds);
                        continue;
                    } else {
                        buffer[ret] = '\0';
                        printf("got %d bytes of OOB data: %s\n", ret, buffer);
                    }
                }

                // normal data
                if (events[i].events & EPOLLIN) {
                    int ret = recv(eventfd, buffer, sizeof(buffer) - 1, 0);
                    if (ret == -1) {
                        int err = errno;
                        if (err == EINTR) {
                            printf("recv socket interrupted (normal): %d\n", err);
                        } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                            // printf("recv socket error (normal): %d (EAGAIN/EWOULDBLOCK)\n", err);
                        } else {
                            printf("recv socket error (normal): %d\n", err);
                            removefd(epollfd, eventfd, &connfds);
                        }
                    } else if (ret == 0) {
                        removefd(epollfd, eventfd, &connfds);
                        printf("connection closed by client (total = %ld)\n", connfds.size());
                    } else {
                        buffer[ret] = '\0';
                        ret = trim_crlf(buffer, ret);
                        printf("got %d bytes of normal data: %s\n", ret, buffer);
                        quit = (strncmp(buffer, "quit", 4) == 0);
                        if (quit) {
                            removefd(epollfd, eventfd, &connfds);
                            break;
                        }
                    }
                }
            }
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
    close(epollfd);
    close(sockfd);

    return 0;
}

// g++ -o 03_use_epoll 03_use_epoll.cpp common.cpp -std=c++20
