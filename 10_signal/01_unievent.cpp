#include "common.h"

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 1024

static int pipefd[2];

void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    if (send(pipefd[1], (char *)&msg, 1, 0) == -1) {
        printf("send pipe error: %d\n", errno);
    }
    errno = save_errno;
}

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

    if (addfd(epollfd, sockfd) == -1) {
        printf("addfd sockfd error\n");
        close(epollfd);
        close(sockfd);
        return -1;
    }

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    if (ret == -1) {
        printf("socketpair error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    if (setnonblocking(pipefd[1]) == -1) {
        printf("setnonblocking pipefd[1] error\n");
        close(pipefd[1]);
        close(pipefd[0]);
        close(epollfd);
        close(sockfd);
        return -1;
    }

    if (addfd(epollfd, pipefd[0]) == -1) {
        printf("addfd pipefd[0] error\n");
        close(pipefd[1]);
        close(pipefd[0]);
        close(epollfd);
        close(sockfd);
        return -1;
    }

    if (addsig(SIGHUP, sig_handler) == -1 || addsig(SIGCHLD, sig_handler) == -1 ||
        addsig(SIGTERM, sig_handler) == -1 || addsig(SIGINT, sig_handler) == -1) {
        printf("addsig SIGHUP | SIGCHLD | SIGTERM | SIGINT error\n");
        close(pipefd[1]);
        close(pipefd[0]);
        close(epollfd);
        close(sockfd);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    int connfd = -1;
    bool stop_server = false;
    while (!stop_server) {
        int count = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (count == -1) {
            if (errno == EINTR) {
                printf("epollwait interrupted: %d\n", errno);
                continue;
            }
            printf("epollwait error: %d\n", errno);
            break;
        }
        printf("epollwait return %d events\n", count);

        for (int i = 0; i < count; i++) {
            int eventfd = events[i].data.fd;
            if (eventfd == sockfd) {
                struct sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
                if (connfd == -1) {
                    printf("accept socket error: %d\n", errno);
                    stop_server = true;
                    break;
                }
                if (addfd(epollfd, connfd) == -1) {
                    printf("addfd connfd error\n");
                    stop_server = true;
                    break;
                }
            } else if (events[i].events & EPOLLIN) {
                if (eventfd == pipefd[0]) {
                    int sig;
                    char signals[1024];
                    while (1) {
                        int ret = recv(pipefd[0], signals, sizeof(signals), 0);
                        if (ret == -1) {
                            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                                printf("recv pipefd[0] error: EAGAIN/EWOULDBLOCK\n");
                                break;
                            }
                            printf("recv pipefd[0] error: %d\n", errno);
                            stop_server = true;
                            break;
                        } else if (ret == 0) {
                            printf("pipefd[0] closed by peer\n");
                            stop_server = true;
                            break;
                        } else {
                            for (int i = 0; i < ret; ++i) {
                                printf("signal %d caught\n", signals[i]);
                                switch (signals[i]) {
                                    case SIGCHLD:
                                    case SIGHUP:
                                        continue;
                                    case SIGTERM:
                                    case SIGINT:
                                        stop_server = true;
                                }
                                if (stop_server) {
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    while (1) {
                        int ret = recv(eventfd, buffer, BUFFER_SIZE - 1, 0);
                        if (ret == -1) {
                            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                                printf("recv socket error: EAGAIN/EWOULDBLOCK\n");
                                break;
                            }
                            printf("recv socket error: %d\n", errno);
                            ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, eventfd, NULL);
                            if (ret == -1) {
                                printf("epoll_ctl DEL error: %d\n", errno);
                            }
                            close(eventfd);
                            if (eventfd == connfd) {
                                connfd = -1;
                            }
                            break;
                        } else if (ret == 0) {
                            printf("connection closed by client\n");
                            ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, eventfd, NULL);
                            if (ret == -1) {
                                printf("epoll_ctl DEL error: %d\n", errno);
                            }
                            close(eventfd);
                            if (eventfd == connfd) {
                                connfd = -1;
                            }
                            break;
                        } else {
                            ret = trim_crlf(buffer, ret);
                            printf("got %d bytes of normal data: %s\n", ret, buffer);
                        }
                    }
                }
            } else {
                printf("something else happened: %d\n", events[i].events);
                assert(0);
            }
        }
    }

    if (connfd != -1) {
        close(connfd);
    }
    close(sockfd);
    close(epollfd);
    close(pipefd[1]);
    close(pipefd[0]);

    return 0;
}

// gcc -o 01_unievent 01_unievent.cpp common.cpp
