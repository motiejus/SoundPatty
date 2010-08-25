/*
 *  fileinput.cpp
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

#include "fileinput.h"
#ifdef HAVE_INOTIFY_INIT
#include <sys/inotify.h>
/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))

/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))
#endif // HAVE_INOTIFY_INIT

void FileInput::monitor_ports(action_t action, const char* isource, all_cfg_t *cfg, void *sp_params) {

#ifdef HAVE_INOTIFY_INIT
    int fd = inotify_init();
    if (fd < 0)
        LOG_FATAL("inotify_init failed");

    inotify_add_watch(fd, isource, IN_CREATE);
    LOG_INFO("Added inotify watch for %s", isource);


    while (1) {
        char buf[BUF_LEN];
        volatile int errno = 0;
        int len, i = 0;

        len = read (fd, buf, BUF_LEN);
        if (len < 0) {
            LOG_ERROR("Something went wrong with inotify, errno: %d", errno);
        } else if (!len) {
            LOG_ERROR("BUF_LEN is too small??");
        }
        while (i < len) {
            struct inotify_event *event;
            event = (struct inotify_event *) &buf[i];

            LOG_DEBUG("Got inotify event: wd=%d mask=%u cookie=%u len=%u",
                    event->wd, event->mask,
                    event->cookie, event->len);
            if (event->len) {
                all_cfg_t cfg_local(cfg->first, cfg->second); // Wow that's a nice hack :-)

                char *filename = (char*)malloc(strlen(isource)+strlen(event->name)+1);
                sprintf(filename, "%s%s", isource, event->name);
                LOG_INFO ("Creating new FileInput for %s", filename);
                FileInput *input = new FileInput(filename, &cfg_local);
                LOG_DEBUG("Calling new_port_created");
                new_port_created( action, filename, input, &cfg_local, sp_params );
                LOG_DEBUG("New port created callback is over");
                delete filename;
            }
            i += EVENT_SIZE + event->len;
        }
    }
#else
    LOG_FATAL("Not implemented!");
#endif // HAVE_INOTIFY_INIT
}
int FileInput::giveInput(buffer_t *buf_prop) {
    if (reading_over) { return 0; }


#ifdef HAVE_SOX
    unsigned bufferlength = (unsigned)(s->signal.rate+0.5) * BUFFERSIZE;
    sample_t buf [bufferlength];
    size_t read_size = sox_read(s, buf, bufferlength);
#elif defined HAVE_ST_H
    unsigned bufferlength = (unsigned)(s->info.rate+0.5) * BUFFERSIZE;
    sample_t buf [bufferlength];
    size_t read_size = st_read(s, buf, bufferlength);
#endif


    if (read_size < bufferlength) {
        LOG_INFO("EOF reached. Read_size: %d, bufferlength: %d",
                read_size, bufferlength);
        reading_over = true;
    }
	buf_prop->buf = (sample_t*) calloc(read_size, sizeof(sample_t));

	for (unsigned i = 0; i < read_size; i++) {
		buf_prop->buf[i] = (sample_t)buf[i];
	}
    buf_prop->nframes = read_size;
    return 1;
};


FileInput::FileInput(const char *isource, all_cfg_t *cfg) {
    name = (char*) malloc(strlen(isource)+1);
    memcpy(name,isource,strlen(isource)+1);

    reading_over = false;
#ifdef HAVE_SOX
    s = sox_open_read(isource, NULL, NULL, NULL);
    LOG_DEBUG("Sox initialized. Sampling rate: %.6f, channels: %d, bitrate: %d, samples: %d",
            s->signal.rate, s->signal.channels, s->signal.precision, s->signal.length);
    SAMPLE_RATE = (int)s->signal.rate;
#elif defined HAVE_ST_H
    s = st_open_read(isource, NULL, NULL);
    LOG_DEBUG("Sox initialized. Sampling rate: %.6f, channels: %d, samples: %d",
            s->info.rate, s->info.channels, s->info.size);
    SAMPLE_RATE = (int)s->info.rate;
#endif

    for (vector<sVolumes>::iterator vol = cfg->second.begin(); vol != cfg->second.end(); vol++) {
        vol->min = (sample_t)vol->min_orig * (1<<30);
        vol->max = (sample_t)vol->max_orig * (1<<30);
    }
};

FileInput::~FileInput() {
#ifdef HAVE_SOX
    LOG_INFO("Closing sox instance");
    sox_close(s);
#elif defined HAVE_ST_H
    LOG_INFO("Closing sox instance");
    st_close(s);
#endif
}
