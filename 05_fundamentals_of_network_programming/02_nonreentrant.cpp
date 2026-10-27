#include "common.h"

static char ip_addr1[] = "192.168.1.1";
static char ip_addr2[] = "10.0.0.1";

void non_reentrant()
{
    struct in_addr addr1, addr2;
    addr1.s_addr = inet_addr(ip_addr1);
    addr2.s_addr = inet_addr(ip_addr2);
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
    struct in_addr addr1;
    if (inet_pton(AF_INET, ip_addr1, &addr1) != 1) {
        printf("inet_pton error: %d\n", errno);
        return;
    }
    printf("integer addr1: %d\n", addr1.s_addr);

    struct in_addr addr2;
    if (inet_pton(AF_INET, ip_addr2, &addr2) != 1) {
        printf("inet_pton error: %d\n", errno);
        return;
    }
    printf("integer addr2: %d\n", addr2.s_addr);

    char buf1[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr1, buf1, sizeof(buf1)) == NULL) {
        printf("inet_ntop error: %d\n", errno);
        return;
    }
    printf("string addr1: %s\n", buf1);

    char buf2[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr2, buf2, sizeof(buf2)) == NULL) {
        printf("inet_ntop error: %d\n", errno);
        return;
    }
    printf("string addr2: %s\n", buf2);
}

int main()
{
    non_reentrant();
    reentrant();

    return 0;
}

// g++ -o 02_nonreentrant 02_nonreentrant.cpp
