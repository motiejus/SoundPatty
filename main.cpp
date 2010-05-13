/*  
 *  main.cpp
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

#include "main.h"
#include "input.h"
int jack_proc(jack_nframes_t nframes, void *arg) {
    JackInput * in_inst = (JackInput*) arg;
    printf("Got %d frames from jack, port name: %s\n", nframes, in_inst->dst_port);
    return 0;
};


void its_over(double place) {
    printf("FOUND, processed %.6f sec\n", place);
    exit(0);
};


int main (int argc, char *argv[]) {
    if (argc < 3) {
        fatal ((void*)"Usage: ./readit config.cfg sample.wav\nor\n"
                "./readit config.cfg samplefile.txt catchable.wav\n"
                "./readit config.cfg samplefile.txt jack jack\n");
    }
    SoundPatty * pat = new SoundPatty(argv[1]); // usually config.cfg
    if (argc == 3 || argc == 4) { // WAV
        pat->setInput(SRC_WAV, argv[argc - 1]);
    }
    if (argc == 3) { // Dump out via WAV
        pat->setAction(ACTION_DUMP);
    }
    if (argc == 4 || argc == 5) { // Catch WAV or Jack
        pat->setAction(ACTION_CATCH, argv[2], its_over);
    }
    if (argc == 5) { // Catch Jack
        pat->setInput(SRC_JACK_ONE, argv[4]);
    }
    pat->go();

    exit(0);
}
