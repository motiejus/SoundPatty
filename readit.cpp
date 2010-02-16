#include <stdlib.h>
#include <string.h>
#include <cstdio>

using namespace std;
bool check_sample(const char*, const char*);
void fatal (const char* );

char errmsg[200]; // Temporary message
int SAMPLERATE;

int main (int argc, char *argv[]) {
    if (argc != 2) {
        fatal ("Unspecified input WAV file. exiting\n");
    }
    FILE *in = fopen(argv[1], "rb");
    
    // Read bytes [0-3] and [8-11], they should be "RIFF" and "WAVE" in ASCII
    char header[5];
    fread(header, 1, 4, in);

    char sample[] = "RIFF";
    if (! check_sample(sample, header) ) {
        fatal ("RIFF header not found, exiting\n");
    }
	// Checking for compression code (21'st byte is 01, 22'nd - 00, little-endian notation
	short int tmp[2]; // two-byte integer
	fseek(in, 0x14, 0); // offset = 20 bytes
    fread(&tmp, 2, 2, in); // Reading two two-byte samples (comp. code and no. of channels)
    if ( tmp[0] != 1 ) {
		sprintf(errmsg, "Only PCM(1) supported, compression code given: %d\n", tmp[0]);
		fatal (errmsg);
    }
	// Number of channels must be "MONO"
    if ( tmp[1] != 1 ) {
		sprintf(errmsg, "Only MONO supported, given number of channels: %d\n", tmp[1]);
		fatal (errmsg);
    }
	fread(&SAMPLERATE, 2, 1, in); // Single two-byte sample
	//printf ("Sample rate: %d\n", SAMPLERATE);
	fseek(in, 0x24, 0);
	fread(header, 1, 4, in);
	strcpy(sample, "data");
	if (! check_sample(sample, header)) {
		fatal ("data chunk not found in byte offset=36, bad file.");
	}

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
