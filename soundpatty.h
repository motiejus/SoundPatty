
#ifndef __SOUNDPATTY_H_INCLUDED__
#define __SOUNDPATTY_H_INCLUDED__

#include "main.h"

struct treshold_t {
    int r;
    double place, sec;
    unsigned long b;
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

class Input;
class SoundPatty {
    public:
        void read_captured_values(const char *);
        SoundPatty(const char * fn);
        void setAction(int action);
        void setAction(int action, char * cfg, void (*fn)(double));
        static void dump_out(const treshold_t args);
        void do_checking(const treshold_t args);
        map<string, double> cfg;
        int setInput(int, void*);
        void go();
        unsigned int WAVE, CHUNKSIZE;
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


vector<string> explode(const string &delimiter, const string &str);
void fatal(void * r);

void fatal(char * msg);

typedef struct {
    jack_default_audio_sample_t * buf;
    jack_nframes_t nframes;
} buffer_t;

#endif //__SOUNDPATTY_H_INCLUDED__
