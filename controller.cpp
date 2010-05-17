#include "controller.h"

void new_port(const jack_port_id_t port_id, int registerr, void *arg) {
    jack_port_t *port = jack_port_by_id(client, port_id);
    if (!registerr) return;

    if (!(JackPortIsOutput & jack_port_flags(port))) return;
    pthread_mutex_lock(&p_queue_mutex);
    p_queue.push_back(port);
    pthread_mutex_unlock(&p_queue_mutex);
    pthread_cond_signal(&p_queue_cond);
};

void *connector(void *arg) {
    while (1) {
        pthread_mutex_lock(&p_queue_mutex);
        pthread_cond_wait(&p_queue_cond, &p_queue_mutex);
        jack_port_t *port = p_queue.front();
        p_queue.pop_front();
        pthread_mutex_unlock(&p_queue_mutex);

        const char *port_name = jack_port_name(port);
        pthread_t tmp;
        pthread_create(&tmp, NULL, go_sp, (void*)(port_name));
        sps.insert(pair<const char*, pthread_t>(port_name, tmp));

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
    sprintf(command, "%s %s %s jack %s", SP_EXEC, SP_CONF, SP_TRES, port_name);

    char line[200];
    if ( !(fpipe = (FILE*)popen(command,"r")) )
    {  // If fpipe is NULL
        fatal((void*)"Problems with pipe");
    }

	fgets( line, sizeof line, fpipe);

    system("date \"+%F %T\" | tr -d '\n\'"); // Date without ENDL
    printf(" *** %s *** ::: %s", port_name, line);
};

int main () {

    pthread_mutex_init(&p_queue_mutex, NULL);
    pthread_cond_init(&p_queue_cond, NULL);
    ostringstream dst_client_str;
    dst_client_str << "sp_manager_" << getpid();
    if ((client = jack_client_new (string(dst_client_str.str()).c_str())) == 0) {
        fatal ((void*)"jack server not running?\n");
    }

    jack_set_port_registration_callback(client, new_port, NULL);
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
