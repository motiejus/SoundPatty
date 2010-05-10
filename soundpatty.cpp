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
#define ACTION_WRITE 1

#define ACTION_FN_ARGS int w, double place, double len, unsigned long int b
#define ACTION_FN(func) void (*func)(ACTION_FN_ARGS)

void fatal(void * r) {
    char * msg = (char*) r;
    printf (msg);
    exit (1);
}
char errmsg[200];   // Temporary message

struct sVolumes {
    unsigned long head, tail;
	jack_default_audio_sample_t min, max;
    bool proc;
};

class Input {
    public:
        int SAMPLE_RATE;
};
class SoundPatty {
    public:
        SoundPatty(const char * fn) { 
            read_cfg(fn);
        };
        map<string, double> cfg;
        virtual void Error(void*);
        int setInput(int, void*);
        int setAction(int, ACTION_FN(callback));
        int go();
        int WAVE, CHUNKSIZE;
    private:
        Input * _input;
        int read_cfg(const char*);
        int source_app, action;
        char *input_params;
        ACTION_FN(action_fn);
        int SAMPLE_RATE, DATA_SIZE;
        vector<sVolumes> volume;
};
class WavInput : public Input {
    public:
        WavInput(SoundPatty * inst, void * args) {
            _sp_inst = inst;
            char * filename = (char*) args;
            process_headers(filename);
        }
    protected:
        SoundPatty * _sp_inst;
        int process_headers(const char * infile) {/*{{{*/

            FILE * _fh = fopen(infile, "rb");
            if (_fh == NULL) {
                sprintf(errmsg, "Failed to open file %s, exiting\n", infile);
                fatal((void*)errmsg);
            }

            // Read bytes [0-3] and [8-11], they should be "RIFF" and "WAVE" in ASCII
            char header[5];
            fread(header, 1, 4, _fh);

            char sample[] = "RIFF";
            if (! check_sample(sample, header) ) {
                sprintf(errmsg, "RIFF header not found in %s, exiting\n", infile);
                fatal((void*)errmsg);
            }
            // Checking for compression code (21'st byte is 01, 22'nd - 00, little-endian notation
            uint16_t tmp[2]; // two-byte integer
            fseek(_fh, 0x14, 0); // offset = 20 bytes
            fread(&tmp, 2, 2, _fh); // Reading two two-byte samples (comp. code and no. of channels)
            if ( tmp[0] != 1 ) {
                sprintf(errmsg, "Only PCM(1) supported, compression code given: %d\n", tmp[0]);
                fatal ((void*)errmsg);
            }
            // Number of channels must be "MONO"
            if ( tmp[1] != 1 ) {
                sprintf(errmsg, "Only MONO supported, given number of channels: %d\n", tmp[1]);
                fatal ((void*)errmsg);
            }
            fread(&SAMPLE_RATE, 2, 1, _fh); // Single two-byte sample
            _sp_inst->WAVE = (int)SAMPLE_RATE * _sp_inst->cfg["minwavelen"];
            _sp_inst->CHUNKSIZE = _sp_inst->cfg["chunklen"] * (int)SAMPLE_RATE;
            fseek(_fh, 0x22, 0);
            uint16_t BitsPerSample;
            fread(&BitsPerSample, 1, 2, _fh);
            if (BitsPerSample != 16) {
                sprintf(errmsg, "Only 16-bit WAVs supported, given: %d\n", BitsPerSample);
                fatal((void*)errmsg);
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
void SoundPatty::Error(void * msg) {/*{{{*/
    char * mesg = (char*) msg;
    printf(mesg);
    exit(0);
}/*}}}*/
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

int SoundPatty::setInput(const int source_app, void * input_params) {
    if (0 <= source_app && source_app <= 2) {
        this->source_app = source_app;
    }
    switch(this->source_app) {
        case SRC_WAV:
            Input * _input = new WavInput(this, input_params);
            break;
    }
    return 0;
}

int SoundPatty::setAction(const int action, ACTION_FN(callback)) {
    action_fn = callback;
    if (0 <= action && action <= 1) {
        this->action = action;
        return 0;
    }
    return 1;
}

void dump_out(ACTION_FN_ARGS) {
    printf ("%d;%.6f;%.6f\n", w, place, len);
}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        fatal ((void*)"Usage: ./readit config.cfg sample.wav\nor\n"
                "./readit config.cfg samplefile.txt catchable.wav\n"
                "./readit config.cfg samplefile.txt jack jack\n");
    }
    if (argc == 3) {
        SoundPatty * pat = new SoundPatty(argv[1]); // usually config.cfg
        pat->setInput(SRC_WAV, argv[2]);
        pat->setAction(ACTION_WRITE, dump_out);
    }
    exit(0);
}
