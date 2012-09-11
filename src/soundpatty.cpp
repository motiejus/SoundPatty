/*
 *  soundpatty.cpp
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


#include "soundpatty.h"
#include "input.h"

void *SoundPatty::go_thread(void *args) {
    // This is the new function that is created in new thread
    SoundPatty *inst = (SoundPatty*) args;

    LOG_DEBUG("Launching SoundPatty::go");
    if (inst->go() == 2) {
        LOG_WARN("Timed out");
    }
    LOG_DEBUG("Terminating SoundPatty instance");
    // Process is over - either timeout or catch. Callbacks launched already.
    // Must terminate SoundPatty and Input instances
    delete inst->_input;
    delete inst;
    LOG_DEBUG("SoundPatty and Input instances deleted. Exiting thread");
    return NULL;
}

void SoundPatty::dump_out(const treshold_t args) { // STATIC
    printf ("%d;%.6f;%.6f\n", args.r, args.place, args.sec);
    fflush(stdout);
}


SoundPatty::SoundPatty(action_t action, Input *input, all_cfg_t *all_cfg, void *params_uni) {
    _action = action;
    _input = input;
    if (action == ACTION_DUMP) {
        //sp_params_dump_t *params = (sp_params_dump_t*)params_uni;
    } else if (action == ACTION_CAPTURE) {
        sp_params_capture_t *params = (sp_params_capture_t*)params_uni;
        exit_after_capture = params->exit_after_capture;
        vals = params->vals;
        _callback = params->fn;
    }

	cfg = all_cfg->first;
	volume = all_cfg->second;
    gSCounter = gMCounter = 0;

    _input->WAVE = _input->SAMPLE_RATE * cfg["minwavelen"];
    _input->CHUNKSIZE = cfg["chunklen"] * _input->SAMPLE_RATE;
}


all_cfg_t SoundPatty::read_cfg (const char * filename) {
	map<string,double> cfg;
	vector<sVolumes> volume;
    ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    try {
        file.open(filename);
    } catch (ifstream::failure e) {
        LOG_FATAL("Could not read config file %s", filename);
        pthread_exit(NULL);
    }

    string line;
    int x;
    while (! file.eof() ) {
        getline(file, line);
        x = line.find(":");
        if (x == -1) break; // Last line, exit
        istringstream i(line.substr(x+2));
        double tmp; i >> tmp;
        cfg[line.substr(0,x)] = tmp;
    }
    LOG_DEBUG("Read %d config values from %s", cfg.size(), filename);

    sVolumes tmp;
    tmp.head = tmp.tail = tmp.max = tmp.min = tmp.proc = 0;
    volume.assign(cfg.size(), tmp); // Assign a bit more then nescesarry
    int max_index = 0; // Number of different tresholds
    for(map<string, double>::iterator C = cfg.begin(); C != cfg.end(); C++) {
        // Failed to use boost::regex :(
        if (C->first.find("treshold") == 0) {
            istringstream tmp(C->first.substr(8));
            int i; tmp >> i;
            max_index = max(max_index, i);
            if (C->first.find("_min") != string::npos) {
                volume[i].min = C->second;
            } else {
                volume[i].max = C->second;
            }
        }
    }
    if (max_index == 0) {
        LOG_FATAL("ERROR. Length of vol array: 0\n");
    }
    volume.assign(volume.begin(), volume.begin()+max_index+1);
	return all_cfg_t(cfg, volume);
}


int SoundPatty::setInput(Input * input) {
    _input = input;
    return 0;
}


int SoundPatty::go() {

    string which_timeout (_action == ACTION_DUMP ?
            "sampletimeout" : "catchtimeout");
    buffer_t buf;

    while (_input->giveInput(&buf) != 0) { // Have pointer to data
        LOG_TRACE("Got buffer, length: %d", buf.nframes);
        treshold_t ret;

        for (unsigned int i = 0; i < buf.nframes; gSCounter++, i++) {
            sample_t cur = buf.buf[i]<0?-buf.buf[i]:buf.buf[i];
            if (search_patterns(cur, &ret))
            {
                /*
                   LOG_TRACE("Found pattern ("<<setw(3)<<ret.b<<") "
                   "("<< ret.r <<";"<< left << setw(1) << ret.place <<";"<<
                   setw(8) << ret.sec<<")");
                 */
                LOG_TRACE("Found pattern (%-.3f) %-.3f; %.6f; %.6f",
                        ret.b, ret.r, ret.place, ret.sec);

                if (_action == ACTION_DUMP)
                    SoundPatty::dump_out(ret);
                else if (_action == ACTION_AGGREGATE)
                    this->findings.push_back(ret);
                else if (_action == ACTION_CAPTURE)
                    if (SoundPatty::do_checking(ret))
                        // Caught pattern
                        if (exit_after_capture)
                            return 1;
            }
        }
        if (buf.delete_me) {
            delete(buf.buf);
        }

        if ((double)gSCounter/_input->SAMPLE_RATE > cfg[which_timeout]) {
            // Timeout. Return 2
            return 2;
        }
    }
    return 0;
}


