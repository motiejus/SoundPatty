configure_file (
    "${PROJECT_SOURCE_DIR}/src/config.h.in"
    "${PROJECT_BINARY_DIR}/src/config.h"
    )

set(srcs
    main.cpp soundpatty.cpp aggregate.cpp fileinput.cpp input.cpp logger.cpp)

if(HAVE_JACK AND HAVE_PTHREAD)
    set(srcs ${srcs} jackinput.cpp)
endif(HAVE_JACK AND HAVE_PTHREAD)

add_executable (soundpatty ${srcs})
target_link_libraries (soundpatty ${extra_libs})

# TODO: symlink on Unix
file(COPY soundpatty DESTINATION ${PROJECT_SOURCE_DIR})


add_test (soundpattyStarts soundpatty)

#file(DOWNLOAD
#    "http://github.com/downloads/Motiejus/SoundPatty/sample.wav"
#    "${PROJECT_SOURCE_DIR}/sample.wav"
#    EXPECTED_MD5 2c313f0691f872d50d71399b78318fe0
#    )
#
#set_property(TEST soundpattyStarts APPEND PROPERTY DEPENDS sample.wav)
