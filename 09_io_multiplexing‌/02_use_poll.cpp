#include "common.h"

#define BUFFER_SIZE 1024
#define MAX_CLIENT_NUMBER 1024

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
    struct pollfd fds[MAX_CLIENT_NUMBER];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    int nfds = 1;
    bool quit = false;
    while (1) {
        // prepare for poll
        int timeout = 5000;

        // poll
        ret = poll(fds, nfds, timeout);
        if (ret == -1) {
            int err = errno;
            if (err == EINTR) {
                printf("poll socket interrupted: %d\n", err);
                continue;
            } else {
                printf("poll socket error: %d\n", err);
                break;
            }
        } else if (ret == 0) {
            printf("poll timeout\n");
            continue;
        }
        printf("polled: %d\n", ret);

        if (fds[0].revents & POLLIN) {
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
                connfds.push_back(connfd);
                fds[nfds].fd = connfd;
                fds[nfds].events = POLLIN | POLLPRI;
                fds[nfds].revents = 0;
                nfds++;
                char remote_addr[INET_ADDRSTRLEN];
                printf("connected with ip: %s and port: %d (total = %ld)\n",
                    inet_ntop(AF_INET, &client_addr.sin_addr, remote_addr, INET_ADDRSTRLEN),
                    ntohs(client_addr.sin_port), connfds.size());
            }
        }

        for (int i = nfds - 1; i > 0; --i) {
            int connfd = fds[i].fd;

            // urgent data
            if (fds[i].revents & POLLPRI) {
                ret = recv(connfd, buffer, sizeof(buffer) - 1, MSG_OOB);
                if (ret == -1) {
                    int err = errno;
                    if (err == EINTR) {
                        printf("recv socket interrupted (OOB): %d\n", err);
                    } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                        printf("recv socket error (OOB): %d (EAGAIN/EWOULDBLOCK)\n", err);
                    } else {
                        printf("recv socket error (OOB): %d\n", err);
                        close(connfd);
                        std::erase(connfds, connfd);
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        continue;
                    }
                } else if (ret == 0) {
                    close(connfd);
                    std::erase(connfds, connfd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
					continue;
                } else {
                    buffer[ret] = '\0';
                    printf("got %d bytes of OOB data: %s\n", ret, buffer);
                }
            }

            // normal data
            if (fds[i].revents & POLLIN) {
                ret = recv(connfd, buffer, sizeof(buffer) - 1, 0);
                if (ret == -1) {
                    int err = errno;
                    if (err == EINTR) {
                        printf("recv socket interrupted (normal): %d\n", err);
                    } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                        printf("recv socket error (normal): %d (EAGAIN/EWOULDBLOCK)\n", err);
                    } else {
                        printf("recv socket error (normal): %d\n", err);
                        close(connfd);
                        std::erase(connfds, connfd);
                        fds[i] = fds[nfds - 1];
                        nfds--;
                    }
                } else if (ret == 0) {
                    close(connfd);
                    std::erase(connfds, connfd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    printf("connection closed by client (total = %ld)\n", connfds.size());
                } else {
                    buffer[ret] = '\0';
                    ret = trim_crlf(buffer, ret);
                    printf("got %d bytes of normal data: %s\n", ret, buffer);
                    quit = (strncmp(buffer, "quit", 4) == 0);
                    if (quit) {
                        close(connfd);
                        std::erase(connfds, connfd);
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        break;
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
    close(sockfd);

    return 0;
}

// g++ -o 02_use_poll 02_use_poll.cpp common.cpp -std=c++20
