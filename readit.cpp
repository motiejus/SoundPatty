/*
 * Only 16 bit PCM MONO supported now.
 */
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#define BUFFERSIZE 1 // Number of sample rates (ex. seconds)
#define MINWAVEFREQ 20 // Minimum frequency of sound that we want to process
#define CHUNKSIZE 0.1 // Minimum chunk that we count
#define MAXSTEPS 3 // How many steps to skip when we are looking for silences'
#define MATCHME 7 // Matching this number of silences means success

using namespace std;

FILE * process_headers(const char*);
bool check_sample(const char*, const char*);
void dump_values(FILE * in);
void read_periods(const char*);
void read_cfg(const char*);
void fatal (const char* );

char errmsg[200];   // Temporary message
int SAMPLE_RATE,    // 
    WAVE,           // SAMPLE_RATE*MINWAVEFREQ
    DATA_SIZE;      // Data chunk size in bytes

map<string, string> cfg; // OK, ok... Is it possible to store any value but string (Rrrr...) here?

class Range {
    public:
        Range() { tm = tmin = tmax = 0; };
        Range(const double & a) { tm = a; tmin = a * 0.9; tmax = a * 1.1; }
        Range(const double & tmin, const double &tm, const double &tmax) { this->tmin = tmin; this->tm = tm; this->tmax = tmax; }
        Range(const double & tmin, const double &tmax) { this->tmin = tmin; this->tmax = tmax; }
        Range operator = (const double i) { return Range(i); }
        bool operator == (const double & i) const { return tmin < i && i < tmax; }
        bool operator == (const Range & r) const { return r == tm; }
        bool operator > (const double & i) const { return i > tmax; }
        bool operator > (const Range & r) const { return r > tm; }
        bool operator < (const double & i) const { return i < tmin; }
        bool operator < (const Range & r) const { return r < tm; }
        double tmin, tm, tmax;
};

struct CrapRange {
    int last, head, last_max, tail, min, max;
    bool disable;
};

class workitm {
    public:
        int len,a,b;
        workitm(int, int);
        int ami(int, int);
        list<pair<int, int> > trace;
};

workitm::workitm(const int a, const int b) {
    this->a = a; this->b = b;
    len = 0;
    trace.push_back(pair<int,int>(a,b));
};

void its_over(workitm * w) {
    printf("FOUND\n");
    /*
       printf("Found a matching pattern! Length: %d, here comes the trace:\n", (int)w->trace.size());
       for (list<pair<int,int> >::iterator tr = w->trace.begin(); tr != w->trace.end(); tr++) {
       printf("%d %d\n", tr->first, tr->second);
       }
     */
    exit(0);
}

typedef multimap<Range,pair<int,double> > tvals;

int main (int argc, char *argv[]) {
    if (argc < 3) {
        fatal ("Usage: ./a.out config.cfg sample.wav\n\nor\n"
                "./a.out config.cfg samplefile.txt catchable.wav");
    }
    read_cfg(argv[1]);
    // ------------------------------------------------------------
    // Only header given, we just dump out the silence values
    //
    if (argc == 3) {
        FILE * in = process_headers(argv[2]);
        dump_values(in);
    }
    // ------------------------------------------------------------
    // We have values file and a new samplefile. Let's rock
    //
    else if (argc == 4) {
        tvals vals;

        ifstream file;
        file.open(argv[2]);
        string line;
        int x;
        double tmp2;
        for (int i = 0; !file.eof(); i++) {
            getline(file, line);
            x = line.find(";");
            if (x == -1) break;
            istringstream place(line.substr(0,x));
            istringstream range(line.substr(x+1));
            pair<Range,pair<int,double> > tmp;
            range >> tmp2;
            tmp.first = Range(tmp2);
            place >> tmp.second.second;
            tmp.second.first = i;
            vals.insert(tmp);
        }
        // OK, we have it. Let's print?
        //for (vector<trange>::iterator it1 = vals.begin(); it1 != vals.end(); it1++) {
        //printf ("%.6f - %.6f\n", (*it1).first, (*it1).second);
        //}
        // ------------------------------------------------------------
        // Let's work with the seconds sample like with a stream
        //
        FILE * an = process_headers(argv[3]);
        // We can read and analyze it now
        // Trying to match in only first 15 seconds of the record

        short int buf [SAMPLE_RATE * BUFFERSIZE]; // Process buffer every second
        istringstream i(cfg["treshold1"]);
        double treshold1;
        i >> treshold1;
        int min_silence = (int)((1 << 15) * treshold1), // Below this value we have "silence, pshhh..."
            found_s = 0,
            b = -1, // This is the found silence counter (starting with zero)
            head = 0;
        list<workitm> work;

        // ------------------------------------------------------------
        // Print vals<r Range, int number>
        //
        while (! feof(an) ) {
            fread(buf, 2, SAMPLE_RATE * BUFFERSIZE, an);

            if (head/SAMPLE_RATE >= 30) {
                printf ("Exiting after 30 seconds, ");
                break;
            }

            for (int i = 0; i < SAMPLE_RATE * BUFFERSIZE; i++) {
                int cur = abs(buf[i]);
                if (cur <= min_silence) {
                    found_s++;
                } else {
                    if (found_s >= WAVE) {
                        b++; // Increasing the number of silence counter (sort of current index)
                        // Found found_s/SAMPLE_RATE length silence
                        // Look for an analogous thing in vals variable (original file)
                        double sec = (double)found_s/SAMPLE_RATE;
                        // Find sec in range
                        pair<tvals::iterator, tvals::iterator> pa = vals.equal_range(sec);

                        //------------------------------------------------------------
                        // We put a indexes here that we use for continued threads
                        // (we don't want to create a new "thread" with already
                        // used length of a silence)
                        //
                        set<int> used_a; 

                        //------------------------------------------------------------
                        // Iterating through samples that match the found silence
                        //
                        for (tvals::iterator in_a = pa.first; in_a != pa.second; in_a++)
                        {
                            int a = in_a->second.first;
                            //------------------------------------------------------------
                            // Check if it exists in our work array
                            //
                            for (list<workitm>::iterator w = work.begin(); w != work.end();) {
                                if (b - w->b > MAXSTEPS) {
                                    work.erase(w); w = work.begin(); continue;
                                }
                                if (b == w->b || a - w->a > MAXSTEPS || w->a >= a) { w++; continue; }
                                // ------------------------------------------------------------
                                // We fit the "region" here. We either finished,
                                // or just increasing len
                                //
                                w->a = a; w->b = b;
                                w->trace.push_back(pair<int,int>(a,b));
                                if (++(w->len) < MATCHME) { // Proceeding with the "thread"
                                    used_a.insert(a);
                                    //printf ("Thread expanded to %d\n", w->len);
                                } else { // This means the treshold is reached
                                    // This hack is needed to pass pointer to an instance o_O
                                    its_over(&(*w));
                                }
                                w++;
                                // End of work iteration array
                            }

                            if (used_a.find(a) == used_a.end()) {
                                work.push_back(workitm(a,b));
                            }
                        }
                    }
                    // Looking for another silence
                    found_s = 0;
                }
                head++;
            }
        }
        printf("NOT FOUND\n");
    }
    exit(0);
}

