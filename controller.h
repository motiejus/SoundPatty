#ifndef __SP_CONTROLLER_H_INCLUDED__
#define __SP_CONTROLLER_H_INCLUDED__

#include <stdlib.h>
#include <stdio.h>

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <jack/jack.h>

using namespace std;

#define SP_PATH "./"
#define SP_EXEC SP_PATH "main"
#define SP_CONF SP_PATH "config.cfg"
#define SP_TRES SP_PATH "samplefile.txt"
#define SP_OVER SP_PATH "over.sh"

jack_client_t *client;
list<jack_port_t*> p_queue;

map<const char*, pthread_t> sps; // SoundPatty instances

pthread_mutex_t p_queue_mutex;
pthread_cond_t p_queue_cond;

void *go_sp(void *port_name);
void new_port(const jack_port_id_t port_id, int registerr, void *arg);
void fatal(void * r);
void fatal(const char * msg);

#endif //__SP_CONTROLLER_H_INCLUDED__
