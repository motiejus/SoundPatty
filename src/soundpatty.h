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

class Input;
class SoundPatty {
    public:
        SoundPatty(action_t, Input*, all_cfg_t*, void *);
        /*
        SoundPatty(const char*, Input *, all_cfg_t *);
        SoundPatty(const char * name, Input *input, all_cfg_t *all_cfg, vals_t vals,
                       void(*fn)(const char*, const double));
                       */
        static all_cfg_t read_cfg(const char*);
        static vals_t read_captured_values(const char *);
        void setAction(int action, void (*fn)(const char*, const double));
        static void dump_out(const treshold_t args);
        static void *go_thread(void *inst); // Same as SoundPatty::go(), but for pthreads
        int do_checking(const treshold_t args);
        map<string, double> cfg;
        int setInput(Input*);
        int go();
        unsigned long gMCounter, // How many matches we found
                      gSCounter; // How many samples we skipped
        int search_patterns (sample_t cur, treshold_t *);
        vector<sVolumes> volume;
        deque<treshold_t> findings;
        void (*_callback)(const char*, const double);
    private:
        action_t _action;
        vals_t vals;
        list<workitm> work;
        Input * _input;
        char *input_params;
        int exit_after_capture;
};

vector<string> explode(const string &delimiter, const string &str);
void fatal(void * r);

void fatal(char * msg);

#endif //__SOUNDPATTY_H_INCLUDED__
