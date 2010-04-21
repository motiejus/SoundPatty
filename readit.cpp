/*  
 *  readit.cpp
 *
 *  Copyright (c) 2010 Motiejus Jakštys
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


/*
 * Only 16 bit PCM MONO supported now.
 */
#include <math.h>
#include <algorithm>
#include <stdlib.h>
#include <cstdio>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>

#define BUFFERSIZE 1

using namespace std;

FILE * process_headers(const char*);
bool check_sample(const char*, const char*);
void dump_values(FILE * in, void (*callback)(int, float, float, int), float timeout);
void read_periods(const char*);
void do_checking (int, float, float, int);
void read_cfg(const char*);
void fatal (const char* );
vector<string> explode(const string&, const string&); // Found somewhere on NET

char errmsg[200];   // Temporary message
int SAMPLE_RATE,    // 
    WAVE,           // SAMPLE_RATE*MINWAVEFREQ
    CHUNKSIZE,      // CHUNKLEN*SAMPLE_RATE
    DATA_SIZE;      // Data chunk size in bytes

map<string, float> cfg; // OK, ok... Is it possible to store any value but string (Rrrr...) here?

class Range {
    public:
        Range() { tm = tmin = tmax = 0; };
        Range(const float & a) { tm = a; tmin = a * 0.9; tmax = a * 1.1; }
        Range(const float & tmin, const float &tm, const float &tmax) { this->tmin = tmin; this->tm = tm; this->tmax = tmax; }
        Range(const float & tmin, const float &tmax) { this->tmin = tmin; this->tmax = tmax; }
        Range operator = (const float i) { return Range(i); }
        bool operator == (const float & i) const { return tmin < i && i < tmax; }
        bool operator == (const Range & r) const { return r == tm; }
        bool operator > (const float & i) const { return i > tmax; }
        bool operator > (const Range & r) const { return r > tm; }
        bool operator < (const float & i) const { return i < tmin; }
        bool operator < (const Range & r) const { return r < tm; }
        float tmin, tm, tmax;
};

struct CrapRange {
    int head, tail, min, max;
    bool proc;
};
vector<CrapRange> ranges;

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

void its_over(workitm * w, float place) {
    printf("FOUND, processed %.6f sec\n", place);
    /*
       printf("Found a matching pattern! Length: %d, here comes the trace:\n", (int)w->trace.size());
       for (list<pair<int,int> >::iterator tr = w->trace.begin(); tr != w->trace.end(); tr++) {
       printf("%d %d\n", tr->first, tr->second);
       }
     */
    exit(0);
}

struct valsitm {
    int c; // Counter in map
    float place;
};
typedef multimap<pair<int, Range>, valsitm> tvals;
tvals vals;
list<workitm> work;

void dump_out(int w, float place, float len, int noop) {
    printf ("%d;%.6f;%.6f\n", w, place, len);
}


