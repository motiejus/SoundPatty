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
#include <jack/jack.h>

using namespace std;

#define SRC_WAV 0
#define SRC_JACK_ONE 1
#define SRC_JACK_AUTO 2
#define ACTION_DUMP 0
#define ACTION_CATCH 1

#define BUFFERSIZE 1


void fatal(void * r) {
    char * msg = (char*) r;
    printf (msg);
    exit (1);
};
void fatal(char * msg) {
    printf (msg);
    exit (1);
}

void its_over(double place) {
    printf("FOUND, processed %.6f sec\n", place);
    exit(0);
}
class Range {/*{{{*/
    public:
        Range() { tm = tmin = tmax = 0; };
        Range(const double & a) { tm = a; tmin = a * 0.89; tmax = a * 1.11; }
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
typedef struct {
    jack_default_audio_sample_t * buf;
    jack_nframes_t nframes;
} buffer;

class workitm {
    public:
        int len,a;
        unsigned long b;

        workitm(int, unsigned long);
        list<pair<int, unsigned long> > trace;
};

workitm::workitm(const int a, const unsigned long b) {
    this->a = a; this->b = b;
    len = 0;
    trace.push_back(pair<int,unsigned long>(a,b));
};

struct sVolumes {
    unsigned long head, tail;
	jack_default_audio_sample_t min, max;
    bool proc;
};

struct valsitm_t {
    int c; // Counter in map
    double place;
};
struct treshold_t {
    int r;
    double place, sec;
    unsigned long b;
};

typedef multimap<pair<int, Range>, valsitm_t> vals_t;/*}}}*/

class SoundPatty;
class Input {
    public:
        int SAMPLE_RATE, DATA_SIZE;
        virtual buffer * giveInput(buffer *) {
            fatal((void*)"This should never be called!!!");
        };
        virtual void test() {
            printf("Called Input\n");
        }
    protected:
        SoundPatty * _sp_inst;
};

vector<string> explode(const string &delimiter, const string &str); 

class SoundPatty {
    public:
        void read_captured_values(const char *);
        SoundPatty(const char * fn) { 
            gSCounter = gMCounter = 0;
            read_cfg(fn);
        };
        void setAction(int action) {
            _action = action;
        }
        void setAction(int action, char * cfg, void (*fn)(double)) {
            _action = action;
            read_captured_values(cfg);
            _callback = fn; // What to do when we catch a pattern
        }
        static void dump_out(const treshold_t args) {/*{{{*/
            printf ("%d;%.6f;%.6f\n", args.r, args.place, args.sec);
        }/*}}}*/
        treshold_t * do_checking(const treshold_t args);
        map<string, double> cfg;
        int setInput(int, void*);
        int go();
        int WAVE, CHUNKSIZE;
        unsigned long gMCounter, // How many matches we found
                      gSCounter; // How many samples we skipped
        int search_patterns (jack_default_audio_sample_t cur, treshold_t *);
        vector<sVolumes> volume;
    private:
        int _action;
        void (*_callback)(double);
        list<workitm> work;
        vals_t vals;
        Input * _input;
        int read_cfg(const char*);
        int source_app;
        char *input_params;
};

class WavInput : public Input {
    public:
        WavInput(SoundPatty * inst, const void * args) {
            _sp_inst = inst;
            char * filename = (char*) args;
            process_headers(filename);
            // ------------------------------------------------------------
            // Adjust _sp_ volume 
            // Jack has float (-1..1) while M$ WAV has (-2^15..2^15)
            //
            for (vector<sVolumes>::iterator vol = _sp_inst->volume.begin(); vol != _sp_inst->volume.end(); vol++) {
                vol->min *= (1<<15);
                vol->max *= (1<<15);
            }
        }

