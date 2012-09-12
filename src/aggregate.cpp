#include <stdlib.h>
#include <stdio.h>
#include "aggregate.h"
#include <string.h>
#include <sstream>

using namespace std;

const char * percent(deque<treshold_t> d, vector<sVolumes> vol) {
    stringstream ret("");
    vector<double> agg = vector<double>(vol.size(), 0);
    for (deque<treshold_t>::iterator it = d.begin(); it != d.end(); it++) {
        printf("%d: %f\n", it->r, it->sec);
        //agg[it->r] += it->sec;
    }
    for (unsigned i = 0; i < agg.size(); i++) {
        ret << i << ": " << agg[i] << endl;
    }
    return ret.str().c_str();
}
