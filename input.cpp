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

JackInput *JackInput::jack_inst = NULL;
long JackInput::number_of_clients = 0; // Counter of jack clients
// Not protected by mutex, because accessed only in one thread

jack_client_t *mainclient = NULL;
pthread_mutex_t jackInputsMutex = PTHREAD_MUTEX_INITIALIZER;
map<string,JackInput*> jackInputs;

int JackInput::jack_proc(jack_nframes_t nframes, void *arg) {

    pthread_mutex_lock(&jackInputsMutex);
    for(map<string,JackInput*>::iterator inp = jackInputs.begin(); inp != jackInputs.end(); inp++) {
        JackInput *in_inst = inp->second;
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
    string logger_name ("input.jack."); 
    logger_name += string(dst_port_name);

    log4cxx::LoggerPtr l(log4cxx::Logger::getLogger(logger_name));
    bool new_client = false;
    if (mainclient == NULL) { // Jack client initialization (SINGLETON should be called)
        new_client = true;
        ostringstream dst_port_str, dst_client_str;
        dst_client_str << "sp_client_" << getpid();
        if ((mainclient = jack_client_new (string(dst_client_str.str()).c_str())) == 0) {
			LOG4CXX_FATAL(l,"jack server not running?\n");
			exit(1);
        }
        LOG4CXX_INFO(l,"Created new jack client "<< dst_client_str.str());
        if (jack_set_process_callback(mainclient, JackInput::jack_proc, NULL)) {
			LOG4CXX_FATAL(l,"Failed to set process registration callback for " << dst_client_str.str());
        }
        LOG4CXX_DEBUG(l,"Created process callback jack_proc for "<< dst_client_str.str());
        if (jack_activate (mainclient)) { LOG4CXX_FATAL(l,"cannot activate client"); }
        LOG4CXX_INFO(l,"Activated jack client "<< dst_client_str.str());
    }
    if (new_client) {
        LOG4CXX_INFO(l,"NEW Jack client requested");
    } else {
        LOG4CXX_DEBUG(l, "Jack client requested, returned old one");
    }
    return mainclient;
};

JackInput::JackInput(const void * args, all_cfg_t *cfg) {
    log4cxx::LoggerPtr l(log4cxx::Logger::getLogger("input.jack"));

    data_in;
    pthread_mutex_init(&data_mutex, NULL);
	pthread_cond_init(&condition_cond, NULL);
    jack_client_t *client = get_client();

    char *src_port_name = (char*) args;
    src_port = jack_port_by_name(client, src_port_name);

    SAMPLE_RATE = jack_get_sample_rate (client);
    cfg->first["WAVE"] = (int)SAMPLE_RATE * cfg->first["minwavelen"];
    cfg->first["CHUNKSIZE"] = cfg->first["chunklen"] * (int)SAMPLE_RATE;

    char shortname[15];
    if (jack_port_name_size() <= 15) {
        LOG4CXX_FATAL(l,"Too short jack port name supported");
        exit(1);
    }

    sprintf(shortname, "input_%d", ++JackInput::number_of_clients);
    dst_port = jack_port_register (client, shortname, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    dst_port_name = string(jack_port_name(dst_port));

    if (jack_connect(client, src_port_name, jack_port_name(dst_port))) {
        LOG4CXX_FATAL(l,"Failed to connect "<<src_port_name<<" to "<<jack_port_name(dst_port)<<", exiting.\n");
		exit(1);
    }

    LOG4CXX_DEBUG(l,"Locking JackInput mutex to insert instance to jackInputs");
    pthread_mutex_lock(&jackInputsMutex);
    jackInputs.insert( pair<string,JackInput*>(dst_port_name,this) );
    pthread_mutex_unlock(&jackInputsMutex);
    LOG4CXX_DEBUG(l,"jack port to jackInputs inserted, port ready to work");
};


JackInput::~JackInput() {
    // Delete input port
    string logger_name ("input.jack."); 
    logger_name += string(dst_port_name) + ".destructor";
    log4cxx::LoggerPtr l(log4cxx::Logger::getLogger(logger_name));

    LOG4CXX_DEBUG(l, "JackInput destructor called");

    LOG4CXX_DEBUG(l, "Disconnecting ports");
    if (jack_port_unregister(get_client(), dst_port)) {
        LOG4CXX_ERROR(l, "Failed to remove port "<<jack_port_name(src_port));
    }

    pthread_mutex_lock(&jackInputsMutex);
    jackInputs.erase(this->dst_port_name);
    pthread_mutex_unlock(&jackInputsMutex);

    LOG4CXX_DEBUG(l, "Deleted JackInput from jackInputs, ports are closed, leaving destructor");
}

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
    log4cxx::LoggerPtr l(log4cxx::Logger::getLogger("input.wav"));

    _fh = fopen(infile, "rb");
    if (_fh == NULL) {
        LOG4CXX_FATAL(l,"Failed to open file "<<infile<<", exiting");
		exit(1);
    }

    // Read bytes [0-3] and [8-11], they should be "RIFF" and "WAVE" in ASCII
    char header[5];
    fread(header, 1, 4, _fh);

    char sample[] = "RIFF";
    if (! check_sample(sample, header) ) {
        LOG4CXX_FATAL(l, "RIFF header not found in "<< infile <<", exiting");
		exit(1);
    }
    // Checking for compression code (21'st byte is 01, 22'nd - 00, little-endian notation
    uint16_t tmp[2]; // two-byte integer
    fseek(_fh, 0x14, 0); // offset = 20 bytes
    fread(&tmp, 2, 2, _fh); // Reading two two-byte samples (comp. code and no. of channels)
    if ( tmp[0] != 1 ) {
        LOG4CXX_FATAL(l, "Only PCM(1) supported, compression code given: " << tmp[0]);
		exit(1);
    }
    // Number of channels must be "MONO"
    if ( tmp[1] != 1 ) {
        LOG4CXX_FATAL(l, "Only MONO supported, given number of channels: " << tmp[1]);
		exit(1);
    }
    fread(&SAMPLE_RATE, 2, 1, _fh); // Single two-byte sample
    cfg->first["WAVE"] = (int)SAMPLE_RATE * cfg->first["minwavelen"];
    cfg->first["CHUNKSIZE"] = cfg->first["chunklen"] * (int)SAMPLE_RATE;
    fseek(_fh, 0x22, 0);
    uint16_t BitsPerSample;
    fread(&BitsPerSample, 1, 2, _fh);
    if (BitsPerSample != 16) {
        LOG4CXX_FATAL(l, "Only 16-bit WAVs supported, given: " << BitsPerSample);
		exit(1);
    }
    // Get data chunk size here
    fread(header, 1, 4, _fh);
    strcpy(sample, "data");

    if (! check_sample(sample, header)) {
        LOG4CXX_FATAL (l,"data chunk not found in byte offset=36, file corrupted.");
		exit(1);
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


