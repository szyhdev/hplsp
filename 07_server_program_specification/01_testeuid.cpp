#include "common.h"

int main()
{
    uid_t uid = getuid();
    uid_t euid = geteuid();
    printf( "userid is %d\n", uid);
    printf( "effective userid is: %d\n", euid);

    return 0;
}

// gcc -o 01_testeuid 01_testeuid.cpp
