#ifndef __INPUT_H_INCLUDED__
#define __INPUT_H_INCLUDED__

#include <pthread.h>
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
        int giveInput(buffer_t *buf_prop);
        WavInput(SoundPatty * inst, const void * args);
    protected:
        int process_headers(const char * infile);
        bool check_sample (const char * sample, const char * b);
    private:
        FILE *_fh;
};

class JackInput : public Input {
    public:
        static int jack_proc(jack_nframes_t nframes, void *arg);
        int giveInput(buffer_t *buf_prop);
        jack_client_t * client;
        char *src_port_name;
        string dst_port_name;

        JackInput(SoundPatty * inst, const void * args);
    private:
        jack_port_t * dst_port, * src_port;
        pthread_mutex_t data_mutex;
        pthread_cond_t  condition_cond;

        list<buffer_t> data_in;
};

#endif //__INPUT_H_INCLUDED__