        buffer * giveInput(buffer *buf_prop) {
            int16_t buf [SAMPLE_RATE * BUFFERSIZE]; // Process buffer every BUFFERSIZE secs
            if (feof(_fh)) {
                return NULL;
            }
            size_t read_size = fread(buf, 2, SAMPLE_RATE * BUFFERSIZE, _fh);

            // :HACK: fix this, do not depend on jack types where you don't need!
            jack_default_audio_sample_t buf2 [SAMPLE_RATE * BUFFERSIZE];
            for(int i = 0; i < read_size; i++) {
                buf2[i] = (jack_default_audio_sample_t)buf[i];
            }
            buf_prop->buf = buf2;
            buf_prop->nframes = read_size;
            return buf_prop;
        }
    protected:
        int process_headers(const char * infile) {/*{{{*/
            char errmsg[200];

            _fh = fopen(infile, "rb");
            if (_fh == NULL) {
                sprintf(errmsg, "Failed to open file %s, exiting\n", infile);
                fatal(errmsg);
            }

            // Read bytes [0-3] and [8-11], they should be "RIFF" and "WAVE" in ASCII
            char header[5];
            fread(header, 1, 4, _fh);

            char sample[] = "RIFF";
            if (! check_sample(sample, header) ) {
                sprintf(errmsg, "RIFF header not found in %s, exiting\n", infile);
                fatal(errmsg);
            }
            // Checking for compression code (21'st byte is 01, 22'nd - 00, little-endian notation
            uint16_t tmp[2]; // two-byte integer
            fseek(_fh, 0x14, 0); // offset = 20 bytes
            fread(&tmp, 2, 2, _fh); // Reading two two-byte samples (comp. code and no. of channels)
            if ( tmp[0] != 1 ) {
                sprintf(errmsg, "Only PCM(1) supported, compression code given: %d\n", tmp[0]);
                fatal (errmsg);
            }
            // Number of channels must be "MONO"
            if ( tmp[1] != 1 ) {
                sprintf(errmsg, "Only MONO supported, given number of channels: %d\n", tmp[1]);
                fatal (errmsg);
            }
            fread(&SAMPLE_RATE, 2, 1, _fh); // Single two-byte sample
            _sp_inst->WAVE = (int)SAMPLE_RATE * _sp_inst->cfg["minwavelen"];
            _sp_inst->CHUNKSIZE = _sp_inst->cfg["chunklen"] * (int)SAMPLE_RATE;
            fseek(_fh, 0x22, 0);
            uint16_t BitsPerSample;
            fread(&BitsPerSample, 1, 2, _fh);
            if (BitsPerSample != 16) {
                sprintf(errmsg, "Only 16-bit WAVs supported, given: %d\n", BitsPerSample);
                fatal(errmsg);
            }
            // Get data chunk size here
            fread(header, 1, 4, _fh);
            strcpy(sample, "data");

            if (! check_sample(sample, header)) {
                fatal ((void*)"data chunk not found in byte offset=36, file corrupted.");
            }
            int DATA_SIZE;
            fread(&DATA_SIZE, 4, 1, _fh); // Single two-byte sample
            return 0;
        }

