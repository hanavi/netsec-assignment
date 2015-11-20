/*****************************************
 * Author: James Casey
 * Description: Assignment 2
 *
 * This file contains the main crypto
 * functions.  This was probably the most 
 * fun part of the project... ableit
 * somewhat painful.
 *
 *****************************************/

#include "crypto.h"

unsigned char * load_crypto(size_t keysize)
{
  /* load the crypto and get the key from the password */

  int secure_mem = FALSE;
  char *passwd;
  size_t plen;
  int i;
  char buffer[10];

  // Some of the required parameters...
  char salt[] = "NaCl";
  size_t saltlen = strlen(salt);
  unsigned long iterations = 4096;
  unsigned char keybuffer[keysize];
  unsigned char *key;

  // I mostly have error checking, but I should have more...
  gpg_error_t err;

  /*********************************************************
   * The secure memory verision kept crashing... I think I
   * finally figured out why, but I didn't have time to go
   * back and check...
   * ******************************************************/
  if (!gcry_check_version (GCRYPT_VERSION))
  {
     fputs ("libgcrypt version mismatch\n", stderr);
     exit (2);
  }
  if (secure_mem == TRUE)
  {
     // no secure memory for us :-(
      gcry_control (GCRYCTL_SUSPEND_SECMEM_WARN);
      gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);
      gcry_control (GCRYCTL_RESUME_SECMEM_WARN);
      gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
  }
  else
  {
      //maybe next time
      gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
      gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
  }

  /*********************************************************/

  // It doesn't show up on the screen... 
  // I hope thats ok, since it is a little more
  // secure
  passwd = getpass("Password: ");
  plen = strlen(passwd);

  // Generate the key!
  err = gcry_kdf_derive ( passwd, plen, GCRY_KDF_PBKDF2, GCRY_MD_SHA512, 
     (const void *) salt, saltlen, iterations, keysize, keybuffer );

  // I dunno, I just have a problem with using too many buffers
  // DON JUDGE ME!!!
  key = (unsigned char *) malloc(keysize * sizeof (unsigned char));
  for (i = 0; i < keysize; ++i)
      key[i] = keybuffer[i];

  printf("Key: ");

  for (i=0; i<keysize; ++i)
  {
    if (i % 1 == 0)
        printf(" ");
    printf("%02x", keybuffer[i]);
  }
  printf("\n");
  return key;
}

int encrypt_file(char *fname, char *ofname, unsigned char *key, 
        size_t keysize)
{
    /* Encrypt a file in stand alone mode */

    FILE *ifd;
    FILE *ofd;
    size_t l = 16;

    // Buffers buffer buffers
    unsigned char in_buffer[l];
    unsigned char out_buffer[l];
    unsigned char *mac_buffer;

    // This took forever to get figured out!
    // It really pissed me off
    char *iv;
    iv = (char *) malloc(16);
    memset(iv,0,16);
    *iv = 0x16d4;


    int i;
    int c;

    ifd = (FILE *) fopen(fname,"r");
    ofd = (FILE *) fopen(ofname,"w");

    /* Setup the encryption information */
    gcry_error_t err;
    gcry_cipher_hd_t hd;
    gcry_cipher_hd_t hd_mac;

    // Needs more error checking...
    // This is the HMAC stuff
    err = gcry_md_open (&hd_mac, GCRY_MD_SHA512, GCRY_MD_FLAG_HMAC);
    err = gcry_md_setkey (hd_mac, key, keysize);

    // setup the encryption stuff
    err = gcry_cipher_open (&hd, GCRY_CIPHER_AES128, 
            GCRY_CIPHER_MODE_CBC, 0);
    if (err)
        printf("Failed to open cypher: %s - %s\n", 
                gcry_strsource(err), gcry_strerror(err));

    // set the key... now I am just being too obvious
    err = gcry_cipher_setkey (hd, key,  keysize);
    if (err)
        printf("Failed to set key: %s - %s\n",
                gcry_strsource(err), gcry_strerror(err));


    /* Encrypt the file */
    while (!feof(ifd) && !ferror(ifd))
    {
        c = fread(in_buffer,1,l,ifd);
        if (c < l)
        {
            for (i=c;i<l;++i)
            {
                in_buffer[i] = 0;
            }
        }

       
        // I didn't realize at first that you had to do 
        // this every single time....
        err = gcry_cipher_setiv (hd, iv, 16);
        if (err)
            printf("Failed to set iv: %s - %s\n",
                    gcry_strsource(err), gcry_strerror(err));

        err = gcry_cipher_encrypt (hd, out_buffer, l, in_buffer, l);
        if (err)
            printf("Failed to encrypt: %s - %s\n",
                    gcry_strsource(err), gcry_strerror(err));
        gcry_md_write (hd_mac, out_buffer, l);
        
        /* Output encrypted data */
        fwrite(out_buffer,1,l,ofd);

    }

    // Get the MAC info and save it to the end of the output
    mac_buffer = gcry_md_read (hd_mac, GCRY_MD_SHA512);
    fwrite(mac_buffer,1,64,ofd);


    //These kept segfaulting... my fault?
    //gcry_cipher_close(hd);
    //gcry_mac_close (hd_mac);

    fclose(ofd);

    return 0;
}

