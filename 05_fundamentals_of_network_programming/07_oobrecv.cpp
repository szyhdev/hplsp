#include "common.h"

#define BUFFER_SIZE 1024

static volatile sig_atomic_t oob_pending = 0;

void sigurg_handler(int sig)
{
    oob_pending = 1;
}

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sigurg_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGURG, &sa, nullptr) == -1) {
        printf("sigaction error: %d\n", errno);
        return -1;
    }

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

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (connfd == -1) {
        printf("accept socket error: %d\n", errno);
        close(sockfd);
        return -1;
    }

    // Issue 1: Even if fcntl() is called immediately after accept(), the SIGURG
    //          signal may still be missed in some cases.
    //          A better approach is to use poll() to monitor for POLLPRI events.
    //          See 02_use_poll.cpp, 03_use_epoll.cpp and 03_sigurg.cpp
    if (fcntl(connfd, F_SETOWN, getpid()) == -1) {
        printf("fcntl socket F_SETOWN error: %d\n", errno);
        close(connfd);
        close(sockfd);
        return -1;
    }

    if (setnonblocking(connfd) == -1) {
        close(connfd);
        close(sockfd);
        return -1;
    }

    // Issue 2: The following code eliminates the race condition between
    //          checking and clearing the 'oob_pending' flag.
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGURG);

    char buffer[BUFFER_SIZE];
    while (1) {
        // Issue 2
        sigprocmask(SIG_BLOCK, &block_set, &old_set);
        int pending = oob_pending;
        oob_pending = 0;
        sigprocmask(SIG_SETMASK, &old_set, NULL);

        // urgent data
        if (pending) {  // Issue 2
            printf("oob_pending\n");

            ret = recv(connfd, buffer, sizeof(buffer) - 1, MSG_OOB);
            if (ret == -1) {
                int err = errno;
                if (errno == EINTR) {
                    printf("recv socket interrupted (OOB): %d\n", err);
                } else if (err == EAGAIN || err == EWOULDBLOCK) {
                    printf("recv socket error (OOB): %d (EAGAIN/EWOULDBLOCK)\n", err);
                } else {
                    printf("recv socket error (OOB): %d\n", err);
                    break;
                }
            } else {
                buffer[ret] = '\0';
                printf("got %d bytes of OOB data: '%s'\n", ret, buffer);
            }
        }

        // normal data
        ret = recv(connfd, buffer, sizeof(buffer) - 1, 0);
        if (ret == -1) {
            int err = errno;
            if (errno == EINTR) {
                printf("recv socket interrupted (normal): %d\n", err);
            } else if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                // printf("recv socket error (normal): %d (EAGAIN/EWOULDBLOCK)\n", err);
            } else {
                printf("recv socket error (normal): %d\n", err);
                break;
            }
        } else if (ret == 0) {
            printf("connection closed by client\n");
            break;
        } else {
            buffer[ret] = '\0';
            printf("got %d bytes of normal data: '%s'\n", ret, buffer);
        }

        // simulate polling via usleep
        usleep(10000);
    }

    close(connfd);
    close(sockfd);

    return 0;
}

// g++ -o 07_oobrecv 07_oobrecv.cpp common.cpp
