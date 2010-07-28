/*
 *  wavinput.h
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


#ifndef __WAVINPUT_H_INCLUDED__
#define __WAVINPUT_H_INCLUDED__

#include "input.h"

class WavInput : public Input {
    public:
        int giveInput(buffer_t *);
        WavInput(const void *, all_cfg_t *);
    protected:
        int process_headers(const char * infile, all_cfg_t *);
        bool check_sample (const char * sample, const char * b);
    private:
        FILE *_fh;
};

#endif //__WAVINPUT_H_INCLUDED__