void read_cfg (const char * filename) {
    ifstream file;
    file.open(filename);
    string line;
    int x;
    while (! file.eof() ) {
        getline(file, line);
        x = line.find(":");
        if (x == -1) break; // Last line, exit
        cfg[line.substr(0,x)] = line.substr(x+2);
    }
}

FILE * process_headers(const char * infile) {
    FILE * in = fopen(infile, "rb");

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
        printf(errmsg, "Only MONO supported, given number of channels: %d\n", tmp[1]);
        fatal (errmsg);
    }
    fread(&SAMPLE_RATE, 2, 1, in); // Single two-byte sample
    WAVE = SAMPLE_RATE*MINWAVEFREQ;
    fseek(in, 0x24, 0);
    fread(header, 1, 4, in);
    strcpy(sample, "data");
    if (! check_sample(sample, header)) {
        fatal ("data chunk not found in byte offset=36, file corrupted.");
    }
    // Get data chunk size here
    fread(&DATA_SIZE, 4, 1, in); // Single two-byte sample
    //printf ("Datasize: %d bytes\n", DATA_SIZE);
    return in;
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

void dump_values(FILE * in) {
    double treshold1 = 0.2;
    // ------------------------------------------------------------
    // This "15" means we have 16 bits for signed, this means
    // 15 bits for unsigned int.

    // FIX THESE depending on bitrate!!
    vector<CrapRange> ranges;
    CrapRange tmp;

    tmp.min = tmp.disable = tmp.last = tmp.last_max = 0;
    tmp.max = (int)(1<<15)*0.2;
    ranges.push_back(tmp);
    printf ("min: %d, max: %d, last_max: %d, last: %d, disable: %d\n", tmp.min, tmp.max, tmp.last_max, tmp.last, tmp.disable);

    tmp.min = (int)(1<<15)*0.8; tmp.max = (int)(1<<15)*1;
    ranges.push_back(tmp);

    printf ("min: %d, max: %d, last_max: %d, last: %d, disable: %d\n", tmp.min, tmp.max, tmp.last_max, tmp.last, tmp.disable);
    exit(1);
    short int buf [SAMPLE_RATE * BUFFERSIZE]; // Process buffer every second
    for (int i = 0; !feof(in); noop();) {
        fread(buf, 2, SAMPLE_RATE * BUFFERSIZE, in);
        for (int j = 0; j < SAMPLE_RATE * BUFFERSIZE; j++) {
            int cur = abs(buf[j]);

            for (vector<CrapRange>::iterator R = ranges.begin(); R != ranges.end(); R++) {
                if (R->min < cur && cur <= R->max) { // cur in Range
                    if (R->disable or i - R->last > WAVE) {
                        R->tail = i;
                    }
                    R->disable = 1;
                    R->last = i; // Last_item_in_interval
                    R->last_max = i; // last maximum in this range
                }
                // If current value exceeds maximum, we kick it away
                else {
                    if (cur > R->max || i - R->last > WAVE && R->last_max < R->min) {
                        if (R->disable) {
                            // We should add a new value here to our working array (found a treshold)
                            printf("(%.2f-%.2f) Start: %.6f, Length: %.6f\n",
                                    R->min / (1<<15), R->max/(1<<15), R->tail, R->last - R->tail);
                        }
                    }
                }

            }
        }
    }



    /*
    int min_silence = (int)((1 << 15) * treshold1); // Below this value we have "silence, pshhh..."
    short int buf [SAMPLE_RATE * BUFFERSIZE]; // Process buffer every second
    int head = 0, // Assume sample started
        found_s = 0; // Number of silence samples
    int first_silence = 0;

    while (! feof(in) ) {
        fread(buf, 2, SAMPLE_RATE * BUFFERSIZE, in);
        for (int i = 0; i < SAMPLE_RATE * BUFFERSIZE; i++) {
            int cur = abs(buf[i]);

            if (cur <= min_silence) {
                found_s++;
            } else {
                if (found_s >= WAVE) {
                    //printf("%.6f;%.6f\n", (double)(head-found_s)/SAMPLE_RATE, (double)found_s/SAMPLE_RATE);
                }
                found_s = 0;
            }
            head++;
        }
    }
    */
}
