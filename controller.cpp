
#include "controller.h"

/*
void *go_sp(void *port_name_a) {
    // Call SoundPatty on this port
    char *port_name = (char*)port_name_a;

    FILE *fpipe;
    char command[300];
    sprintf(command, "%s %s %s jack \"%s\"", SP_EXEC, SP_CONF, SP_TRES, port_name);
    //printf("Launching %s\n", command);

    char line[256];
    if ( !(fpipe = (FILE*)popen(command,"r")) )
    {  // If fpipe is NULL
        fatal((void*)"Problems with pipe");
    }
	fgets( line, sizeof line, fpipe); pclose(fpipe);
    if (strstr(line, "FOUND") != NULL) {
        // Found... E-mail script output
        char command[300];
        sprintf(command, "%s \"%s\" \"%s\"", SP_OVER, port_name, line);
        system(command);
    }

    fpipe = popen("date \"+%F %T\" | tr -d '\n\'", "r"); // Date without ENDL
    char output[300]; int fd;
    fgets (output, sizeof output, fpipe); pclose(fpipe);

    printf("%s *** %s *** ::: %s", output, port_name, line);
};

*/
void port_connect(jack_port_id_t a, jack_port_id_t b, int connect, void *arg) {
    log4cxx::LoggerPtr l(log4cxx::Logger::getRootLogger());
    if (!connect) {
		LOG4CXX_DEBUG(l,"Ports "<<jack_port_name(jack_port_by_id(client,a))<<" and "
				<< jack_port_name(jack_port_by_id(client,b))<<" disconnect, ignoring");
	}

    // See if "dst" port does not begin with sp_client_
    jack_port_t *port_a = jack_port_by_id(client, a);
    jack_port_t *port_b = jack_port_by_id(client, b);
    if (strstr(jack_port_name(port_b), "sp_client_") == jack_port_name(port_b)) {
        return;
    }
    if (!(JackPortIsOutput & jack_port_flags(port_a))) return;
    LOG4CXX_INFO(l,"Adding to stack a: "<<jack_port_name(port_a)<<" (b: "<< jack_port_name(port_b)<<")");
    pthread_mutex_lock(&p_queue_mutex);
    p_queue.push_back(port_a);
    pthread_mutex_unlock(&p_queue_mutex);
    pthread_cond_signal(&p_queue_cond);
};

void its_over(double place) {
    log4cxx::LoggerPtr l(log4cxx::Logger::getRootLogger());
	char msg[50];
	sprintf(msg,"FOUND, processed %.6f sec\n", place);
    LOG4CXX_INFO(l,msg);
	return;
};

int main () {

    log4cxx::LogManager::resetConfiguration();
    log4cxx::LayoutPtr layoutPtr(new log4cxx::PatternLayout("%d{yyyy-MM-dd HH:mm:ss,SSS} [%t] %-19l %-5p - %m%n"));
    log4cxx::AppenderPtr appenderPtr(new log4cxx::ConsoleAppender(layoutPtr, "System.err"));
    log4cxx::BasicConfigurator::configure(appenderPtr);

    log4cxx::LoggerPtr l(log4cxx::Logger::getRootLogger());
    l->setLevel(log4cxx::Level::getDebug());

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
    LOG4CXX_INFO(l,"Activated jack manager client");

	// Waiting for new client to come up. port_connect fills p_queue
    pthread_mutex_lock(&p_queue_mutex);
    while (1) {
        pthread_cond_wait(&p_queue_cond, &p_queue_mutex);
        while (!p_queue.empty()) {

            jack_port_t *port = p_queue.front();
            p_queue.pop_front();
            pthread_mutex_unlock(&p_queue_mutex);

            const char *port_name = jack_port_name(port);
            LOG4CXX_DEBUG(l,"Got new jack port, name: " << port_name);

            // Create new SoundPatty instance here
            input = new JackInput(port_name, &this_cfg);
            LOG4CXX_DEBUG(l,"Created new JackInput instance");
            pat = new SoundPatty(input, &this_cfg);
            pat->setAction(ACTION_CATCH, (char*)SP_TRES, its_over);
            pthread_mutex_lock(&p_queue_mutex);
        }
	}
	return 0;
}
