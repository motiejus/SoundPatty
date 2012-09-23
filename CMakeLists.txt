cmake_minimum_required (VERSION 2.6)
project (soundpatty)
include (CheckIncludeFile)
include (CheckFunctionExists)

include_directories ("${PROJECT_SOURCE_DIR}/sox")
include_directories ("${PROJECT_SOURCE_DIR}/sox/src")
add_subdirectory (sox)
set (extra_libs ${extra_libs} libsox gsm lpc10)
set (HAVE_SOX 1)

check_include_file("sys/inotify.h" HAVE_INOTIFY)

check_include_file("pthread.h" HAVE_PTHREAD)
if(HAVE_PTHREAD)
    set(extra_libs ${extra_libs} pthread)
endif(HAVE_PTHREAD)

check_include_file("jack/jack.h" HAVE_JACK)
if(HAVE_JACK)
    set(extra_libs ${extra_libs} jack)
endif(HAVE_JACK)

check_function_exists(asprintf HAVE_ASPRINTF)

add_subdirectory(src)

ENABLE_TESTING()
