/*
 *  input.h
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


#ifndef __INPUT_H_INCLUDED__
#define __INPUT_H_INCLUDED__

#include "main.h"
#include "soundpatty.h"

class Input {
    public:
        int SAMPLE_RATE, DATA_SIZE;
        virtual int giveInput(buffer_t * buffer) {
            fatal((void*)"giveInput not implemented, exiting\n");
            return 0;
        };
    protected:
        SoundPatty * _sp_inst;
};

class WavInput : public Input {
    public:
        int giveInput(buffer_t *);
        WavInput(const void *, all_cfg_t *);
    protected:
        int process_headers(const char * infile, all_cfg_t *);
        bool check_sample (const char * sample, const char * b);
    private:
        FILE *_fh;
};

class JackInput : public Input {
    public:
        static int jack_proc(jack_nframes_t nframes, void *arg);
        static jack_client_t *get_client();
        int giveInput(buffer_t *buf_prop);
        char *src_port_name;
        string dst_port_name;

        JackInput(const void * args, all_cfg_t *cfg);
        pthread_mutex_t data_mutex;
        pthread_cond_t  condition_cond;
        static jack_client_t *_client;
    private:
        jack_port_t * dst_port, * src_port;
        list<buffer_t> data_in;
};

jack_client_t *JackInput::_client;

pthread_mutex_t jackInputsMutex;
list<JackInput*> jackInputs;

#endif //__INPUT_H_INCLUDED__
