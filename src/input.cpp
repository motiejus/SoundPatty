/*
 *  input.cpp
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

#include "input.h"
#include "soundpatty.h"

void Input::its_over(const char *port_name, double place) {
	char msg[50];
	sprintf(msg,"FOUND for %s, processed %.6f sec", port_name, place);
    printf("%s", msg);
    LOG_INFO(msg);
    // Call the over.sh function
    char command[300];
    sprintf(command, "./over.sh \"%s\" \"%.6f\"", port_name, place);
    int ret = system(command);
    if (ret != 0) {
        printf("System command failed! Return number: %d\n", ret);
    }
};

void Input::new_port_created(
        action_t action, const char *port_name,
        Input *input, all_cfg_t *cfg, void *sp_params) {

    // Hum, must create new SomeInput instance here
    // So we have new port created here. In case of jack -
    // it's safe to call jack server here, since it's not a caller thread :-)

    LOG_DEBUG("Creating SoundPatty instance for new port");
    SoundPatty *pat = new SoundPatty(action, input, cfg, sp_params);

    pthread_t tmp; pthread_attr_t attr; pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (int err = pthread_create(&tmp, &attr, SoundPatty::go_thread, (void*)pat)) {
        LOG_ERROR("Failed to create thread for %s, error %d", port_name, err);
    }
    LOG_INFO("Launched new SoundPatty thread for %s", port_name);
}
