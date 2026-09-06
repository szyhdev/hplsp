#include "common.h"

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    if (old_option == -1) {
        printf("fcntl error: %d\n", errno);
        return -1;
    }

    int new_option = old_option | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, new_option) == -1)
    {
        printf("fcntl error: %d\n", errno);
        return -1;
    }

    return old_option;
}

int setreuseaddr(int fd)
{
    int reuse = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret == -1) {
        printf("setsocketopt error: %d\n", errno);
        return -1;
    }
    return 0;
}

int addfd(int epollfd, int fd, bool enable_et, bool nonblocking)
{
    if (nonblocking) {
        if (setnonblocking(fd) == -1) {
            return -1;
        }
    }

    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_et) {
        event.events |= EPOLLET;
    }
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) == -1) {
        printf("epoll_ctl ADD error: %d\n", errno);
        return -1;
    }

    return 0;
}

int addsig(int sig, void (*sig_handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    if (sigfillset(&sa.sa_mask) == -1) {
        printf("sigfillset error: %d\n", errno);
        return -1;
    }

    if (sigaction(sig, &sa, NULL) == -1) {
        printf("sigaction error: %d\n", errno);
        return -1;
    }

    return 0;
}

int trim_right_crlf(char *buf, int buf_len)
{
    if (buf) {
        while (buf_len > 0) {
            char last = buf[buf_len - 1];
            if (last == '\n' || last == '\r') {
                buf[buf_len - 1] = '\0';
                buf_len--;
            } else {
                break;
            }
        }
        return buf_len;
    }

    return 0;
}

int trim_left_crlf(char *buf, int buf_len)
{
    if (buf) {
        while (buf_len > 0) {
            char first = buf[0];
            if (first == '\n' || first == '\r') {
                buf_len--;
                memmove(buf, buf + 1, buf_len);
            } else {
                break;
            }
        }
        return buf_len;
    }

    return 0;
}

int trim_crlf(char *buf, int buf_len)
{
    buf_len = trim_right_crlf(buf, buf_len);
    buf_len = trim_left_crlf(buf, buf_len);
    return buf_len;
}