        bool check_sample (const char * sample, const char * b) { // My STRCPY.
            for(int i = 0; sample[i] != '\0'; i++) {
                if (sample[i] != b[i]) {
                    return false;
                }
            }
            return true;
        }/*}}}*/
    private:
        FILE *_fh;

};

class JackInput : public Input {
    public:
        bool single_chan;
        char * chan_name;
        JackInput(SoundPatty * inst, const bool single_chan, const void * args) {
            // single_chan = (true|false) must be set before calling this function
            char * chan_name = (char*) args;
            this->single_chan = single_chan;
            // Open up a port
            if (single_chan) {
            }
        }
};

int SoundPatty::read_cfg (const char * filename) {/*{{{*/
    ifstream file;
    file.open(filename);
    string line;
    int x;
    while (! file.eof() ) {
        getline(file, line);
        x = line.find(":");
        if (x == -1) break; // Last line, exit
        istringstream i(line.substr(x+2));
        double tmp; i >> tmp;
        cfg[line.substr(0,x)] = tmp;
    }
    // Change cfg["treshold\d+_(min|max)"] to
    // something more compatible with sVolumes map
    sVolumes tmp;
    tmp.head = tmp.tail = tmp.max = tmp.min = tmp.proc = 0;
    volume.assign(cfg.size(), tmp); // Make a bit more then nescesarry
    int max_index = 0; // Number of different tresholds
    for(map<string, double>::iterator C = cfg.begin(); C != cfg.end(); C++) {
        // Failed to use boost::regex :(
        if (C->first.find("treshold") == 0) {
            istringstream tmp(C->first.substr(8));
            int i; tmp >> i;
            max_index = max(max_index, i);
            // C->second and volume[i].m{in,ax} are double
            if (C->first.find("_min") != -1) {
                volume[i].min = C->second;
            } else {
                volume[i].max = C->second;
            }
        }
    }
    volume.assign(volume.begin(), volume.begin()+max_index+1);
    return 0;
}/*}}}*/
int SoundPatty::setInput(const int source_app, void * input_params) {/*{{{*/
    if (0 <= source_app && source_app <= 2) {
        this->source_app = source_app;
    }
    switch(this->source_app) {
        case SRC_WAV:
            _input = new WavInput(this, input_params);
            break;
        case SRC_JACK_ONE:
            _input = new JackInput(this, true, input_params);
            break;
        case SRC_JACK_AUTO:
            _input = new JackInput(this, false, input_params);
            break;
    }
    return 0;
}/*}}}*/
int SoundPatty::go() {
    string which_timeout (_action == ACTION_DUMP ? "sampletimeout" : "catchtimeout");
    buffer buf;

    while (_input->giveInput(&buf) != NULL) { // Have pointer to data
        treshold_t ret;

        for (int i = 0; i < buf.nframes; gSCounter++, i++) {
            jack_default_audio_sample_t cur = buf.buf[i]<0?-buf.buf[i]:buf.buf[i];
            if (search_patterns(cur, &ret))
            {
                if (_action == ACTION_DUMP) {
                    SoundPatty::dump_out(ret);
                }
                if (_action == ACTION_CATCH) {
                    SoundPatty::do_checking(ret);
                    //SoundPatty::dump_out(ret);
                }
            }
        }


        if ((double)gSCounter/_input->SAMPLE_RATE > cfg[which_timeout]) {
            //printf ("Timed out. Seconds passed: %.6f\n", (double)gSCounter/_input->SAMPLE_RATE);
            return 0;
        }
    }
}

void SoundPatty::read_captured_values(const char * filename) {/*{{{*/
        ifstream file;
        file.open(filename);
        string line;
        for (int i = 0; !file.eof(); i++) {
            getline(file, line);
            if (line.size() == 0) break;
            vector<string> numbers = explode(";", line);
            istringstream num(numbers[0]);
            istringstream place(numbers[1]);
            istringstream range(numbers[2]);

            double tmp2;
            pair<pair<int, Range>,valsitm_t > tmp;
            num >> tmp2; tmp.first.first = tmp2; // Index in volume
            range >> tmp2; tmp.first.second = Range(tmp2);
            place >> tmp.second.place; // Place in the stream
            tmp.second.c = i; // Counter in the stream
            vals.insert(tmp);
        }
}/*}}}*/
int SoundPatty::search_patterns (jack_default_audio_sample_t cur, treshold_t * ret) {/*{{{*/
    int v = 0; // Counter for volume
    for (vector<sVolumes>::iterator V = volume.begin(); V != volume.end(); V++, v++) {
        if (V->min <= cur && cur <= V->max) {
            // ------------------------------------------------------------
            // If it's first item in this wave (proc = processing started)
            //
            if (!V->proc) {
                V->tail = gSCounter;
                V->proc = true;
            }
            // ------------------------------------------------------------
            // Here we are just normally in the wave.
            //
            V->head = gSCounter;
        } else { // We are not in the wave
            if (V->proc && (V->min < 0.001 || gSCounter - V->head > WAVE)) {

                //------------------------------------------------------------
                // This wave is over
                //
                V->proc = false; // Stop processing for both cases: found and not

                if (gSCounter - V->tail >= CHUNKSIZE) {
                    // ------------------------------------------------------------
                    // The previous chunk is big enough to be noticed
                    //
                    ret -> r = v;
                    ret -> place = (double)V->tail/_input->SAMPLE_RATE;
                    ret -> sec = (double)(V->head - V->tail)/_input->SAMPLE_RATE;
                    ret -> b = gMCounter++;
                    //callback(v, (double)V->tail/_input->SAMPLE_RATE, (double)(V->head - V->tail)/_input->SAMPLE_RATE, gMCounter++);
                    return 1;
                } 
                // ------------------------------------------------------------
                // Else it is too small, but we don't want to do anything in that case
                // So therefore we just say that wave processing is over
                //
            }
        }
    }
    return 0;
}/*}}}*/
// --------------------------------------------------------------------------------/*{{{*/
// This gets called every time there is a treshold found.
// Work array is global
//
// int r - id of treshold found (in config.cfg: treshold_(<?r>\d)_(min|max))
// double place - place of sample from the stream start (sec)
// double sec - length of a found sample (sec)
// int b - index (overall) of sample found
///*}}}*/
treshold_t * SoundPatty::do_checking(const treshold_t tr) {/*{{{*/

    //pair<vals_t::iterator, vals_t::iterator> pa = vals.equal_range(pair<int,double>(r,sec));
    // Manually searching for matching values because with that pairs equal_range doesnt work
    // Iterate through pa

    int b = tr.b; vals_t fina; // FoundInA
    Range demorange(tr.sec);
    pair<int,Range> sample(tr.r, demorange);
    for (vals_t::iterator it1 = vals.begin(); it1 != vals.end(); it1++) {
        if (it1->first == sample) {
            fina.insert(*it1);
        }
    }
    //------------------------------------------------------------
    // We put a indexes here that we use for continued threads
    // (we don't want to create a new "thread" with already
    // used length of a sample)
    //
    set<int> used_a; 

    //------------------------------------------------------------
    // Iterating through samples that match the found sample
    //
    for (vals_t::iterator in_a = fina.begin(); in_a != fina.end(); in_a++)
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
            w->trace.push_back(pair<int,unsigned long>(a,b));
            if (++(w->len) < round(cfg["matchme"])) { // Proceeding with the "thread"
                used_a.insert(a);
                //printf ("Thread expanded to %d\n", w->len);
            } else { // This means the treshold is reached

                // This kind of function is called when the pattern is recognized
                //void(*end_fn)(workitm *, double) = (void*)(workitm *, double) _callback;
                _callback (tr.place + tr.sec);
            }
            w++;
            // End of work iteration array
        }
        if (used_a.find(a) == used_a.end()) {
            work.push_back(workitm(a,b));
            //printf ("Pushed back %d %d\n", a,b);
        }
    }
}/*}}}*/
vector<string> explode(const string &delimiter, const string &str) { // Found somewhere on NET/*{{{*/

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
};/*}}}*/

int main (int argc, char *argv[]) {
    if (argc < 3) {
        fatal ((void*)"Usage: ./readit config.cfg sample.wav\nor\n"
                "./readit config.cfg samplefile.txt catchable.wav\n"
                "./readit config.cfg samplefile.txt jack jack\n");
    }
    SoundPatty * pat = new SoundPatty(argv[1]); // usually config.cfg
    if (argc == 3 || argc == 4) { // WAV
        pat->setInput(SRC_WAV, argv[argc - 1]);
    }
    if (argc == 3) { // Dump out via WAV
        pat->setAction(ACTION_DUMP);
    }
    if (argc == 4) { // Catch
        pat->setAction(ACTION_CATCH, argv[2], its_over);
    }
    pat->go();

    exit(0);
}
