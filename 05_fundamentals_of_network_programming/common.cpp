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
