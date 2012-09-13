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

        JackInput(const char*, all_cfg_t *cfg);
        pthread_mutex_t data_mutex;
        pthread_cond_t  condition_cond;

        static void monitor_ports(action_t, const char*, all_cfg_t *, void *);

        static jack_client_t *client;

        // bool is true if port connects, false - disconects
        static list<pair<jack_port_t*, bool> > p_queue;
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
