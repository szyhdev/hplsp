#include "common.h"

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
