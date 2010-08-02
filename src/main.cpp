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

#include <unistd.h>
#include "main.h"
#include "soundpatty.h"
#include "fileinput.h"
#ifdef HAVE_JACK
#include "jackinput.h"
#endif // HAVE_JACK


void its_over(const char* noop, double place) {
    printf("FOUND, processed %.6f sec\n", place);
    exit(0);
};
void usage() {
    perror (
        "main <options> [channel/file]name\n\n"

        "Options:\n"
        "  -a  action (always mandatory, see below)\n"
        "  -c  config file (always mandatory)\n"
        "  -s  sample file (only if action is \"capture\")\n"
        "  -d  input driver (see command showdrv for possible drivers), default: file\n"
        "  -v  verbosity level (-v, -vv, -vvv, -vvvv)\n"
        "  -q  quiet output. Only FATAL are shown (supersedes -v)\n\n"

        "Actions:\n"
        "  dump           : creates a sample fingerprint\n"
        "  capture        : tries to capture a sound pattern\n"
        "  capture-auto   : same as capture, but gets input source automatically\n"
        "  showdrv        : show possible input drivers\n\n"

        "[Channel/file]name:\n"
#ifdef HAVE_JACK
        "  file name (- for stdin) or jack channel name\n\n"
#else
        "  file name (- for stdin)\n\n"
#endif
            );
    exit(0);
}

int main (int argc, char *argv[]) {
    char *cfgfile = NULL, *isource = NULL, *samplefile = NULL,
         *idrv = NULL, *action = NULL;

    int action_num = -1, quiet = 0;
    LogLevel = 3;

    if (argc < 2) {
        usage();
    }

    int c;
    while ((c = getopt(argc, argv, "a:c:s:d:v::q")) != -1)
        switch (c)
        {
            case 'a':
                action = optarg;
                break;
            case 'c':
                cfgfile = optarg;
                break;
            case 's':
                samplefile = optarg;
                break;
            case 'd':
                idrv = optarg;
                break;
            case 'q':
                quiet = 1;
                break;
            case 'v':
                LogLevel = 3;
                if (optarg != NULL) {
                    LogLevel += strlen(optarg);
                }
                break;
        }

    if (optind != argc) {
        isource = argv[optind];
    }

    if (action == NULL) {
        perror("Action not specified\n\n");
        usage();
    }

    if (strcmp(action, "showdrv") == 0) {
        string drivers("file");
#ifdef HAVE_JACK
        drivers += " jack";
#endif
        printf("Possible drivers: %s\n", drivers.c_str());
        exit(0);
    } else if (strcmp(action, "capture") == 0 or strcmp(action, "capture-auto") == 0) {
        action_num = ACTION_CATCH;
        if (samplefile == NULL) {
            perror("Action is catch, but samplefile not specified.");
            usage();
        }
    } else if (strcmp(action, "dump") == 0) {
        action_num = ACTION_DUMP;
    } else {
        cerr << "Invalid action: " << action << endl << endl;
        usage();
    }


    if (cfgfile == NULL) {
        perror("Config file not specified\n\n");
        usage();
    }
    if (idrv == NULL) {
        LOG_DEBUG("idrv null, making default: file");
        idrv = (char*) malloc(5 * sizeof(char));
        strcpy(idrv, "file");
    }
    if (strcmp(action, "capture-auto") != 0 and isource == NULL) {
        perror("Input source not specified\n\n");
        usage();
    }

    LOG_TRACE("action: %s", action);
    LOG_TRACE("cfgfile: %s", cfgfile);
    LOG_TRACE("samplefile: %s", samplefile);
    LOG_TRACE("idrv: %s", idrv);
    LOG_TRACE("isource: %s", isource);

    // ------------------------------------------------------------
    // End of parameter capturing, creating input instance
    //

    LOG_DEBUG("Starting to read configs from %s", cfgfile);
    all_cfg_t this_cfg = SoundPatty::read_cfg(cfgfile);
    Input *input = NULL;

    if (strcmp(action, "capture-auto") == 0) {
        if (strcmp(idrv, "file") == 0) {
            FileInput::monitor_ports(&this_cfg);
            LOG_INFO("Starting FileInput monitor_ports");
        } else {
#ifdef HAVE_JACK
            LOG_INFO("Starting FileInput monitor_ports");
            JackInput::monitor_ports(&this_cfg);
#endif // HAVE_JACK
        }

    } else {
        if (strcmp(idrv, "file") == 0) {
            input = new FileInput(isource, &this_cfg);
            LOG_INFO("Sox input, input file: %s, created instance", argv[argc-1]);
        } else {
#ifdef HAVE_JACK
            input = new JackInput(isource, &this_cfg);
#endif // HAVE_JACK
        }
        LOG_DEBUG("Creating SoundPatty instance");
        SoundPatty *pat = new SoundPatty("nothing", input, &this_cfg);

        if (action_num == ACTION_DUMP) {
            pat->setAction(ACTION_DUMP);
            LOG_INFO("Action is DUMP");
        } else if (action_num == ACTION_CATCH) {
            LOG_INFO("Action is CATCH, sample file: %s", samplefile);
            pat->setAction(ACTION_CATCH, samplefile, its_over);
        }

        LOG_INFO("Starting main SoundPatty loop");
        SoundPatty::go_thread(pat);
        LOG_INFO("SoundPatty main loop completed");
    }

    exit(0);
}
