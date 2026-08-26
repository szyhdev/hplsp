#include "common.h"

void non_reentrant()
{
    struct in_addr addr1, addr2;
    addr1.s_addr = inet_addr("192.168.1.1");
    addr2.s_addr = inet_addr("10.0.0.1");

    char *str1 = inet_ntoa(addr1);
    char *str2 = inet_ntoa(addr2);

    printf("addr1: %s\n", str1);
    printf("addr2: %s\n", str2);
}

int main()
{
    non_reentrant();

    return 0;
}

// gcc -o 02_nonreentrant 02_nonreentrant.cpp
