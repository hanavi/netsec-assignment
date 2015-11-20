/*****************************************
 * Author: James Casey
 * Description: Assignment 2
 *
 * This file contains the main function
 * for the techdec program.  It decrypts
 * files in standalone mode and acts as 
 * a daemon to recieve and decrypt files
 * as well.
 *
 *****************************************/
#include "techdec.h"

int main(int argc, char **argv)
{
    // mostly "counting" variables...
    int c;
    int i;
    int err;
    int skip;

    int *csize;
    char * fname;
    unsigned char * key;
    unsigned char * cipher_text;
    size_t keysize = 16;
    unsigned int port;

    // Parse the cmd line options
    // This should have been done
    // using getopt, but I never
    // had the chance to fix it!
    c = strlen(argv[1]);
    fname = (char *) malloc(c*sizeof(char));
    strncpy(fname,argv[1],c);

    csize = (int *) malloc(sizeof(int));

    
    if ((argc == 3) && (strcmp(argv[2],"-l")==0))
    {
        /* Stand alone mode first */
        int in_len = strlen(fname);
        char ofname[in_len+6];
        char tfname[in_len+6];
        for(i=0;i< c;++i)
        {
            if(fname[i] == '.')
            {
                skip = c - i;
                break;
            }
        }
        strncpy(tfname,fname,in_len);
        tfname[in_len-skip] = '\0';
        sprintf(ofname,"%s.check",tfname);

        // Check for filename conflict 
        if (access(ofname,F_OK)!=-1)
        {
            printf("Output File Exists!\n");
            return 33;
        }

        /* Get the key and setyp up libgcrypt */
        key = (unsigned char *) load_crypto(keysize);

        /* This handles the decryption */
        decrypt_file(fname,ofname,key,keysize);
    }
    else if ((argc == 4) && (strcmp(argv[2],"-d")==0))
    {
        /* Daemon mode */

        // Check for filename conflict 
        if (access(fname,F_OK)!=-1)
        {
            printf("Output File Exists!\n");
            return 33;
        }
        port = atoi(argv[3]); 
        
        // Get the cipher text from the network
        cipher_text = start_server(port,csize);

        // Get the key from the password
        key = (unsigned char *) load_crypto(keysize);
        
        // Decrypt the file
        err = decrypt_network_file(fname, key, keysize, cipher_text,*csize);
        return err;
    }
    else
    {
        /* Usage.... */
        printf("\nUsage: \n"
               "   techdec <filename> [-d <Port>] [-l]\n\n");
        return 1;
    }
    
    return 0;
}

