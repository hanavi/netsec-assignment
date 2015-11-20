#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h> 
#include <errno.h>

#define RECVLEN 1024
#define TRUE 1
#define FALSE 0


int start_client(char * remote_addr, int port, char *cipher_text, 
        int *ct_len);

int start_server(int port,int *csize);

