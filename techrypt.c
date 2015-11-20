/*****************************************
 * Author: James Casey
 * Description: Assignment 2
 *
 * This file contains the main function
 * for the techcrypt program.  It encrypts
 * files in standalone mode and acts as 
 * a client to encrypt and send files
 * as well.
 *
 *****************************************/
#include "techrypt.h"

int main(int argc, char **argv)
{
    int c;
    int i;
    int pstop = 0;
    int *file_len;
    int *cfile_len;
    int skip;
    
    // set file lenths variables to make files
    // easier to work with
    file_len = (int *) malloc(sizeof(int));
    cfile_len = (int *) malloc(sizeof(int));

    // file name stuff
    char * fname;
    char * ofname;
    char * tfname;
    unsigned char * key;
    size_t keysize = 16;

    // server stuff
    int port;
    char * cport;
    char * server_addr;
    char * tmp_addr_info;
    unsigned char * file_buffer;
    unsigned char * cipher_text;

    /* Parse input options */
    if ((argc == 3) && (strcmp(argv[2],"-l")==0))
    {
        /* stand alone mode first */

        // Deal with file names
        c = strlen(argv[1]);
        fname = (char *) malloc(c*sizeof(char));
        strncpy(fname,argv[1],c);
        ofname = (char *) malloc(c+4);
        tfname = (char *) malloc(c+4);

        int len_fname = strlen(fname);
        for(i=0;i< len_fname;++i)
        {
            if(fname[i] == '.')
            {
                skip = len_fname - i;
                break;
            }
        }
        strncpy(tfname,fname,len_fname);
        tfname[len_fname-skip] = '\0';
        sprintf(ofname,"%s.gt",tfname);

        // Check for existing files
        if (access(ofname,F_OK)!=-1)
        {
            printf("Output File Exists!\n");
            return 33;
        }

        /* Do the crypto work here! */
        // Get the key
        key = (unsigned char *) load_crypto(keysize);
        
        // Encrypt the file
        encrypt_file(fname,ofname,key,keysize);
    }
    else if ((argc == 4) && (strcmp(argv[2],"-d")==0))
    {
        /* daemon mode */

        // File name stuff
        c = strlen(argv[1]);
        fname = (char *) malloc(c*sizeof(char));
        strncpy(fname,argv[1],c);

        c = strlen(argv[3]);
        tmp_addr_info = (char *) malloc(c*sizeof(char)+1);
        strncpy(tmp_addr_info,argv[3],c);

        // Separate ip address from port
        for (i=0;i<c;++i)
        {
            if (tmp_addr_info[i] == ':')
            {
                pstop = i;
                break;
            }
        }

        cport = (char *) malloc((c-pstop-1)*sizeof(char)+1);
        int j = 0;
        for (i=(pstop+1) ; i < c; ++i)
        {
            cport[j] = tmp_addr_info[i];
            ++j;
        }

        // port address
        cport[c-pstop] = '\0';
        port = atoi(cport);

        // ip address 
        server_addr = (char *) malloc(pstop*sizeof(char)+1);
        strncpy(server_addr,tmp_addr_info,pstop);
        server_addr[pstop] = '\0';


        // Get the key
        key = (unsigned char *) load_crypto(keysize);
        
        // load the file...
        file_buffer = load_file(fname,file_len);
        
        // encrypt the file... I am sure this can probably be done
        // better, but it has been a while since I did any coding in
        // c, so it is a little rough...
        cipher_text = encrypt_network_file(key,keysize,file_buffer,*file_len,cfile_len);

        // run the client
        start_client(server_addr,port,cipher_text,cfile_len);
    }
    else
    {
        printf("\nUsage: \n"
               "   techrypt <input file> [-d <IP-addr:port>] [-l]\n\n");

        return 1;
    }

    
    return 0;
}

unsigned char * load_file(char *fname, int *file_len)
{
    /* This jus loads the file for the daemon... If I had thought it through
     * better, I would have used it for stand alone mode as well, but that was
     * written first and I didn't feel like rewriting it after it was all said
     * and done.  It was just poor planning on my part.
     */

    long file_size;
    int l = 16;
    unsigned char * file_buffer;
    unsigned char in_buffer[l];
    int c;
    int i;


    FILE * fd = fopen(fname, "rb");
    fseek(fd, 0, SEEK_END);
    file_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    *file_len = file_size;

    file_buffer = (unsigned char *) malloc(file_size*sizeof(unsigned char));

    int pos = 0; 
    while (!feof(fd) && !ferror(fd))
    {
        c = fread(in_buffer,1,l,fd);
        if (c < l)
        {
            for (i=c;i<l;++i)
            {
                in_buffer[i] = 0;
            }
        }
        for(i=0;i<l;++i)
            file_buffer[pos*l + i] = in_buffer[i];
        pos += 1;
    }

    return file_buffer;
}
