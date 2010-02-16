#include <stdlib.h>
#include <string.h>
#include <cstdio>

using namespace std;
bool check_sample(const char*, const char*);
void fatal (const char* );

int main (int argc, char *argv[]) {
    if (argc != 2) {
        fatal ("Unspecified input WAV file. exiting\n");
    }
    FILE *in = fopen(argv[1], "rb");
    
    // Read bytes [0-3] and [8-11], they should be "RIFF" and "WAVE" in ASCII
    char h[5];
    fread(h, 1, 4, in);

    char sample[] = "RIFF";
    if (! check_sample(sample, h) ) {
        fatal ("RIFF header not found, exiting\n");
    }
    long pos = ftell(in);
 
    short int buf [100]; // 200 byte array
    while (! feof(in) ) {
        printf ("OK\n");
        fread(buf, 2, 100, in);
        for (int i = 0; i < 100; i++) {
            printf ("%d ", buf[i]);
        }
        //printf ("%s\n", buf);
    }

    exit(0);

}

bool check_sample (const char * sample, const char * b) { // My STRCPY.
    for(int i = 0; i < sizeof(sample)-1; i++) {
        if (sample[i] != b[i]) {
            return false;
        }
    }
    return true;
}

void fatal (const char * msg) { // Print error message and exit
    perror (msg);
    exit (1);
}
