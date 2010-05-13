/*
 *  soundpatty.cpp
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


#include "soundpatty.h"
#include "input.h"

void SoundPatty::dump_out(const treshold_t args) { // STATIC
    printf ("%d;%.6f;%.6f\n", args.r, args.place, args.sec);
};


void SoundPatty::setAction(int action, char * cfg, void (*fn)(double)) {
    _action = action;
    read_captured_values(cfg);
    _callback = fn; // What to do when we catch a pattern
};


void SoundPatty::setAction(int action) {
    _action = action;
};


SoundPatty::SoundPatty(const char * fn) { 
    gSCounter = gMCounter = 0;
    read_cfg(fn);
};


int SoundPatty::read_cfg (const char * filename) {
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
            if (C->first.find("_min") != string::npos) {
                volume[i].min = C->second;
            } else {
                volume[i].max = C->second;
            }
        }
    }
    volume.assign(volume.begin(), volume.begin()+max_index+1);
    return 0;
};


int SoundPatty::setInput(const int source_app, void * input_params) {
    if (0 <= source_app && source_app <= 2) {
        this->source_app = source_app;
    }
    switch(this->source_app) {
        case SRC_WAV:
            _input = new WavInput(this, input_params);
            break;
        case SRC_JACK_ONE:
            _input = new JackInput(this, input_params);
            break;
    }
    return 0;
};


void SoundPatty::go() {
    string which_timeout (_action == ACTION_DUMP ? "sampletimeout" : "catchtimeout");
    buffer_t buf;

    while (_input->giveInput(&buf) != 0) { // Have pointer to data
        treshold_t ret;

        for (unsigned int i = 0; i < buf.nframes; gSCounter++, i++) {
            jack_default_audio_sample_t cur = buf.buf[i]<0?-buf.buf[i]:buf.buf[i];
            if (search_patterns(cur, &ret))
            {
                if (_action == ACTION_DUMP) {
                    SoundPatty::dump_out(ret);
                }
                if (_action == ACTION_CATCH) {
                    SoundPatty::do_checking(ret);
                }
            }
        }

        if ((double)gSCounter/_input->SAMPLE_RATE > cfg[which_timeout]) {
            printf ("Timed out. Seconds passed: %.6f\n", (double)gSCounter/_input->SAMPLE_RATE);
            return;
        }
    }
};


void SoundPatty::read_captured_values(const char * filename) {
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
};


int SoundPatty::search_patterns (jack_default_audio_sample_t cur, treshold_t * ret) {
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
};


// --------------------------------------------------------------------------------
// This gets called every time there is a treshold found.
// Work array is global
//
// int r - id of treshold found (in config.cfg: treshold_(<?r>\d)_(min|max))
// double place - place of sample from the stream start (sec)
// double sec - length of a found sample (sec)
// int b - index (overall) of sample found
//
void SoundPatty::do_checking(const treshold_t tr) {

    //pair<vals_t::iterator, vals_t::iterator> pa = vals.equal_range(pair<int,double>(r,sec));
    // Manually searching for matching values because with that pairs equal_range doesnt work
    // Iterate through pa

    unsigned long b = tr.b; vals_t fina; // FoundInA
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
};


vector<string> explode(const string &delimiter, const string &str) { // Found somewhere on NET

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


void fatal(void * r) {
    char * msg = (char*) r;
    printf (msg);
    exit (1);
};


void fatal(char * msg) {
    printf (msg);
    exit (1);
};


workitm::workitm(const int a, const unsigned long b) {
    this->a = a; this->b = b;
    len = 0;
    trace.push_back(pair<int,unsigned long>(a,b));
};
