/*  
 *  types.h
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

#ifndef __TYPES_H_INCLUDED__
#define __TYPES_H_INCLUDED__

#ifdef HAS_JACK
#define nframes_t jack_nframes_t
#define sample_t jack_audio_sample_t
#else
#define nframes_t unsigned int
#define sample_t float
#endif

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
	sample_t min, max;
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

typedef struct {
    sample_t * buf;
    nframes_t nframes;
} buffer_t;


struct sp_params_dump_t {
};

struct sp_params_capture_t {
    int exit_after_capture;
    vals_t vals;
    void (*fn)(const char*, const double);
};

enum action_t { ACTION_UNDEF = -1, ACTION_CAPTURE = 0, ACTION_DUMP = 1, ACTION_SHOWDRV = 2 };

#endif // __TYPES_H_INCLUDED__
