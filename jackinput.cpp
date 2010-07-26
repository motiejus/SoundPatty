/*
 *  jackinput.cpp
 *
 *  Copyright (c) 2010 Motiejus Jakštys
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.

 *  This program is distributed in the hope that it will be usefu
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "jackinput.h"

JackInput *JackInput::jack_inst = NULL;
int JackInput::number_of_clients = 0; // Counter of jack clients
// Not protected by mutex, because accessed only in one thread

jack_client_t *mainclient = NULL;
pthread_mutex_t jackInputsMutex = PTHREAD_MUTEX_INITIALIZER;
map<string,JackInput*> jackInputs;

int JackInput::jack_proc(nframes_t nframes, void *arg) {
    pthread_mutex_lock(&jackInputsMutex);
    for(map<string,JackInput*>::iterator inp = jackInputs.begin(); inp != jackInputs.end(); inp++) {
        JackInput *in_inst = inp->second;
        pthread_mutex_lock(&in_inst->data_mutex);

        buffer_t buffer;
        buffer.buf = (sample_t *) jack_port_get_buffer (in_inst->dst_port, nframes);
        buffer.nframes = nframes;
        in_inst->data_in.push_back(buffer);
        pthread_cond_signal(&in_inst->condition_cond);
        pthread_mutex_unlock(&in_inst->data_mutex);
    }
    pthread_mutex_unlock(&jackInputsMutex);
    return 0;
};

jack_client_t *JackInput::get_client() {
    string logger_name ("input.jack."); 
    logger_name += string(dst_port_name);

    bool new_client = false;
    if (mainclient == NULL) { // Jack client initialization (SINGLETON should be called)
        new_client = true;
        ostringstream dst_port_str, dst_client_str;
        dst_client_str << "sp_client_" << getpid();
        if ((mainclient = jack_client_new (string(dst_client_str.str()).c_str())) == 0) {
			LOG_FATAL("jack server not running?\n");
			exit(1);
        }
        LOG_INFO("Created new jack client %s", dst_client_str.str().c_str());
        if (jack_set_process_callback(mainclient, JackInput::jack_proc, NULL)) {
			LOG_FATAL("Failed to set process registration callback for %s", dst_client_str.str().c_str());
        }
        LOG_DEBUG("Created process callback jack_proc for %s", dst_client_str.str().c_str());
        if (jack_activate (mainclient)) { LOG_FATAL("cannot activate client"); }
        LOG_INFO("Activated jack client %s", dst_client_str.str().c_str());
    }
    if (new_client) {
        LOG_INFO("NEW Jack client requested");
    } else {
        LOG_DEBUG( "Jack client requested, returned old one");
    }
    return mainclient;
};

JackInput::JackInput(const void * args, all_cfg_t *cfg) {

    // G++ SAYS THIS HAS NO EFFECT!
    //data_in;
    pthread_mutex_init(&data_mutex, NULL);
	pthread_cond_init(&condition_cond, NULL);
    jack_client_t *client = get_client();

    char *src_port_name = (char*) args;
    src_port = jack_port_by_name(client, src_port_name);

    SAMPLE_RATE = jack_get_sample_rate (client);
    cfg->first["WAVE"] = (int)SAMPLE_RATE * cfg->first["minwavelen"];
    cfg->first["CHUNKSIZE"] = cfg->first["chunklen"] * (int)SAMPLE_RATE;

    char shortname[15];
    if (jack_port_name_size() <= 15) {
        LOG_FATAL("Too short jack port name supported");
        exit(1);
    }

    sprintf(shortname, "input_%d", ++JackInput::number_of_clients);
    dst_port = jack_port_register (client, shortname, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    dst_port_name = string(jack_port_name(dst_port));

    if (jack_connect(client, src_port_name, jack_port_name(dst_port))) {
        LOG_FATAL("Failed to connect %s to %s, exiting.\n", src_port_name, jack_port_name(dst_port));
		exit(1);
    }

    LOG_DEBUG("Locking JackInput mutex to insert instance to jackInputs");
    pthread_mutex_lock(&jackInputsMutex);
    jackInputs.insert( pair<string,JackInput*>(dst_port_name,this) );
    pthread_mutex_unlock(&jackInputsMutex);
    LOG_DEBUG("jack port to jackInputs inserted, port ready to work");
};


JackInput::~JackInput() {
    // Delete input port
    string logger_name ("input.jack."); 
    logger_name += string(dst_port_name) + ".destructor";

    LOG_DEBUG( "JackInput destructor called");

    LOG_DEBUG( "Disconnecting ports");
    if (jack_port_unregister(get_client(), dst_port)) {
        LOG_ERROR( "Failed to remove port %s", jack_port_name(src_port));
    }

    pthread_mutex_lock(&jackInputsMutex);
    jackInputs.erase(this->dst_port_name);
    pthread_mutex_unlock(&jackInputsMutex);

    LOG_DEBUG( "Deleted JackInput from jackInputs, ports are closed, leaving destructor");
}

int JackInput::giveInput(buffer_t *buffer) {
    // Create a new thread and wait for input. When get a buffer - return back.

    pthread_mutex_lock(&data_mutex);
    pthread_cond_wait(&condition_cond, &data_mutex);

    memcpy((void*)buffer, (void*)&data_in.front(), sizeof(*buffer)); // Copy all buffer data blindly
    data_in.pop_front(); // Remove the processed waiting member
    pthread_mutex_unlock(&data_mutex);
    return 1;
}
