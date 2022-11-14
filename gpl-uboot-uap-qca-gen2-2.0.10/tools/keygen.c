/*
 * Copyright (c) 2013, Google Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <rsa/rsa.h>

    /* Ignore the following strings in the public key while reading */          
char *start_end_str[] = {                                                       
        "-----BEGIN PUBLIC KEY-----",                                           
        "-----END PUBLIC KEY-----"                                              
};

int convert_bignum_to_uint32(BIGNUM *, int, uint32_t **);

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
#define HAVE_ERR_REMOVE_THREAD_STATE
#endif

static int rsa_err(const char *msg)
{
        unsigned long sslErr = ERR_get_error();

        fprintf(stderr, "%s", msg);
        fprintf(stderr, ": %s\n",
                ERR_error_string(sslErr, 0));

        return -1;
}

/**
 * rsa_get_pub_key() - read a public key from a .rsa file
 *
 * @keydir:     Directory containins the key
 * @name        Name of key file (will have a .crt extension)
 * @rsap        Returns RSA object, or NULL on failure
 * @return 0 if ok, -ve on error (in which case *rsap will be set to NULL)
 */
static int rsa_get_pub_key(const char *name, RSA **rsap)
{
        EVP_PKEY *key;
        RSA *rsa;
        FILE *f;
        int ret;
        size_t len, decoded_len;
        char path[1024], *line=NULL;
        unsigned char *pstr, *p, *evp;

        *rsap = NULL;
        snprintf(path, sizeof(path), "%s", name);
        f = fopen(path, "r");
        if (!f) {
                fprintf(stderr, "Couldn't open RSA certificate: '%s': %s\n",
                        path, strerror(errno));
                return -EACCES;
        }

        p = (unsigned char *) malloc(2048);
        evp = (unsigned char *) malloc(2048);
                                                                                
        if (!p || !evp) {                                                               
            perror(" Malloc failed \n");                                        
            exit(1);                                                            
        }

       pstr = p;
                                                                                
        while (getline(&line, &len, f) != -1) {
            /* Ignore the begin and end strings of public key */
            if (!strncmp(line, start_end_str[0], strlen(start_end_str[0])) ||
                !strncmp(line, start_end_str[1], strlen(start_end_str[1])))
                continue;
 
             /* remove the end-of-line for each read by subtracting -1 */
            strncpy((char *)p, line, strlen(line)-1);
            p += strlen(line)-1;
        }

        decoded_len = EVP_DecodeBlock(evp, pstr, strlen((const char *)pstr));

        key = d2i_PUBKEY(NULL, (const unsigned char **)&evp, decoded_len);

        if (!key) {
                rsa_err("Couldn't read public key\n");
                ret = -EINVAL;
                goto err_pubkey;
        }

        /* Convert to a RSA_style key. */
        rsa = EVP_PKEY_get1_RSA(key);
        if (!rsa) {
                rsa_err("Couldn't convert to a RSA style key");
                goto err_rsa;
        }
        fclose(f);
        EVP_PKEY_free(key);
        *rsap = rsa;

        return 0;

err_rsa:
        EVP_PKEY_free(key);
err_pubkey:
        free(pstr);
        free(evp);
        fclose(f);
        return ret;
}


/*
 * rsa_get_params(): - Get the important parameters of an RSA public key
 */
int rsa_get_params(RSA *key, uint32_t *n0_invp, BIGNUM **modulusp,
                   BIGNUM **r_squaredp)
{
        BIGNUM *big1, *big2, *big32, *big2_32;
        BIGNUM *n, *r, *r_squared, *tmp;
        BN_CTX *bn_ctx = BN_CTX_new();
        int ret = 0;

        /* Initialize BIGNUMs */
        big1 = BN_new();
        big2 = BN_new();
        big32 = BN_new();
        r = BN_new();
        r_squared = BN_new();
        tmp = BN_new();
        big2_32 = BN_new();
        n = BN_new();
        if (!big1 || !big2 || !big32 || !r || !r_squared || !tmp || !big2_32 ||
            !n) {
                fprintf(stderr, "Out of memory (bignum)\n");
                return -ENOMEM;
        }

        if (!BN_copy(n, key->n) || !BN_set_word(big1, 1L) ||
            !BN_set_word(big2, 2L) || !BN_set_word(big32, 32L))
                ret = -1;

        /* big2_32 = 2^32 */
        if (!BN_exp(big2_32, big2, big32, bn_ctx))
                ret = -1;

        /* Calculate n0_inv = -1 / n[0] mod 2^32 */
        if (!BN_mod_inverse(tmp, n, big2_32, bn_ctx) ||
            !BN_sub(tmp, big2_32, tmp))
                ret = -1;
        *n0_invp = BN_get_word(tmp);

        /* Calculate R = 2^(# of key bits) */
        if (!BN_set_word(tmp, BN_num_bits(n)) ||
            !BN_exp(r, big2, tmp, bn_ctx))
                ret = -1;

        /* Calculate r_squared = R^2 mod n */
        if (!BN_copy(r_squared, r) ||
            !BN_mul(tmp, r_squared, r, bn_ctx) ||
            !BN_mod(r_squared, tmp, n, bn_ctx))
                ret = -1;

        *modulusp = n;
        *r_squaredp = r_squared;

        BN_free(big1);
        BN_free(big2);
        BN_free(big32);
        BN_free(r);
        BN_free(tmp);
        BN_free(big2_32);
        if (ret) {
                fprintf(stderr, "Bignum operations failed\n");
                return -ENOMEM;
        }

        return ret;
}

