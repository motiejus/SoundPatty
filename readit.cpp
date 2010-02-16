#include <stdlib.h>
#include <string.h>
#include <cstdio>

#define BUFFERSIZE 1 // Number of sample rates

using namespace std;
bool check_sample(const char*, const char*);
void process_headers(const char*);
void fatal (const char* );

char errmsg[200]; // Temporary message
FILE *in;
int SAMPLERATE, DATA_SIZE;

int main (int argc, char *argv[]) {
    if (argc != 2) {
        fatal ("Unspecified input WAV file. exiting\n");
    }

    process_headers(argv[1]);

    short int buf [SAMPLERATE * BUFFERSIZE]; // Process buffer every second
    while (! feof(in) ) {
        fread(buf, 2, SAMPLERATE * BUFFERSIZE, in);
        for (int i = 0; i < SAMPLERATE * BUFFERSIZE; i++) {
            printf ("%d ", buf[i]);
        }
        // Let's do something with that numbers
    }

    exit(0);

}

void process_headers(const char * infile) {
    in = fopen(infile, "rb");
    
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
		fatal ("data chunk not found in byte offset=36, file corrupted.");
	}
    // Get data chunk size here
	fread(&DATA_SIZE, 4, 1, in); // Single two-byte sample
    //printf ("Datasize: %d bytes\n", DATA_SIZE);
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
