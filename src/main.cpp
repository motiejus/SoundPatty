/*  
 *  main.cpp
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

#include "main.h"
#include "wavinput.h"
#include "soundpatty.h"
// TODO: #include <getopt.h>

#ifdef HAVE_JACK
#include "jackinput.h"
#endif

void its_over(const char* noop, double place) {
    printf("FOUND, processed %.6f sec\n", place);
    exit(0);
};

int main (int argc, char *argv[]) {
    if (argc < 3) {
        perror ("Usage: ./main config.cfg sample.wav\nor\n"
                "./main config.cfg samplefile.txt catchable.wav\n"
#ifdef HAVE_JACK
                "./main config.cfg samplefile.txt jack jack\n"
#endif
                );
		exit(1);
    }

    LOG_DEBUG("Starting to read configs from %s", argv[1]);
    all_cfg_t this_cfg = SoundPatty::read_cfg(argv[1]); //usually config.cfg
    SoundPatty *pat;
    Input *input = NULL;

    if (argc == 3 || argc == 4) { // WAV
        LOG_DEBUG("Wav input, input file: %s");
        input = new WavInput(argv[argc-1], &this_cfg);
    }
#ifdef HAVE_JACK
    if (argc == 5) { // Catch Jack
        LOG_DEBUG("Jack input instance, file: %s", argv[4]);
        input = new JackInput(argv[4], &this_cfg);
    }
#endif
    pat = new SoundPatty("nothing", input, &this_cfg);
    LOG_DEBUG("Created first SoundPatty instance");

    if (argc == 3) { // Dump out via WAV
        pat->setAction(ACTION_DUMP);
        LOG_DEBUG("Action is DUMP");
    }
    if (argc == 4 || argc == 5) { // Catch WAV or Jack
        pat->setAction(ACTION_CATCH, argv[2], its_over);
        LOG_DEBUG("Action is CATCH, filename: %s", argv[2]);
    }
    // Hmm if more - we can create new SoundPatty instances right here! :-)
    LOG_INFO("Starting main SoundPatty loop");
    pat->go();
    LOG_INFO("SoundPatty main loop completed");

    exit(0);
}
