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
        int SAMPLE_RATE, DATA_SIZE;
        virtual ~Input() {};
        virtual int giveInput(buffer_t * buffer) {
            perror("giveInput not implemented, exiting\n");
			exit(1);
        };
};

#endif //__INPUT_H_INCLUDED__