int rsa_preprocess_key(char *keyfile, struct rsa_public_key *keydest)
{
        BIGNUM *modulus, *r_squared;
        uint32_t n0_inv, *ptr;
        int ret, bits;
        RSA *rsa;

        ret = rsa_get_pub_key(keyfile, &rsa);

        if (ret)
                return ret;

        ret = rsa_get_params(rsa, &n0_inv, &modulus, &r_squared);
        
        if (ret)
                return ret;

        bits = BN_num_bits(modulus);

        keydest->n0inv = n0_inv;

        convert_bignum_to_uint32(modulus, bits, &ptr);
        keydest->modulus = ptr;

        convert_bignum_to_uint32(r_squared, BN_num_bits(r_squared), &ptr);
        keydest->rr = ptr;

        return 0;
}

void rsa_convert_big_endian(uint32_t *dst, const uint32_t *src, int len)
{
        int i;

        for (i = 0; i < len; i++) 
                dst[i] = ntohl(src[len - 1 - i]);
}


int convert_bignum_to_uint32(BIGNUM *num, int num_bits, uint32_t **out)
{
        int nwords = num_bits / 32;
        int size;
        uint32_t *buf, *ptr, *buf1;
        BIGNUM *tmp, *big2, *big32, *big2_32;
        BN_CTX *ctx;
        int ret;

        tmp = BN_new();
        big2 = BN_new();
        big32 = BN_new();
        big2_32 = BN_new();
        if (!tmp || !big2 || !big32 || !big2_32) {
                fprintf(stderr, "Out of memory (bignum)\n");
                return -ENOMEM;
        }
        ctx = BN_CTX_new();
        if (!tmp) {
                fprintf(stderr, "Out of memory (bignum context)\n");
                return -ENOMEM;
        }
        BN_set_word(big2, 2L);
        BN_set_word(big32, 32L);
        BN_exp(big2_32, big2, big32, ctx); /* B = 2^32 */

        size = nwords * sizeof(uint32_t);
        buf = malloc(size);
        buf1 = malloc(size);
        if (!buf) {
                fprintf(stderr, "Out of memory (%d bytes)\n", size);
                return -ENOMEM;
        }

        for (ptr = buf + nwords - 1; ptr >= buf; ptr--) {
                BN_mod(tmp, num, big2_32, ctx); /* n = N mod B */
                *ptr = htonl(BN_get_word(tmp));
                BN_rshift(num, num, 32); /*  N = N/B */
        }
        /* Convert to big endian */
        rsa_convert_big_endian(buf1, buf,  nwords);
        *out = buf1;
        free(buf);
        BN_free(tmp);
        BN_free(big2);
        BN_free(big32);
        BN_free(big2_32);

        return ret;
}


int write_key_file(struct rsa_public_key *key, char*outfile)
{
        FILE *f=NULL;
        const char header_str[] = "      /* Copyright Ubiquiti Networks Inc \n "
                                   "        Auto generated RSA public key file for firmware "
                                   "authentication. \n         DO NOT EDIT. */ ";
        const char space_str[] = "\n\n";
        const char n0_inv_str[] = "unsigned int n0_inv __attribute__((section(\".keyfile\")))  = ";
        const char modulus_str[] = "unsigned int modulus[] __attribute__((section(\".keyfile\")))= {";
        const char r_squared_str[] = "unsigned int r_squared[] __attribute__((section(\".keyfile\")))= {";
        const char end_str[] = "};";
        int i;
        
        if (!(f = fopen(outfile, "w")))
        {
                printf(" Error opening file :%d \n", errno);
                return errno;
        }

        fprintf(f, "%s%s \n\n",  header_str, space_str);

        fprintf(f, "%s 0x%08x;\n\n", n0_inv_str, key->n0inv);

        fprintf(f, "%s \n",  modulus_str);

        for (i=0; i<key->len; i+=4) 
        {
           fprintf( f, "0x%08x, 0x%08x, 0x%08x, 0x%08x, \n", key->modulus[i], 
                    key->modulus[i+1], key->modulus[i+2], key->modulus[i+3]);
        }

        fprintf(f, "%s%s%s\n", end_str, space_str, r_squared_str);

        for (i=0; i<key->len; i+=4) 
        {
           fprintf( f, "0x%08x, 0x%08x, 0x%08x, 0x%08x, \n", key->rr[i], 
                    key->rr[i+1], key->rr[i+2], key->rr[i+3]);
        }
        
        fprintf(f, "%s\n", end_str);
        fclose(f);

        return 0;
}

void usage() 
{

  fprintf(stdout, "keygen <certificate file> <outputfile>\n");

} 

int main(int argc, char *argv[])
{
        struct rsa_public_key key; 

        if (argc < 3) {
          usage();
          return -1;
        }
        rsa_preprocess_key(argv[1], &key);
        key.len = 64;

        write_key_file(&key, argv[2]);

        return 0;
}
