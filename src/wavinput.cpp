/*
 *  wavinput.cpp
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

#include "wavinput.h"

int WavInput::giveInput(buffer_t *buf_prop) {
    if (feof(_fh)) {
        return 0;
    }
	int16_t buf [SAMPLE_RATE * BUFFERSIZE];
    size_t read_size = fread((void*)buf, 2, SAMPLE_RATE * BUFFERSIZE, _fh);

	buf_prop->buf = (sample_t*) calloc(read_size, sizeof(sample_t));
	for (unsigned int i = 0; i < read_size; i++) {
		buf_prop->buf[i] = (sample_t)buf[i];
	}
    buf_prop->nframes = read_size;
    return 1;
};



/// WavInput here ///
int WavInput::process_headers(const char * infile, all_cfg_t *cfg) {
    _fh = fopen(infile, "rb");
    if (_fh == NULL) {
        LOG_FATAL("Failed to open file %s, exiting", infile);
		exit(1);
    }

    // Read bytes [0-3] and [8-11], they should be "RIFF" and "WAVE" in ASCII
    char header[5];
    fread(header, 1, 4, _fh);

    char sample[] = "RIFF";
    if (! check_sample(sample, header) ) {
        LOG_FATAL( "RIFF header not found in %s, exiting", infile);
		exit(1);
    }
    // Checking for compression code (21'st byte is 01, 22'nd - 00, little-endian notation
    uint16_t tmp[2]; // two-byte integer
    fseek(_fh, 0x14, 0); // offset = 20 bytes
    fread(&tmp, 2, 2, _fh); // Reading two two-byte samples (comp. code and no. of channels)
    if ( tmp[0] != 1 ) {
        LOG_FATAL( "Only PCM(1) supported, compression code given: %d", tmp[0]);
		exit(1);
    }
    // Number of channels must be "MONO"
    if ( tmp[1] != 1 ) {
        LOG_FATAL( "Only MONO supported, given number of channels: %d", tmp[1]);
		exit(1);
    }
    fread(&SAMPLE_RATE, 2, 1, _fh); // Single two-byte sample
    /*
    cfg->first["WAVE"] = (int)SAMPLE_RATE * cfg->first["minwavelen"];
    cfg->first["CHUNKSIZE"] = cfg->first["chunklen"] * (int)SAMPLE_RATE;
    */

    //LOG_INFO("WAVE: %.6f, CHUNKSIZE: %.6f", cfg->first["WAVE"], cfg->first["CHUNKSIZE"]);

    fseek(_fh, 0x22, 0);
    uint16_t BitsPerSample;
    fread(&BitsPerSample, 1, 2, _fh);
    if (BitsPerSample != 16) {
        LOG_FATAL( "Only 16-bit WAVs supported, given: %d", BitsPerSample);
		exit(1);
    }
    // Get data chunk size here
    fread(header, 1, 4, _fh);
    strcpy(sample, "data");

    if (! check_sample(sample, header)) {
        LOG_FATAL ("data chunk not found in byte offset=36, file corrupted.");
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
	LOG_DEBUG("Called WavInput constructor, filename: %s", (char*)args);
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


