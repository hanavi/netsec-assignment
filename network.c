/*****************************************
 * Author: James Casey
 * Description: Assignment 2
 *
 * This file contains the networking 
 * functions.
 *****************************************/
#include "network.h"

int start_client(char * remote_addr, int port, char *cipher_text, int *ct_len)
{
    /* This is the client function.  It connects and 
     * sends the encrypted file 
     */ 
    
    int n = 0;
    int i;
    int j;
    int count;
    int bcount;
    int pos = 0;

    // I probably have way too many buffers laying around... This
    // could probably be cleand up a ton.
    char buffer[RECVLEN+1];

    // Networking stuff
    int sockfd = 0;
    struct sockaddr_in server; 

    memset(buffer, 0, sizeof(buffer));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError Creating Socket: %s\n", strerror(errno));
        return 1;
    } 

    // More networking...
    memset(&server, 0, sizeof(server)); 
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(remote_addr);
    server.sin_port = htons(port); 

    // Connect to the mothership! 
    if( connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
       printf("\nFailed to Connect: %s\n", strerror(errno));
       return 1;
    } 


    // How much data are we processing?
    count = *ct_len / RECVLEN + 1;
    
    // I did this all wrong the first time... for some reason it didn't hit me
    // that you can't use strncpy for non strings.  I had to go back and
    // rewrite it all to make it work with the binary information from the
    // cipher text.
    for (i=0; i < count; ++i)
    {
        memset(buffer,0,RECVLEN);
        if (i < count-1)
        {
            bcount = RECVLEN;
            for(j=0;j<bcount;++j)
            {
                buffer[j] = cipher_text[RECVLEN*i+j];
            }
        }
        else
        {       
            bcount = (*ct_len) % RECVLEN;
            for(j=0;j < bcount;++j)
            {
                buffer[j] = cipher_text[RECVLEN*i+j];
            }
        }
        pos += 1;
        // Click here to send!
        write(sockfd, buffer, bcount); 
    }
    

    return 0;
}

int start_server(int port,int *csize)
{
    /* This is the server side */
    int i,c;
    int err;

    int socketfd;
    int clientfd;

    unsigned char buffer[RECVLEN];
    struct sockaddr_in server_addr; 
    unsigned char * out_buffer;
    unsigned char * tbuffer;

    // Configure a bunch-o-network stuffs
    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    memset(buffer, 0, sizeof(buffer)); 

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port); 

    err = bind(socketfd, (struct sockaddr*)&server_addr, 
            sizeof(server_addr)); 
    listen(socketfd, 1); 

    if (err)
    {
        printf("Failed to bind to port\n");
        return 1;
    }

    int first = TRUE;
    int tot = 0;
    out_buffer = NULL;
    int count = 0;
    int step = 0;
    while(TRUE)
    {
        // Anyone want to play with me?
        printf("Waiting for connections.\n");
        
        // Knock knock...
        clientfd = accept(socketfd, (struct sockaddr*)NULL, NULL); 
        while ((c = read(clientfd, buffer, sizeof(buffer))) > 0)
        {
            tot += c;
            // resize the buffer to handle increasing data....  I don't like
            // this, but I couldn't find a better way.  I miss python :-(
            out_buffer = (unsigned char *) realloc(out_buffer,tot*sizeof(unsigned char));
            for (i=0; i<c; ++i)
            {
                step = tot-c + i;
                out_buffer[step]=buffer[i];
            }
        } 

        *csize = tot;
        // Done!
        close(clientfd);
        return out_buffer;
     }
}


