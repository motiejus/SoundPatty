/*
 *  input.h
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


#ifndef __INPUT_H_INCLUDED__
#define __INPUT_H_INCLUDED__

#include "main.h"

class Input {
    public:
        // ----------------------------------------------------------------------
        // Input subclasses must have these methods defined:
        // constructor (any type)
        virtual ~Input() {};
        virtual int giveInput(buffer_t * buffer) {
            (void)buffer;
            perror("giveInput not implemented, exiting\n");
			exit(1);
        };
        

        // ----------------------------------------------------------------------
        // Subclasses must also fill these values (constant at runtime)
        bool reading_over;
        char *name; // Unique name for each input stream (eg. filename)
        int SAMPLE_RATE, DATA_SIZE;
        double WAVE, CHUNKSIZE;

        // ----------------------------------------------------------------------
        // If subclass has SubInput::monitor_ports, it calls this function
        static void new_port_created(action_t, const char*, Input*, all_cfg_t*, void*);
        static void its_over(const char*, double);
};

#endif //__INPUT_H_INCLUDED__
