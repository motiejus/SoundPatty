/*  
 *  controller.h
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

pthread_mutex_t p_queue_mutex;
pthread_cond_t p_queue_cond;

void its_over(double place);
void new_port(const jack_port_id_t port_id, int registerr, void *arg);

#endif //__SP_CONTROLLER_H_INCLUDED__
