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
	sprintf(msg,"FOUND, processed %.6f sec", place);
    LOG_INFO(msg);
    // Call the over.sh function
    //char command[300];
    //sprintf(command, "%s \"%s\" \"%.6f\"", SP_OVER, port_name, place);
    //system(command);
};

void Input::new_port_created(const char *port_name, Input *input, all_cfg_t *cfg) {
    // Hum, must create new SomeInput instance here
    // So we have new port created here. In case of jack -
    // it's safe to call jack server here, since it's not a caller thread :-)

    SoundPatty *pat = new SoundPatty(port_name, input, cfg);
    LOG_DEBUG("Created SoundPatty instance, setting action callback");
    pat->setAction(ACTION_CATCH, (char*)"fixme", Input::its_over);

    pthread_t tmp; pthread_attr_t attr; pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (int err = pthread_create(&tmp, &attr, SoundPatty::go_thread, (void*)pat)) {
        LOG_ERROR("Failed to create thread for %s, error %d", port_name, err);
    }
    LOG_INFO("Launched new SoundPatty thread for %s", port_name);
}