int main (int argc, char *argv[]) {
    if (argc < 3) {
        fatal ("Usage: ./readit config.cfg sample.wav\n\nor\n"
                "./readit config.cfg samplefile.txt catchable.wav");
    }
    read_cfg(argv[1]);
    // ------------------------------------------------------------
    // Only header given, we just dump out the silence values
    //
    if (argc == 3) {
        FILE * in = process_headers(argv[2]);
        dump_values(in, dump_out, cfg["sampletimeout"]);
    }
    // ------------------------------------------------------------
    // We have values file and a new samplefile. Let's rock
    //
    else if (argc == 4) {

        ifstream file;
        file.open(argv[2]);
        string line;
        for (int i = 0; !file.eof(); i++) {
            getline(file, line);
            if (line.size() == 0) break;
            vector<string> numbers = explode(";", line);
            istringstream num(numbers[0]);
            istringstream place(numbers[1]);
            istringstream range(numbers[2]);

            float tmp2;
            pair<pair<int, Range>,valsitm > tmp;
            num >> tmp2; tmp.first.first = tmp2; // Index in ranges
            range >> tmp2; tmp.first.second = Range(tmp2);
            place >> tmp.second.place; // Place in the stream
            tmp.second.c = i; // Counter in the stream
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
        float treshold1 = cfg["treshold1"];
        int min_silence = (int)((1 << 15) * treshold1), // Below this value we have "silence, pshhh..."
            found_s = 0,
            b = -1, // This is the found silence counter (starting with zero)
            head = 0;

        // ------------------------------------------------------------
        // Print vals<r Range, int number>
        //
        dump_values(an, do_checking, cfg["catchtimeout"]);
        printf("NOT FOUND\n");
    }
    exit(0);
}

// --------------------------------------------------------------------------------
// This gets called every time there is a treshold found.
// Work array is global
//
// int b - index of silence found
// float place - place of silence from the stream start
// float sec - length of a found silence
//
bool mygreater(pair<int,Range> a, pair<int,Range> b) {
    //return (a.first < b.first && a.second < b.second);
    if (a.first < b.first) {
        return a.second < b.second;
    }
    return false;
}
void do_checking (int r, float place, float sec, int b) {

    //pair<tvals::iterator, tvals::iterator> pa = vals.equal_range(pair<int,float>(r,sec));
    // Manually searching for matching values because with that pairs equal_range doesnt work
    // Let's iterate through pa

    tvals fina; // FoundInA
    Range demorange(sec);
    pair<int,Range> sample(r,demorange);
    for (tvals::iterator it1 = vals.begin(); it1 != vals.end(); it1++) {
        if (it1->first == sample)
            fina.insert(*it1);
    }
    //------------------------------------------------------------
    // We put a indexes here that we use for continued threads
    // (we don't want to create a new "thread" with already
    // used length of a silence)
    //
    set<int> used_a; 

    //------------------------------------------------------------
    // Iterating through samples that match the found silence
    //
    for (tvals::iterator in_a = fina.begin(); in_a != fina.end(); in_a++)
    //for (tvals::iterator in_a = pa.first; in_a != pa.second; in_a++)
    {
        //printf("%d %.6f matches %.6f (%d)\n", in_a->first.first, sec, in_a->first.second.tm, in_a->second.c);
        int a = in_a->second.c;
        //------------------------------------------------------------
        // Check if it exists in our work array
        //
        for (list<workitm>::iterator w = work.begin(); w != work.end();) {
            if (b - w->b > round(cfg["maxsteps"])) {
                work.erase(w); w = work.begin(); continue;
            }
            if (b == w->b || a - w->a > round(cfg["maxsteps"]) || w->a >= a) { w++; continue; }
            // ------------------------------------------------------------
            // We fit the "region" here. We either finished,
            // or just increasing len
            //
            w->a = a; w->b = b;
            w->trace.push_back(pair<int,int>(a,b));
            if (++(w->len) < round(cfg["matchme"])) { // Proceeding with the "thread"
                used_a.insert(a);
                //printf ("Thread expanded to %d\n", w->len);
            } else { // This means the treshold is reached
                // This hack is needed to pass pointer to an instance o_O
                its_over(&(*w), place+sec);
            }
            w++;
            // End of work iteration array
        }
        if (used_a.find(a) == used_a.end()) {
            work.push_back(workitm(a,b));
            //printf ("Pushed back %d %d\n", a,b);
        }
    }
    //printf("\n");
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
        istringstream i(line.substr(x+2));
        float tmp; i >> tmp;
        cfg[line.substr(0,x)] = tmp;
    }
    // Change cfg["treshold\d+_(min|max)"] to
    // something more compatible with CrapRange map
    CrapRange tmp;
    tmp.head = tmp.tail = tmp.max = tmp.min = tmp.proc = 0;
    ranges.assign(cfg.size(), tmp); // Make a bit more then nescesarry
    int max_index = 0; // Number of different tresholds
    for(map<string, float>::iterator C = cfg.begin(); C != cfg.end(); C++) {
        // Failed to use boost::regex :(
        if (C->first.find("treshold") == 0) {
            istringstream tmp(C->first.substr(8));
            int i; tmp >> i;
            max_index = max(max_index, i);
            if (C->first.find("_min") != -1) {
                ranges[i].min = (int)(C->second*(1<<15));
            } else {
                ranges[i].max = (int)(C->second*(1<<15));
            }
        }
    }
    ranges.assign(ranges.begin(), ranges.begin()+max_index+1); // Because [start,end), but we need all

    /*
    for(map<string,float>::iterator C = cfg.begin(); C != cfg.end(); C++) {
        printf("%s: %.6f\n", C->first.c_str(), C->second);
    }
    */

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
    WAVE = SAMPLE_RATE*cfg["minwavelen"];
    CHUNKSIZE = cfg["chunklen"]*SAMPLE_RATE;
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

void dump_values(FILE * in, void (*callback)(int, float, float, int), float timeout) {

    int counter = 0;
    short int buf [SAMPLE_RATE * BUFFERSIZE]; // Process buffer every BUFFERSIZE secs
    for (int i = 0; !feof(in);) {
        fread(buf, 2, SAMPLE_RATE * BUFFERSIZE, in);
        if (i/SAMPLE_RATE > timeout) {
            break;
        }
        for (int j = 0; j < SAMPLE_RATE * BUFFERSIZE; i++, j++) {
            int cur = abs(buf[j]);

            int r = 0; // Counter for R
            for (vector<CrapRange>::iterator R = ranges.begin(); R != ranges.end(); R++, r++) {
                if (R->min <= cur && cur <= R->max) {
                    // ------------------------------------------------------------
                    // If it's first item in this wave (proc = processing started)
                    //
                    if (!R->proc) {
                        R->tail = i;
                        R->proc = true;
                    }
                    // ------------------------------------------------------------
                    // Here we are just normally in the wave.
                    //
                    R->head = i;
                } else { // We are not in the wave
                    if (R->proc && (R->min == 0 || i - R->head > WAVE)) {
                        //------------------------------------------------------------
                        // This wave is over
                        //
                        if (i - R->tail >= CHUNKSIZE) {
                            // ------------------------------------------------------------
                            // The previous chunk is big enough to be noticed
                            //
                            callback(r, (float)R->tail/SAMPLE_RATE, (float)(R->head - R->tail)/SAMPLE_RATE, counter++);
                        } 
                        // ------------------------------------------------------------
                        // Else it is too small, but we don't want to do anything in that case
                        // So therefore we just say that wave processing is over
                        //
                        R->proc = false;
                    }
                }
            }
        }
    }
}

vector<string> explode(const string &delimiter, const string &str) // Found somewhere on NET
{
    vector<string> arr;
    int strleng = str.length();
    int delleng = delimiter.length();
    if (delleng == 0)
        return arr; //no change
    int i = 0, k = 0;
    while (i < strleng) {
        int j = 0;
        while (i+j < strleng && j < delleng && str[i+j] == delimiter[j])
            j++;
        if (j == delleng) {
            arr.push_back(str.substr(k, i-k));
            i += delleng;
            k = i;
        } else {
            i++;
        }
    }
    arr.push_back(str.substr(k, i-k));
    return arr;
};
