#include "jackinput.h"

//JackInput *JackInput::jack_inst = NULL;
jack_client_t *JackInput::client;
pthread_mutex_t JackInput::p_queue_mutex;
pthread_cond_t JackInput::p_queue_cond;
list<pair<jack_port_t*,bool> > JackInput::p_queue;

int JackInput::number_of_clients = 0; // Counter of jack clients
// Not protected by mutex, because accessed only in one thread

jack_client_t *mainclient = NULL;
pthread_mutex_t jackInputsMutex = PTHREAD_MUTEX_INITIALIZER;
map<string,JackInput*> jackInputs;

int JackInput::jack_proc(nframes_t nframes, void *arg) {
    pthread_mutex_lock(&jackInputsMutex);
    for(map<string,JackInput*>::iterator inp = jackInputs.begin();
            inp != jackInputs.end(); inp++) {
        JackInput *in_inst = inp->second;
        pthread_mutex_lock(&in_inst->data_mutex);

        buffer_t buffer;
        buffer.buf = (sample_t *) jack_port_get_buffer(
                in_inst->dst_port, nframes);
        buffer.nframes = nframes;
        in_inst->data_in.push_back(buffer);
        pthread_cond_signal(&in_inst->condition_cond);
        pthread_mutex_unlock(&in_inst->data_mutex);
    }
    pthread_mutex_unlock(&jackInputsMutex);
    return 0;
}

jack_client_t *JackInput::get_client() {
    bool new_client = false;
    // Jack client initialization (SINGLETON should be called)
    if (mainclient == NULL) {
        new_client = true;
        ostringstream dst_port_str, dst_client_str;
        dst_client_str << "sp_client_" << getpid();
        if ((mainclient = jack_client_new (
                        string(dst_client_str.str()).c_str())) == 0) {
			LOG_FATAL("jack server not running?\n");
			exit(1);
        }
        LOG_INFO("Created new jack client %s", dst_client_str.str().c_str());
        if (jack_set_process_callback(mainclient, JackInput::jack_proc, NULL)) {
			LOG_FATAL("Failed to set process registration callback for %s",
                    dst_client_str.str().c_str());
        }
        LOG_DEBUG("Created process callback jack_proc for %s",
                dst_client_str.str().c_str());
        if (jack_activate (mainclient)) { LOG_FATAL("can't activate client"); }
        LOG_INFO("Activated jack client %s", dst_client_str.str().c_str());
    }
    if (new_client) {
        LOG_INFO("NEW Jack client requested");
    } else {
        LOG_DEBUG("Jack client requested, returned old one");
    }
    return mainclient;
}

