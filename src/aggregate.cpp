#include <stdlib.h>
#include <stdio.h>
#include "aggregate.h"
#include <string.h>
#include <sstream>
#include <iostream>

using namespace std;

string percent(deque<treshold_t> d, vector<sVolumes> vol, double len) {
    stringstream ret("");
    vector<double> agg = vector<double>(vol.size(), 0);

    for (deque<treshold_t>::iterator it = d.begin(); it != d.end(); it++) {
        //printf("%d: %f\n", it->r, it->sec);
        agg[it->r] += it->sec;
    }

    for (unsigned i = 0; i < vol.size(); i++) {
        if (agg[i] != 0) {
            double percent = agg[i] / len*100;
            ret << i << ": " << agg[i] << ", " << percent << "%" << endl;
            //cout << ret.str();
        } else {
            ret << i << ": " << 0 << ", " << 0 << "%" << endl;
        }
    }
    return ret.str();
}
