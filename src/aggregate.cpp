#include "aggregate.h"
#include <string.h>
#include <sstream>

using namespace std;

char * percent(deque<treshold_t> d, vector<sVolumes> vol) {
    stringstream ret("");
    vector<double> agg = vector<double>(vol.size(), 0);
    for (deque<treshold_t>::iterator it = d.begin(); it != d.end(); it++) {
        agg[it->r] += it->sec;
    }

    return strdup("bac\n");
}