int encrypt_network_file(unsigned char *key, size_t keysize, 
        unsigned char * file_buffer, int file_len, int *cbuffer_len)
{
    /* encrypt files for network transfer */

    // block size... l is not very descriptive...
    // next time I will use blk_size or something
    size_t l = 16;

    // more too many buffers...
    unsigned char in_buffer[l];
    unsigned char out_buffer[l];
    unsigned char *cipher_text;
    unsigned char *mac_buffer;
    
    // More of the same... iv happiness
    char *iv;
    iv = (char *) malloc(16);
    memset(iv,0,16);
    *iv = 0x16d4;


    // counters... too many of these as well!
    int i,j,k;
    int c;
    int count;

    /* Setup the encryption information */
    gcry_error_t err;
    gcry_cipher_hd_t hd;
    gcry_cipher_hd_t hd_mac;

    // Set up the MAC work again
    err = gcry_md_open (&hd_mac, GCRY_MD_SHA512, GCRY_MD_FLAG_HMAC);
    err = gcry_md_setkey (hd_mac, key, keysize);

    // Start the encryption
    err = gcry_cipher_open (&hd, GCRY_CIPHER_AES128, 
            GCRY_CIPHER_MODE_CBC, 0);
    if (err)
        printf("Failed to open cypher: %s - %s\n", 
                gcry_strsource(err), gcry_strerror(err));

    // Set the key
    err = gcry_cipher_setkey (hd, key,  keysize);
    if (err)
        printf("Failed to set key: %s - %s\n",
                gcry_strsource(err), gcry_strerror(err));

    /* Encrypt the file */
    int x = file_len / l;
    int nsize = (x+1)*16;
    cipher_text = (unsigned char *) malloc((nsize)*sizeof(unsigned char)+64);
    memset(cipher_text,0,file_len);
    count = file_len/l+1;
    
    for(i = 0; i < count; ++i)
    {
        if (i < count-1)
        {
            for(j = 0; j < l; ++j)
            {
                in_buffer[j] = file_buffer[i*l+j];
            }
        }
        else
        {
            k = file_len % l;
            for(j = 0; j < k; ++j)
            {
                in_buffer[j] = file_buffer[i*l+j];
            }
            for(j = k; j < l; ++j)
            {
                in_buffer[j] = 0;
            }
        }

        err = gcry_cipher_setiv (hd, iv, 16);
        if (err)
            printf("Failed to set iv: %s - %s\n",
                    gcry_strsource(err), gcry_strerror(err));

        err = gcry_cipher_encrypt (hd, out_buffer, l, in_buffer, l);
        if (err)
            printf("Failed to encrypt: %s - %s\n",
                    gcry_strsource(err), gcry_strerror(err));
        gcry_md_write (hd_mac, out_buffer, l);
        
        /* Output encrypted data */
        for (j = 0; j < l; ++j)
        {
            cipher_text[i*l+j] = out_buffer[j];
            *cbuffer_len = i * l + j;
        }
    }
    // get the MAC digest and send it off!
    mac_buffer = gcry_md_read (hd_mac, GCRY_MD_SHA512);

    i += 1;
    for (j = 0; j < 64; ++j)
    {
        cipher_text[i*l+j] = mac_buffer[j];
        *cbuffer_len = i * l + j;
    }

    //gcry_cipher_close(hd);
    return cipher_text;
}

