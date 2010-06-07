/*  
 *  controller.cpp
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

#include "controller.h"

void port_connect(jack_port_id_t a, jack_port_id_t b, int connect, void *arg) {
    log4cxx::LoggerPtr l(log4cxx::Logger::getLogger("controller"));

    jack_port_t *port_a = jack_port_by_id(client, a);
    jack_port_t *port_b = jack_port_by_id(client, b);

    if (!connect) {
		LOG4CXX_INFO(l,"Ports "<<jack_port_name(port_a)<<" and " << jack_port_name(port_b)<<" disconnect");
        // Check if this is one of sp_client_ ports
        if (strstr(jack_port_name(port_b), "sp_client_") != jack_port_name(port_b)) {
            return;
        }
        // We have to disconnect this port
		LOG4CXX_DEBUG(l,"Attempting to disconnect "<<jack_port_name(port_a)<<" from " << jack_port_name(port_b));

	}

    // See if "dst" port does not begin with sp_client_
    if (strstr(jack_port_name(port_b), "sp_client_") == jack_port_name(port_b)) {
        return;
    }
    if (!(JackPortIsOutput & jack_port_flags(port_a))) return;
    LOG4CXX_INFO(l,"Adding to stack " << jack_port_name(port_a));
    pthread_mutex_lock(&p_queue_mutex);
    p_queue.push_back(port_a);
    pthread_mutex_unlock(&p_queue_mutex);
    pthread_cond_signal(&p_queue_cond);
};

void its_over(const char *port_name, double place) {
    log4cxx::LoggerPtr l(log4cxx::Logger::getRootLogger());
	char msg[50];
	sprintf(msg,"FOUND, processed %.6f sec", place);
    LOG4CXX_INFO(l,msg);
    // Call the over.sh function
    char command[300];
    sprintf(command, "%s \"%s\" \"%.6f\"", SP_OVER, port_name, place);
    system(command);
};

int main () {

    log4cxx::PropertyConfigurator::configure("log4j.cfg");
    log4cxx::LoggerPtr l(log4cxx::Logger::getLogger("controller"));

    LOG4CXX_INFO(l,"Starting to read configs from " << SP_CONF);
    all_cfg_t this_cfg = SoundPatty::read_cfg(SP_CONF); //usually config.cfg
    Input *input;
    SoundPatty *pat;

    pthread_mutex_init(&p_queue_mutex, NULL);
    pthread_cond_init(&p_queue_cond, NULL);

    ostringstream dst_client_str;
    dst_client_str << "sp_manager_" << getpid();
    if ((client = jack_client_new (string(dst_client_str.str()).c_str())) == 0) {
         LOG4CXX_FATAL (l,"jack server not running?");
		 exit(1);
    }
    dst_client_str.~ostringstream();

    LOG4CXX_INFO(l,"Created jack_client: " << jack_get_client_name(client));

    jack_set_port_connect_callback(client, port_connect, NULL);
    if (jack_activate (client)) { LOG4CXX_FATAL (l,"cannot activate client"); }
    LOG4CXX_DEBUG(l,"Activated jack manager client");

	// Waiting for new client to come up. port_connect fills p_queue
    pthread_mutex_lock(&p_queue_mutex);
    while (1) {
        pthread_cond_wait(&p_queue_cond, &p_queue_mutex);
        while (!p_queue.empty()) {
            jack_port_t *port = p_queue.front();
            p_queue.pop_front();
            pthread_mutex_unlock(&p_queue_mutex);

            const char *port_name = jack_port_name(port);
            LOG4CXX_DEBUG(l,"Got new jack port, name: "<<port_name);

            // Create new SoundPatty instance here
            input = new JackInput(port_name, &this_cfg);
            LOG4CXX_DEBUG(l,"Created new JackInput instance, creating SoundPatty");
            pat = new SoundPatty(port_name, input, &this_cfg);
            LOG4CXX_DEBUG(l,"Created SoundPatty instance, setting action callback");
            pat->setAction(ACTION_CATCH, (char*)SP_TRES, its_over);

            pthread_t tmp; pthread_attr_t attr; pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            if (int err = pthread_create(&tmp, &attr, SoundPatty::go_thread, (void*)pat)) {
                LOG4CXX_ERROR(l,"Failed to create thread for "<<port_name<<", error "<<err);
            }
            LOG4CXX_INFO(l,"Launched new SoundPatty thread for "<<port_name);

            pthread_mutex_lock(&p_queue_mutex);
        }
	}
	return 0;
}
