#include <stdio.h>
#include <gcrypt.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0


unsigned char * load_crypto(size_t keysize);

int encrypt_file(char *fname, char *ofname, unsigned char *key, 
        size_t keysize);

int encrypt_network_file(unsigned char *key, size_t keysize, 
        unsigned char * file_buffer, int file_len, int *cbuffer_len);

int decrypt_file(char *fname, char *ofname, unsigned char *key, 
        size_t keysize);
