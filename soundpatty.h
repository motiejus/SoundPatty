/*
 *  soundpatty.h
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


#ifndef __SOUNDPATTY_H_INCLUDED__
#define __SOUNDPATTY_H_INCLUDED__

#include "main.h"

struct treshold_t {
    int r; // Volume index (from configs)
    double place, // Absolute place in file
           sec; // Length of treshold
    unsigned long b; // Number of found treshold
};

class Range {
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

struct sVolumes {
    unsigned long head, tail;
	jack_default_audio_sample_t min, max;
    bool proc;
};

struct valsitm_t {
    int c; // Counter in map
    double place;
};

typedef multimap<pair<int, Range>, valsitm_t> vals_t;

class workitm {
    public:
        int len,a;
        unsigned long b;

        workitm(int, unsigned long);
        list<pair<int, unsigned long> > trace;
};

typedef pair<map<string,double>, vector<sVolumes> > all_cfg_t;

class Input;
class SoundPatty {
    public:
        SoundPatty(const char*, Input *, all_cfg_t *);
        static all_cfg_t read_cfg(const char*);
        void read_captured_values(const char *);
        void setAction(int action);
        void setAction(int action, char * cfg, void (*fn)(const char*, const double));
        static void dump_out(const treshold_t args);
        static void *go_thread(void *inst); // Same as SoundPatty::go(), but for pthreads
        int do_checking(const treshold_t args);
        map<string, double> cfg;
        int setInput(Input*);
        int go();
        unsigned long gMCounter, // How many matches we found
                      gSCounter; // How many samples we skipped
        int search_patterns (jack_default_audio_sample_t cur, treshold_t *);
        vector<sVolumes> volume;
        char *name; // Each instance has a name
    private:
        int _action;
        void (*_callback)(const char*, const double);
        list<workitm> work;
        vals_t vals;
        Input * _input;
        char *input_params;
};

vector<string> explode(const string &delimiter, const string &str);
void fatal(void * r);

void fatal(char * msg);

typedef struct {
    jack_default_audio_sample_t * buf;
    jack_nframes_t nframes;
} buffer_t;

#endif //__SOUNDPATTY_H_INCLUDED__
