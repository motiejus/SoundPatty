#include "controller.h"

void port_connect(jack_port_id_t a, jack_port_id_t b, int connect, void *arg) {
    if (!connect) return;
    // See if "dst" port does not begin with sp_client_
    jack_port_t *port_a = jack_port_by_id(client, a);
    jack_port_t *port_b = jack_port_by_id(client, b);
    if (strstr(jack_port_name(port_b), "sp_client_") == jack_port_name(port_b)) {
        return;
    }
    if (!(JackPortIsOutput & jack_port_flags(port_a))) return;
    //printf("Adding to stack a: %s (b: %s)\n", jack_port_name(port_a), jack_port_name(port_b));
    pthread_mutex_lock(&p_queue_mutex);
    p_queue.push_back(port_a);
    pthread_mutex_unlock(&p_queue_mutex);
    pthread_cond_signal(&p_queue_cond);
}

void *connector(void *arg) {
    pthread_mutex_lock(&p_queue_mutex);
    while (1) {
        pthread_cond_wait(&p_queue_cond, &p_queue_mutex);
        while (!p_queue.empty()) {
            jack_port_t *port = p_queue.front();
            p_queue.pop_front();
            pthread_mutex_unlock(&p_queue_mutex);

            const char *port_name = jack_port_name(port);
            //printf("Got a port to connect to: %s\n", port_name);
            pthread_t tmp; pthread_attr_t attr;
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            pthread_create(&tmp, &attr, go_sp, (void*)(port_name));
            sps.insert(pair<const char*, pthread_t>(port_name, tmp));

            pthread_mutex_lock(&p_queue_mutex);
        }
    }
};

void its_over(double place) {
    printf("FOUND, processed %.6f sec\n", place);
    pthread_exit(NULL);
};

void *go_sp(void *port_name_a) {
    // Call SoundPatty on this port
    char *port_name = (char*)port_name_a;

    FILE *fpipe;
    char command[300];
    sprintf(command, "%s %s %s jack \"%s\"", SP_EXEC, SP_CONF, SP_TRES, port_name);

    char line[256];
    if ( !(fpipe = (FILE*)popen(command,"r")) )
    {  // If fpipe is NULL
        fatal((void*)"Problems with pipe");
    }
	fgets( line, sizeof line, fpipe); pclose(fpipe);

    fpipe = popen("date \"+%F %T\" | tr -d '\n\'", "r"); // Date without ENDL
    char output[300]; int fd;
    fgets (output, sizeof output, fpipe); pclose(fpipe);

    sprintf(output, "%s *** %s *** ::: %s", output, port_name, line);
    write(fd, output, strlen(output)+1);
    pthread_exit(NULL);
};

int main () {

    pthread_mutex_init(&p_queue_mutex, NULL);
    pthread_cond_init(&p_queue_cond, NULL);
    ostringstream dst_client_str;
    dst_client_str << "sp_manager_" << getpid();
    if ((client = jack_client_new (string(dst_client_str.str()).c_str())) == 0) {
        fatal ((void*)"jack server not running?\n");
    }
    dst_client_str.~ostringstream();

    jack_set_port_connect_callback(client, port_connect, NULL);
    if (jack_activate (client)) { fatal ((void*)"cannot activate client"); }

    pthread_t connector1;
    pthread_create(&connector1, NULL, connector, NULL);

    while (1) {
        sleep (1);
    }
    return 0;
}

void fatal(void * r) {
    char * msg = (char*) r;
    printf (msg);
    pthread_exit (NULL);
};


void fatal(char * msg) {
    printf (msg);
    pthread_exit (NULL);
};
