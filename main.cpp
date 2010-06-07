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
#include "soundpatty.h"
// TODO: #include <getopt.h>

void its_over(const char* noop, double place) {
    printf("FOUND, processed %.6f sec\n", place);
    exit(0);
};

int main (int argc, char *argv[]) {
    if (argc < 3) {
        perror ("Usage: ./main config.cfg sample.wav\nor\n"
                "./main config.cfg samplefile.txt catchable.wav\n"
                "./main config.cfg samplefile.txt jack jack\n"
				"./main (jack input, self-capturing ports, most feature-rich args");
		exit(1);
    }

    log4cxx::PropertyConfigurator::configure("log4j.conf");
    log4cxx::LoggerPtr l(log4cxx::Logger::getRootLogger());
    l->setLevel(log4cxx::Level::getTrace());


    LOG4CXX_DEBUG(l,"Starting to read configs from " << argv[1]);
    all_cfg_t this_cfg = SoundPatty::read_cfg(argv[1]); //usually config.cfg
    Input *input;
    SoundPatty *pat;

    if (argc == 3 || argc == 4) { // WAV
        LOG4CXX_DEBUG(l,"Wav input, input file: " << argv[argc - 1]);
        input = new WavInput(argv[argc-1], &this_cfg);
    }
    if (argc == 5) { // Catch Jack
        LOG4CXX_DEBUG(l,"Jack input instance, file: " << argv[4]);
        input = new JackInput(argv[4], &this_cfg);
    }
    pat = new SoundPatty("nothing", input, &this_cfg);
    LOG4CXX_DEBUG(l,"Created first SoundPatty instance");

    if (argc == 3) { // Dump out via WAV
        pat->setAction(ACTION_DUMP);
        LOG4CXX_DEBUG(l,"Action is DUMP");
    }
    if (argc == 4 || argc == 5) { // Catch WAV or Jack
        pat->setAction(ACTION_CATCH, argv[2], its_over);
        LOG4CXX_DEBUG(l,"Action is CATCH, filename: " << argv[2]);
    }
    // Hmm if more - we can create new SoundPatty instances right here! :-)
    LOG4CXX_INFO(l,"Starting main SoundPatty loop");
    pat->go();
    LOG4CXX_INFO(l,"SoundPatty main loop completed");

    exit(0);
}
