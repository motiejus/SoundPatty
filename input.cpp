/*
 *  input.cpp
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

#include "input.h"

jack_client_t *JackInput::_client;
JackInput *JackInput::jack_inst;

pthread_mutex_t jackInputsMutex;
list<JackInput*> jackInputs;

int JackInput::jack_proc(jack_nframes_t nframes, void *arg) {

    pthread_mutex_lock(&jackInputsMutex);
    for(list<JackInput*>::iterator inp = jackInputs.begin(); inp != jackInputs.end(); inp++) {
        JackInput *in_inst = *inp;
        pthread_mutex_lock(&in_inst->data_mutex);

        buffer_t buffer;
        buffer.buf = (jack_default_audio_sample_t *) jack_port_get_buffer (in_inst->dst_port, nframes);
        buffer.nframes = nframes;
        in_inst->data_in.push_back(buffer);
        pthread_cond_signal(&in_inst->condition_cond);
        pthread_mutex_unlock(&in_inst->data_mutex);
    }
    pthread_mutex_unlock(&jackInputsMutex);

    return 0;
};

jack_client_t *JackInput::get_client() {
    if (_client == NULL) { // Jack client initialization (SINGLETON)
        ostringstream dst_port_str, dst_client_str;
        dst_client_str << "sp_client_" << getpid();
        if ((JackInput::_client = jack_client_new (string(dst_client_str.str()).c_str())) == 0) {
            fatal ((void*)"jack server not running?\n");
        }
        jack_set_process_callback(_client, JackInput::jack_proc, NULL);
        if (jack_activate (_client)) { fatal ((void*)"cannot activate client"); }
    }
    return JackInput::_client;
};

JackInput::JackInput(const void * args, all_cfg_t *cfg) {

    pthread_mutex_init(&jackInputsMutex, NULL);

    data_in;
    pthread_mutex_init(&data_mutex, NULL);
	pthread_cond_init(&condition_cond, NULL);

    char *src_port_name = (char*) args;

    jack_client_t *client = JackInput::get_client();
    SAMPLE_RATE = jack_get_sample_rate (client);
    cfg->first["WAVE"] = (int)SAMPLE_RATE * cfg->first["minwavelen"];
    cfg->first["CHUNKSIZE"] = cfg->first["chunklen"] * (int)SAMPLE_RATE;

    pthread_mutex_lock(&jackInputsMutex);
    jackInputs.push_back(this);
    pthread_mutex_unlock(&jackInputsMutex);

    dst_port = jack_port_register (client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    if (jack_connect(client, src_port_name, jack_port_name(dst_port))) {
        char errmsg[200];
        sprintf(errmsg, "Failed to connect %s to %s, exiting.\n", src_port_name, jack_port_name(dst_port));
        fatal((void*)errmsg);
    }
};


int JackInput::giveInput(buffer_t *buffer) {
    // Create a new thread and wait for input. When get a buffer - return back.

    pthread_mutex_lock(&data_mutex);
    pthread_cond_wait(&condition_cond, &data_mutex);

    memcpy((void*)buffer, (void*)&data_in.front(), sizeof(*buffer)); // Copy all buffer data blindly
    data_in.pop_front(); // Remove the processed waiting member
    pthread_mutex_unlock(&data_mutex);
    return 1;
}

int WavInput::giveInput(buffer_t *buf_prop) {
    int16_t buf [SAMPLE_RATE * BUFFERSIZE]; // Process buffer every BUFFERSIZE secs
    if (feof(_fh)) {
        return 0;
    }
    size_t read_size = fread(buf, 2, SAMPLE_RATE * BUFFERSIZE, _fh);

    // :HACK: fix this, do not depend on jack types where you don't need!
    jack_default_audio_sample_t buf2 [SAMPLE_RATE * BUFFERSIZE];
    for(unsigned int i = 0; i < read_size; i++) {
        buf2[i] = (jack_default_audio_sample_t)buf[i];
    }
    buf_prop->buf = buf2;
    buf_prop->nframes = read_size;
    return 1;
};



/// WavInput here ///
int WavInput::process_headers(const char * infile, all_cfg_t *cfg) {
    char errmsg[200];

    _fh = fopen(infile, "rb");
    if (_fh == NULL) {
        sprintf(errmsg, "Failed to open file %s, exiting\n", infile);
        fatal(errmsg);
    }

    // Read bytes [0-3] and [8-11], they should be "RIFF" and "WAVE" in ASCII
    char header[5];
    fread(header, 1, 4, _fh);

    char sample[] = "RIFF";
    if (! check_sample(sample, header) ) {
        sprintf(errmsg, "RIFF header not found in %s, exiting\n", infile);
        fatal(errmsg);
    }
    // Checking for compression code (21'st byte is 01, 22'nd - 00, little-endian notation
    uint16_t tmp[2]; // two-byte integer
    fseek(_fh, 0x14, 0); // offset = 20 bytes
    fread(&tmp, 2, 2, _fh); // Reading two two-byte samples (comp. code and no. of channels)
    if ( tmp[0] != 1 ) {
        sprintf(errmsg, "Only PCM(1) supported, compression code given: %d\n", tmp[0]);
        fatal (errmsg);
    }
    // Number of channels must be "MONO"
    if ( tmp[1] != 1 ) {
        sprintf(errmsg, "Only MONO supported, given number of channels: %d\n", tmp[1]);
        fatal (errmsg);
    }
    fread(&SAMPLE_RATE, 2, 1, _fh); // Single two-byte sample
    cfg->first["WAVE"] = (int)SAMPLE_RATE * cfg->first["minwavelen"];
    cfg->first["CHUNKSIZE"] = cfg->first["chunklen"] * (int)SAMPLE_RATE;
    fseek(_fh, 0x22, 0);
    uint16_t BitsPerSample;
    fread(&BitsPerSample, 1, 2, _fh);
    if (BitsPerSample != 16) {
        sprintf(errmsg, "Only 16-bit WAVs supported, given: %d\n", BitsPerSample);
        fatal(errmsg);
    }
    // Get data chunk size here
    fread(header, 1, 4, _fh);
    strcpy(sample, "data");

    if (! check_sample(sample, header)) {
        fatal ((void*)"data chunk not found in byte offset=36, file corrupted.");
    }
    int DATA_SIZE;
    fread(&DATA_SIZE, 4, 1, _fh); // Single two-byte sample
    return 0;
};


bool WavInput::check_sample (const char * sample, const char * b) { // My STRCPY.
    for(int i = 0; sample[i] != '\0'; i++) {
        if (sample[i] != b[i]) {
            return false;
        }
    }
    return true;
};


WavInput::WavInput(const void * args, all_cfg_t *cfg) {
    process_headers((char*)args, cfg);
    // ------------------------------------------------------------
    // Adjust _sp_ volume 
    // Jack has float (-1..1) while M$ WAV has (-2^15..2^15)
    //
    for (vector<sVolumes>::iterator vol = cfg->second.begin(); vol != cfg->second.end(); vol++) {
        vol->min *= (1<<15);
        vol->max *= (1<<15);
    }
};


