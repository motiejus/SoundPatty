#include <stdlib.h>
#include <string.h>
#include <cstdio>

using namespace std;

int main (int argc, char *argv[]) {
    if (argc != 2) {
        perror ("Unspecified input WAV file. exiting\n");
        exit(1);
    }
    FILE *in = fopen(argv[1], "r");
    
    // Read bytes [0-3] and [8-11], they should be "RIFF" and "WAVE" in ASCII
    char h[4];
    fread(h, sizeof(char), 4, in);
    h[4] = '\0';

    //printf ("Read from input file: %s, sample: %s", h, "RIFF");
    if (strcmp(h, "RIFF") != 0) {
        perror ("RIFF header not found, exiting\n");
        exit(1);
    }
    // Move 4 bytes forward to find the "WAVE" string
    long pos;
    pos = ftell(in);

    //printf ("Current file position: %d\n", pos);



    exit(0);
    short int buf [100]; // 200 byte array
    while (! feof(in) ) {
        fread(buf, 2, 100, in);
        for (int i = 0; i < 100; i++) {
            printf ("%d ", buf[i]);
        }
        //printf ("%s\n", buf);
    }

    exit(0);

}
