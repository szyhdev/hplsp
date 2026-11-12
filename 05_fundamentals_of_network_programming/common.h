#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int setreuseaddr(int fd);
int setnonblocking(int fd);
int get_sendbuf_size(int fd);
int set_sendbuf_size(int fd, int sendbuf_size);
int get_recvbuf_size(int fd);
int set_recvbuf_size(int fd, int recvbuf_size);
int trim_crlf(char *buf, int buf_len);
