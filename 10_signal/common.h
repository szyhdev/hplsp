#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

int setnonblocking(int fd);
int setreuseaddr(int fd);
int addfd(int epollfd, int fd, bool enable_et = true, bool nonblocking = true);
int addsig(int sig, void (*sig_handler)(int), bool restart = true);
int trim_crlf(char *buf, int buf_len);
