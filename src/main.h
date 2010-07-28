/*  
 *  main.h
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

#ifndef __SP_MAIN_H_INCLUDED__
#define __SP_MAIN_H_INCLUDED__

#include <math.h>
#include <algorithm>
#include <stdlib.h>
#include <cstdio>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <pthread.h>
#ifdef HAS_JACK
#include <jack/jack.h>
#endif // HAS_JACK

#include <sys/types.h>
#include <unistd.h>

using namespace std;

#include "config.h"
#include "types.h"
#include "logger.h"

#define SRC_WAV 0
#define SRC_JACK_ONE 1
#define SRC_JACK_AUTO 2
#define ACTION_DUMP 0
#define ACTION_CATCH 1

#define BUFFERSIZE 1

#endif // __SP_MAIN_H_INCLUDED__
