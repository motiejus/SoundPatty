/*
 *  jackinput.h
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


#ifndef __JACKINPUT_H_INCLUDED__
#define __JACKINPUT_H_INCLUDED__

#include "input.h"
#ifdef HAVE_JACK
#include <jack/jack.h>
#endif // HAVE_JACK

class JackInput : public Input {
    public:
        ~JackInput();
        static int jack_proc(nframes_t nframes, void *arg);
        static int number_of_clients; // Leave this for jackInput
        static void port_connect(jack_port_id_t, jack_port_id_t, int, void *);

        jack_client_t* get_client();
        int giveInput(buffer_t *buf_prop);
        char *src_port_name;

        JackInput(const void * args, all_cfg_t *cfg);
        pthread_mutex_t data_mutex;
        pthread_cond_t  condition_cond;
        //static JackInput *jack_inst;
        //static jack_client_t *_client;

        static void monitor_ports(all_cfg_t *);

        static jack_client_t *client;
        static list<jack_port_t*> p_queue;
        static pthread_mutex_t p_queue_mutex;
        static pthread_cond_t p_queue_cond;
    private:
        jack_port_t *dst_port, *src_port;
        string dst_port_name;
        list<buffer_t> data_in;
};

extern jack_client_t * mainclient;
extern pthread_mutex_t jackInputsMutex;
extern map<string, JackInput*> jackInputs; // DST port full name, jackInput instance

#endif //__JACKINPUT_H_INCLUDED__
