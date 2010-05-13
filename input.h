
#ifndef __INPUT_H_INCLUDED__
#define __INPUT_H_INCLUDED__

#include "soundpatty.h"

class Input {
    public:
        int SAMPLE_RATE, DATA_SIZE;
        virtual buffer_t * giveInput(buffer_t * buffer) {
            fatal((void*)"giveInput not implemented, exiting\n");
            return buffer;
        };

        virtual void test() {
            printf("Called Input\n");
        }
    protected:
        SoundPatty * _sp_inst;
};

class WavInput : public Input {
    public:
        buffer_t * giveInput(buffer_t *buf_prop);
        WavInput(SoundPatty * inst, const void * args);
    protected:
        int process_headers(const char * infile);
        bool check_sample (const char * sample, const char * b);
    private:
        FILE *_fh;
};

class JackInput : public Input {
    public:
        jack_client_t * client;
        char *src_port;
        const char *dst_port;

        JackInput(SoundPatty * inst, const void * args);
};

#endif //__INPUT_H_INCLUDED__
