/*
 *  fileinput.h
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


#ifndef __FILENPUT_H_INCLUDED__
#define __FILENPUT_H_INCLUDED__

#include "input.h"
#include <sox.h>

class FileInput : public Input {
    public:
        ~FileInput();
        int giveInput(buffer_t *);
        FileInput(const char *, all_cfg_t *);
        static void monitor_ports(action_t, const char*, all_cfg_t*, void*);
    private:
        sox_format_t *s;
        bool reading_over;
};

#endif //__FILEINPUT_H_INCLUDED__