int decrypt_file(char *fname, char *ofname, unsigned char *key, size_t keysize)
{
    /* Decrypt in standalone mode */

    FILE *ifd;
    FILE *ofd;
    size_t l = 16;
    int file_len;

    unsigned char in_buffer[l];
    unsigned char out_buffer[l];
    unsigned char tmp_buffer[l];
    memset(tmp_buffer,0,sizeof(tmp_buffer));


    char *iv;
    iv = (char *) malloc(16);
    memset(iv,0,16);
    *iv = 0x16d4;

    int i,j;
    int c;
    int in_len;

    // open our files!
    // in file 
    ifd = (FILE *) fopen(fname,"r");
    // out file
    ofd = (FILE *) fopen(ofname,"w");

    // Set up gcrypt stuffs
    gcry_error_t err;
    gcry_cipher_hd_t hd;
    gcry_cipher_hd_t hd_mac;

    // set up HMAC
    err = gcry_md_open (&hd_mac, GCRY_MD_SHA512, GCRY_MD_FLAG_HMAC);
    err = gcry_md_setkey (hd_mac, key, keysize);

    // set up encryption
    err = gcry_cipher_open (&hd, GCRY_CIPHER_AES128, 
            GCRY_CIPHER_MODE_CBC, 0);
    if (err)
        printf("Failed to open cypher: %s - %s\n", 
                gcry_strsource(err), gcry_strerror(err));

    err = gcry_cipher_setkey (hd, key,  keysize);
    if (err)
        printf("Failed to set key: %s - %s\n",
                gcry_strsource(err), gcry_strerror(err));

    // Get the in file length to make things easier
    fseek(ifd, 0, SEEK_END);
    file_len = ftell(ifd);
    fseek(ifd, 0, SEEK_SET);

    // How many iterations do we have?
    int count = file_len/l - 64/l;
    int first = TRUE;
    
    // Read the files in and encrypt them out!
    for(i = 0; i < count; ++i)
    {
        c = fread(in_buffer,1,l,ifd);
        err = gcry_cipher_setiv (hd, iv, 16);
        if (err)
            printf("Failed to set iv: %s - %s\n",
                    gcry_strsource(err), gcry_strerror(err));
        err = gcry_cipher_decrypt (hd, out_buffer, l, in_buffer, l);
        if (err)
            printf("Failed to decrypt: %s - %s\n",
                   gcry_strsource(err), gcry_strerror(err));
        gcry_md_write (hd_mac, in_buffer, l);
        if (i < count - 1)
        {
            fwrite(out_buffer,1,l,ofd);
        }
        else
        {
            // get rid of padding
            c = 0; 
            for (j=0;j<l;++j)
            {
                if(out_buffer[j] != 0)
                {
                    c += 1;
                }
            }
            c+=1;
            fwrite(out_buffer,1,c,ofd);
        }
    }

    // Check the current MAC vs what it is supposed to be
    unsigned char *mac_buffer_check;
    unsigned char *mac_buffer;
    mac_buffer = (unsigned char *)malloc(64);
    mac_buffer_check = gcry_md_read (hd_mac, GCRY_MD_SHA512);
    // Get the mac from the end of the data stream
    for(i = 0; i < 64/l; ++i)
    {
        c = fread(in_buffer,1,l,ifd);
        for(j = 0; j < l; ++j)
        {
            mac_buffer[i*l+j] = in_buffer[j];
        }
    }


    // Close it up... 
    fclose(ifd);
    fclose(ofd);

    int check = memcmp(mac_buffer,mac_buffer_check,64);
    if (check != 0 )
    {
        // if you are here, something went very wrong!
        printf("HMAC Verification Failed!\n");
        return 62;
    } 

    // YAY!
    printf("Successfully Decrypted File\n");

    //gcry_cipher_close(hd);


    return 0;
}

