#include "common.h"

int main()
{
    openlog("01_syslog", LOG_PID | LOG_CONS, LOG_USER);
    printf("syslog initialized: PID = %d\n", getpid());

    setlogmask(LOG_UPTO(LOG_DEBUG));

    syslog(LOG_DEBUG, "This is a debug message that should be filtered out");
    syslog(LOG_INFO, "Application started successfully");
    syslog(LOG_WARNING, "Disk space is running low on /dev/sda1");
    errno = ENOENT;
    syslog(LOG_ERR, "Failed to open config file: %m");
    syslog(LOG_CRIT, "Critical database connection failure");

    closelog();
    printf("syslog closed\n");

    return 0;
}

// gcc -o 01_syslog 01_syslog.cpp