JackInput::JackInput(const char *src_port_name, all_cfg_t *cfg) {
    name = (char*) malloc(strlen(src_port_name)+1);
    memcpy(name,src_port_name,strlen(src_port_name)+1);

    pthread_mutex_init(&data_mutex, NULL);
	pthread_cond_init(&condition_cond, NULL);
    jack_client_t *client = get_client();

    src_port = jack_port_by_name(client, src_port_name);

    SAMPLE_RATE = jack_get_sample_rate(client);

    char shortname[15];
    if (jack_port_name_size() <= 15) {
        LOG_FATAL("Too short jack port name supported");
        exit(1);
    }

    sprintf(shortname, "input_%d", ++JackInput::number_of_clients);
    dst_port = jack_port_register (client, shortname,
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    dst_port_name = string(jack_port_name(dst_port));

    if (jack_connect(client, src_port_name, jack_port_name(dst_port))) {
        LOG_FATAL("Failed to connect %s to %s, exiting.\n",
                src_port_name, jack_port_name(dst_port));
		exit(1);
    }

    LOG_DEBUG("Locking JackInput mutex to insert instance to jackInputs");
    pthread_mutex_lock(&jackInputsMutex);
    jackInputs.insert( pair<string,JackInput*>(dst_port_name,this) );
    pthread_mutex_unlock(&jackInputsMutex);
    LOG_INFO("jack port to jackInputs inserted, port ready to work");
}


JackInput::~JackInput() {
    // Delete input port

    delete name;
    LOG_DEBUG("Disconnecting ports");
    if (jack_port_unregister(get_client(), dst_port)) {
        LOG_ERROR( "Failed to remove port %s", jack_port_name(src_port));
    }

    pthread_mutex_lock(&jackInputsMutex);
    jackInputs.erase(this->dst_port_name);
    pthread_mutex_unlock(&jackInputsMutex);

    LOG_DEBUG("Deleted JackInput from jackInputs, ports are closed,\
            leaving destructor");
}

int JackInput::giveInput(buffer_t *buf_prop) {
    // Create a new thread and wait for input. When get a buffer - return back.

    pthread_mutex_lock(&data_mutex);
    pthread_cond_wait(&condition_cond, &data_mutex);

    memcpy((void*)buf_prop, (void*)&data_in.front(), sizeof(*buf_prop));
    buf_prop->delete_me = false;

    data_in.pop_front(); // Remove the processed waiting member
    pthread_mutex_unlock(&data_mutex);
    return 1;
}

void JackInput::monitor_ports(action_t action, const char* isource,
        all_cfg_t *cfg, void *sp_params) {
    // Should be called in new thread. Waits for inputs

    pthread_mutex_init(&p_queue_mutex, NULL);
    pthread_cond_init(&p_queue_cond, NULL);

    ostringstream dst_client_str;
    dst_client_str << "sp_manager_" << getpid();
    if ((client = jack_client_new(string(dst_client_str.str()).c_str())) == 0){
         LOG_FATAL ("jack server not running?");
		 exit(1);
    }
    dst_client_str.~ostringstream();

    LOG_INFO("Created jack_client: %s", jack_get_client_name(client));

    jack_set_port_connect_callback(client, JackInput::port_connect, NULL);
    if (jack_activate (client)) { LOG_FATAL ("cannot activate client"); }
    LOG_DEBUG("Activated jack manager client");

	// Waiting for new client to come up. port_connect fills p_queue
    pthread_mutex_lock(&p_queue_mutex);
    while (1) {
        pthread_cond_wait(&p_queue_cond, &p_queue_mutex);
        while (!p_queue.empty()) {
            pair<jack_port_t*, bool> port_info = p_queue.front();
            jack_port_t *port = port_info.first;
            p_queue.pop_front();
            const char *port_name = jack_port_name(port);
            // New port
            pthread_mutex_unlock(&p_queue_mutex);
            if (port_info.second) {
                LOG_DEBUG("Got new jack port, name: %s", port_name);
                // Wow that's a nice hack :-)
                all_cfg_t cfg_local(cfg->first, cfg->second);

                JackInput *input = new JackInput(port_name, &cfg_local);
                LOG_DEBUG("Created new JackInput instance (port name: %s),\
                        calling new_port_created", port_name);
                new_port_created(action, port_name, input, &cfg_local,
                        sp_params);
            } else { // Destroy this JackInput instance
                LOG_INFO("Call JackInput destructor for port %s", port_name);
                //jackInputs[string(port_name)]->~JackInput();
            }
            pthread_mutex_lock(&p_queue_mutex);
        }
	}
}

void JackInput::port_connect(jack_port_id_t a, jack_port_id_t b, int connect,
        void *arg) {
    jack_port_t *port_a = jack_port_by_id(JackInput::client, a);
    jack_port_t *port_b = jack_port_by_id(JackInput::client, b);

    if (!connect) {
		LOG_INFO("Ports %s and %s disconnect", jack_port_name(port_a),
                jack_port_name(port_b));
        // Check if this is one of sp_client_ ports
        if (strstr(jack_port_name(port_b), "sp_client_") !=
                jack_port_name(port_b)) {
            return;
        }
        // We have to disconnect this port
		LOG_DEBUG("Attempting to disconnect %s from %s",jack_port_name(port_a),
                jack_port_name(port_b));
        LOG_INFO("Adding to stack <%s, false>", jack_port_name(port_b));
        pthread_mutex_lock(&JackInput::p_queue_mutex);
        JackInput::p_queue.push_back(pair<jack_port_t*,bool>(port_b, false));
        pthread_mutex_unlock(&JackInput::p_queue_mutex);
        pthread_cond_signal(&JackInput::p_queue_cond);
	}

    // See if "dst" port does not begin with sp_client_
    if (strstr(jack_port_name(port_b), "sp_client_")==jack_port_name(port_b)) {
        return;
    }
    if (!(JackPortIsOutput & jack_port_flags(port_a))) return;
    LOG_INFO("Adding to stack <%s, true>", jack_port_name(port_a));
    pthread_mutex_lock(&JackInput::p_queue_mutex);
    JackInput::p_queue.push_back(pair<jack_port_t*,bool>(port_a, true));
    pthread_mutex_unlock(&JackInput::p_queue_mutex);
    pthread_cond_signal(&JackInput::p_queue_cond);
}
