#include "common.h"

int setreuseaddr(int fd)
{
    int reuse = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret == -1) {
        printf("setsocketopt SO_REUSEADDR error: %d\n", errno);
        return -1;
    }
    return 0;
}

int setnonblocking(int fd)
{
    int old_flags = fcntl(fd, F_GETFL);
    if (old_flags == -1) {
        printf("fcntl F_GETFL error: %d\n", errno);
        return -1;
    }

    int new_flags = old_flags | O_NONBLOCK;
    int ret = fcntl(fd, F_SETFL, new_flags);
    if (ret == -1) {
        printf("fcntl F_SETFL error: %d\n", errno);
        return -1;
    }

    return old_flags;
}

int get_buf_size(int fd, int buf_name)
{
    int sendbuf = 0;
    int sendbuf_len = sizeof(sendbuf);
    int ret = getsockopt(fd, SOL_SOCKET, buf_name, &sendbuf,
            (socklen_t *)&sendbuf_len);
    if (ret == -1) {
        printf("getsockopt error: %d\n", errno);
        return -1;
    }

    return sendbuf;
}

int get_sendbuf_size(int fd)
{
    return get_buf_size(fd, SO_SNDBUF);
}

int get_recvbuf_size(int fd)
{
    return get_buf_size(fd, SO_RCVBUF);
}

int set_buf_size(int fd, int buf_name, int buf_size)
{
    int ret = setsockopt(fd, SOL_SOCKET, buf_name, &buf_size,
            sizeof(buf_size));
    if (ret == -1) {
        printf("setsockopt error: %d\n", errno);
        return -1;
    }

    return 0;
}

int set_sendbuf_size(int fd, int sendbuf_size)
{
    return set_buf_size(fd, SO_SNDBUF, sendbuf_size);
}

int set_recvbuf_size(int fd, int recvbuf_size)
{
    return set_buf_size(fd, SO_RCVBUF, recvbuf_size);
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
