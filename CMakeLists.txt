cmake_minimum_required (VERSION 2.6)
project (soundpatty)

include (CheckIncludeFile)

check_include_file("sys/inotify.h" HAVE_INOTIFY)

check_include_file("sox.h" HAVE_SOX)
if(${HAVE_SOX})
    set(extra_libs ${extra_libs} sox)
endif(${HAVE_SOX})

check_include_file("pthread.h" HAVE_PTHREAD)
if(${HAVE_PTHREAD})
    set(extra_libs ${extra_libs} pthread)
endif(${HAVE_PTHREAD})

check_include_file("jack/jack.h" HAVE_JACK)
if(${HAVE_JACK})
    set(extra_libs ${extra_libs} jack)
endif(${HAVE_JACK})

add_subdirectory(src)

ENABLE_TESTING()
