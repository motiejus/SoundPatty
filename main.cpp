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
#include "getopt.h"

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

    all_cfg_t this_cfg = SoundPatty::read_cfg(argv[1]); //usually config.cfg
    Input *input;
    SoundPatty *pat;


    int c;
    while (1) {
        static struct option long_options[] =
        {
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"action",     required_argument,       0, 'a'},
            // Dump or catch

            {"config-file",  required_argument,       0, 'c'},
            // Config file

            {"input-driver",  required_argument, 0, 'd'},
            // Jack or WAV

            {"input-name",  required_argument, 0, 'n'},
            // For jack: input port name, for WAV: filename.wav


            {"dump-file",  optional_argument, 0, 'd'},
            // Usually samplefile.txt (file with treshold values)

            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "abc:d:f:", long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1)
            break;

        char *input_driver; // Allowed: jack or wav
        switch (c)
        {
            case 'd':
                if (strcmp(optarg, "jack") & strcmp(optarg, "wav") != 0) {
                    // Neither jack nor wav
                    printf ("Input driver must be either jack or wav!");
                    strcpy(input_driver, optarg);
                }
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort ();
        }


    }


    if (argc == 3 || argc == 4) { // WAV
        input = new WavInput(argv[argc - 1], &this_cfg);
    }
    if (argc == 5) { // Catch Jack
        input = new JackInput(argv[4], &this_cfg);
    }
    pat = new SoundPatty(input, &this_cfg);

    if (argc == 3) { // Dump out via WAV
        pat->setAction(ACTION_DUMP);
    }
    if (argc == 4 || argc == 5) { // Catch WAV or Jack
        pat->setAction(ACTION_CATCH, argv[2], its_over);
    }
    // Hmm if more - we can create new SoundPatty instances right here! :-)
    pat->go();

    exit(0);
}