vals_t SoundPatty::read_captured_values(const char * filename) {
    vals_t vals;

    ifstream file;
    file.open(filename);
    string line;
    for (int i = 0; !file.eof(); i++) {
        getline(file, line);
        if (line.size() == 0) break;
        vector<string> numbers = explode(";", line);
        istringstream num(numbers[0]);
        istringstream place(numbers[1]);
        istringstream range(numbers[2]);

        double tmp2;
        pair<pair<int, Range>,valsitm_t > tmp;
        num >> tmp2; tmp.first.first = tmp2; // Index in volume
        range >> tmp2; tmp.first.second = Range(tmp2);
        place >> tmp.second.place; // Place in the stream
        tmp.second.c = i; // Counter in the stream
        vals.insert(tmp);
    }
    return vals;
}


int SoundPatty::search_patterns (sample_t cur, treshold_t * ret) {
    int v = 0; // Counter for volume
    for (vector<sVolumes>::iterator V = volume.begin(); V != volume.end(); V++, v++) {
        if (V->min <= cur && cur <= V->max) {
            // ------------------------------------------------------------
            // If it's first item in this wave (proc = processing started)
            //
            if (!V->proc) {
                V->tail = gSCounter;
                V->proc = true;
            }
            // ------------------------------------------------------------
            // Here we are just normally in the wave.
            //
            V->head = gSCounter;
        } else { // We are not in the wave
            if (V->proc && (V->min < 0.001 || gSCounter - V->head > _input->WAVE)) {

                //------------------------------------------------------------
                // This wave is over
                //
                V->proc = false; // Stop processing for both cases: found and not

                if (gSCounter - V->tail >= _input->CHUNKSIZE) {
                    // ------------------------------------------------------------
                    // The previous chunk is big enough to be noticed
                    //
                    ret -> r = v;
                    ret -> place = (double)V->tail/_input->SAMPLE_RATE;
                    ret -> sec = (double)(V->head - V->tail)/_input->SAMPLE_RATE;
                    ret -> b = gMCounter++;
                    return 1;
                } 
                // ------------------------------------------------------------
                // Else it is too small, but we don't want to do anything in that case
                // So therefore we just say that wave processing is over
                //
            }
        }
    }
    return 0;
}


// --------------------------------------------------------------------------------
// This gets called every time there is a treshold found.
// Work array is global
//
// int r - id of treshold found (in config.cfg: treshold_(<?r>\d)_(min|max))
// double place - place of sample from the stream start (sec)
// double sec - length of a found sample (sec)
// int b - index (overall) of sample found
//
int SoundPatty::do_checking(const treshold_t tr) {
    //pair<vals_t::iterator, vals_t::iterator> pa = vals.equal_range(pair<int,double>(r,sec));
    // Manually searching for matching values because with that pairs equal_range doesnt work
    // Iterate through pa

    unsigned long b = tr.b; vals_t fina; // FoundInA
    Range demorange(tr.sec);
    pair<int,Range> sample(tr.r, demorange);
    for (vals_t::iterator it1 = vals.begin(); it1 != vals.end(); it1++) {
        if (it1->first == sample) {
            fina.insert(*it1);
        }
    }
    //------------------------------------------------------------
    // We put a indexes here that we use for continued threads
    // (we don't want to create a new "thread" with already
    // used length of a sample)
    //
    set<int> used_a; 

    //------------------------------------------------------------
    // Iterating through samples that match the found sample
    //
    for (vals_t::iterator in_a = fina.begin(); in_a != fina.end(); in_a++)
    {
        LOG_TRACE("%.6f (%d) matches %.6f (%d)",tr.sec,in_a->first.first,in_a->first.second.tm,in_a->second.c);
        int a = in_a->second.c;
        //------------------------------------------------------------
        // Check if it exists in our work array
        //
        int i = 0; // Work list counter
        for (list<workitm>::iterator w = work.begin(); w != work.end(); i++) {
            if (b - w->b > round(cfg["maxsteps"])) {
                LOG_TRACE("Work item %d removed, no match for a long time", i);
                work.erase(w); w = work.begin(); continue;
            }
            if (b == w->b || a - w->a > round(cfg["maxsteps"]) || w->a >= a) { w++; continue; }
            // ------------------------------------------------------------
            // We fit the "region" here. We either finished,
            // or just increasing len
            //
            w->a = a; w->b = b;
            w->trace.push_back(pair<int,unsigned long>(a,b));
            LOG_TRACE("Work item %d expanded to %d items", i, w->len+1);
            if (++(w->len) < round(cfg["matchme"])) { // Proceeding with the "thread"
                used_a.insert(a);
            } else { // This means the treshold is reached
                // This kind of function is called when the pattern is recognized
                _callback (_input->name, tr.place + tr.sec);
                return 1;
            }
            w++;
            // End of work iteration array
        }
        if (used_a.find(a) == used_a.end()) {
            work.push_back(workitm(a,b));
        }
    }
    return 0;
}


vector<string> explode(const string &delimiter, const string &str) { // Found somewhere on NET
    vector<string> arr;
    int strleng = str.length();
    int delleng = delimiter.length();
    if (delleng == 0)
        return arr; //no change
    int i = 0, k = 0;
    while (i < strleng) {
        int j = 0;
        while (i+j < strleng && j < delleng && str[i+j] == delimiter[j])
            j++;
        if (j == delleng) {
            arr.push_back(str.substr(k, i-k));
            i += delleng;
            k = i;
        } else {
            i++;
        }
    }
    arr.push_back(str.substr(k, i-k));
    return arr;
}


workitm::workitm(const int a, const unsigned long b) {
    this->a = a; this->b = b;
    len = 0;
    trace.push_back(pair<int,unsigned long>(a,b));
}
