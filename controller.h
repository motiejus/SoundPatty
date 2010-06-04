#ifndef __SP_CONTROLLER_H_INCLUDED__
#define __SP_CONTROLLER_H_INCLUDED__


#include "main.h"
#include "soundpatty.h"
#include "input.h"

#define SP_PATH "./"
#define SP_CONF SP_PATH "config.cfg"
#define SP_TRES SP_PATH "samplefile.txt"
#define SP_OVER SP_PATH "over.sh"

jack_client_t *client;
list<jack_port_t*> p_queue;

map<const char*, pthread_t> sps; // SoundPatty instances

pthread_mutex_t p_queue_mutex;
pthread_cond_t p_queue_cond;

void *go_sp(void *port_name);
void its_over(double place);
void new_port(const jack_port_id_t port_id, int registerr, void *arg);

#endif //__SP_CONTROLLER_H_INCLUDED__
