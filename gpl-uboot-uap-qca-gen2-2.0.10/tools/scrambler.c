
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char key[] = "\x81\x93\xe0\xc4";
static const size_t key_len = 4;

unsigned char pub_key[2048];

void
descramble(unsigned char* data, unsigned int datalen)
{
    unsigned int i;

    for (i = 0; i < datalen; ++i) {
         data[i] ^= key[ i % key_len ];
    }
}


void usage()
{
    fprintf(stdout, "scrambler <public_key_filename>\n");
}

int create_key_file(char *fn, char *pstr, int tot_len)
{
    int i=0;
    FILE *fp = fopen(strcat(fn, ".scrambled"), "w+");

    if (!fp) {
     perror("Error opening key_file.c\n");
     return -1;
    }

     for (i=0; i< tot_len; i++) {
          fprintf(fp, "%c", (char)pstr[i]);
        }

    fclose(fp);

    return 0;
}


int main(int argc, char *argv[])
{
	char *p, *line=NULL;
        size_t len = 294, rlen=0, tot_len=0;
        FILE *fp;

        if (argc < 2) {
           usage();
           exit (1);
        }

        fp = fopen(argv[1], "r");
        if (!fp) {
            perror("Error in opening the public key file \n");
            exit(1);
            return -1;
        }

        p = pub_key;

       rlen=fread(p, len, 1, fp);

        if (rlen != 1) {
            printf("Error: Was able to read only %d bytes instead of %d\n",
                    rlen, len);
            fclose(fp);
            return 0;
        }

        fclose(fp);

	descramble(pub_key, len);

        return create_key_file(argv[1], pub_key, len);
}
