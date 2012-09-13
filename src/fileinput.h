#ifndef __FILENPUT_H_INCLUDED__
#define __FILENPUT_H_INCLUDED__

#include "input.h"
extern "C" {
#include <sox.h>
}

class FileInput : public Input {
    public:
        ~FileInput();
        int giveInput(buffer_t *);
        FileInput(const char *, all_cfg_t *);
        static void monitor_ports(action_t, const char*, all_cfg_t*, void*);
    private:
        sox_format_t *s;
};

#endif //__FILEINPUT_H_INCLUDED__