int decrypt_network_file(char *fname, unsigned char *key, size_t keysize, 
        unsigned char * cipher_text, int file_len)
{
    /* Decrypt network data */

    FILE *ofd;
    size_t l = 16;

    // too many bufferses
    unsigned char in_buffer[l];
    unsigned char out_buffer[l];
    unsigned char tmp_buffer[l];
    unsigned char * plain_text;

    // Mac initial vector
    char *iv;
    iv = (char *) malloc(16);
    memset(iv,0,16);
    *iv = 0x16d4;

    int i,j,k,c;
    int count;

    // ope the file for output
    ofd = (FILE *) fopen(fname,"w");

    // set up gcrypt variables
    gcry_error_t err;
    gcry_cipher_hd_t hd;
    gcry_cipher_hd_t hd_mac;

    // set up hmac
    err = gcry_md_open (&hd_mac, GCRY_MD_SHA512, GCRY_MD_FLAG_HMAC);
    err = gcry_md_setkey (hd_mac, key, keysize);

    // set up aes 
    err = gcry_cipher_open (&hd, GCRY_CIPHER_AES128, 
            GCRY_CIPHER_MODE_CBC, 0);
    if (err)
        printf("Failed to open cypher: %s - %s\n", 
                gcry_strsource(err), gcry_strerror(err));

    err = gcry_cipher_setkey (hd, key,  keysize);
    if (err)
        printf("Failed to set key: %s - %s\n",
                gcry_strsource(err), gcry_strerror(err));

    
    // Get our buffer for our plain text
    plain_text = (unsigned char *) malloc(file_len*sizeof(unsigned char));

    // This is for making sure we can identify the last block...
    // or it was before I fixed it.
    int first = TRUE;

    // This is just to make the code a little easier to read
    int step;

    // Number of iterations
    count = file_len/l - 64/l;
    for(i = 0; i < count; ++i)
    {
        for(j = 0; j < l; ++j)
        {
            step = i*l + j;
            in_buffer[j] = cipher_text[step];
        }

        // Encryption and HMAC
        err = gcry_cipher_setiv (hd, iv, 16);
        if (err)
            printf("Failed to set iv: %s - %s\n",
                gcry_strsource(err), gcry_strerror(err));

        err = gcry_cipher_decrypt (hd, tmp_buffer, l, in_buffer, l);
        if (err)
            printf("Failed to decrypt: %s - %s\n",
                   gcry_strsource(err), gcry_strerror(err));
        gcry_md_write (hd_mac, in_buffer, l);

        if (first == TRUE)
        {
            // I don't remember why I had it set up like this
            // but there was a reason...
            first = FALSE;
            for (k = 0; k < l; ++k)
                out_buffer[k] = tmp_buffer[k];
        }
        else
        {
            fwrite(out_buffer,1,l,ofd);
            for (k = 0; k < l; ++k)
                out_buffer[k] = tmp_buffer[k];
        }
    }
    

    // Drop the padding
    c = 0; 
    for (i=0;i<l;++i)
    {
        if(out_buffer[i] != 0)
        {
            c += 1;
        }
    }
    c+=1;
    fwrite(out_buffer,1,c,ofd);
    //gcry_cipher_close(hd);


    // compare the MACs
    unsigned char *mac_buffer_check;
    unsigned char *mac_buffer;
    mac_buffer = (unsigned char *)malloc(64);
    mac_buffer_check = gcry_md_read (hd_mac, GCRY_MD_SHA512);
    i = (count+1)*l;
    for(j = 0; j < 64; ++j)
    {
        mac_buffer[j] = cipher_text[i+j];
    }

    int check = memcmp(mac_buffer,mac_buffer_check,63);
    if (check != 0 )
    {
        // 1f y0u 4r3 h3r3 y0U h4v3 b33n h4x0red!
        printf("HMAC Verification Failed!\n");
        return 62;
    } 

    // Uber cyber ninja
    printf("Successfully Decrypted File\n");

    fclose(ofd);
    return 0;
}
