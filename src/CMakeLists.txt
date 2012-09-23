configure_file (
    "${PROJECT_SOURCE_DIR}/src/config.h.in"
    "${PROJECT_BINARY_DIR}/src/config.h"
    )

set(srcs
    main.cpp soundpatty.cpp aggregate.cpp fileinput.cpp input.cpp logger.cpp)

if(${HAVE_JACK})
    set(srcs ${srcs} jackinput.cpp)
endif(${HAVE_JACK})

add_executable (soundpatty ${srcs})

target_link_libraries (soundpatty ${extra_libs})
