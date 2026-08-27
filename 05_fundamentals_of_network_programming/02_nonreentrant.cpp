#include "common.h"

#define INET_ADDRSTRLEN 16

void non_reentrant()
{
    struct in_addr addr1, addr2;
    addr1.s_addr = inet_addr("192.168.1.1");
    addr2.s_addr = inet_addr("10.0.0.1");
    printf("integer addr1: %d\n", addr1.s_addr);
    printf("integer addr2: %d\n", addr2.s_addr);

    char *str1 = inet_ntoa(addr1);
    char *str2 = inet_ntoa(addr2);
    printf("string addr1: %s\n", str1);
    printf("string addr2: %s\n", str2);
    printf("\n");
}

void reentrant()
{
    struct in_addr addr;
    if (inet_pton(AF_INET, "192.168.1.1", &addr) == 1) {
        printf("integer addr: %d\n", addr.s_addr);
    } else {
        printf("inet_pton error: %d\n", errno);
    }

    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, buf, sizeof(buf)) != NULL) {
        printf("string addr: %s\n", buf);
    } else {
        printf("inet_ntop error: %d\n", errno);
    }
}

int main()
{
    non_reentrant();
    reentrant();

    return 0;
}

// gcc -o 02_nonreentrant 02_nonreentrant.cpp
