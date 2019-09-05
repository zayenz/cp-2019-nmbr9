find_library(GECODE Gecode)
if (NOT GECODE)
    message(FATAL_ERROR "Gecode not found")
endif()

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
        "${PROJECT_SOURCE_DIR}/src/config.h.in"
        "${PROJECT_BINARY_DIR}/src/config.h"
)

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories("${PROJECT_BINARY_DIR}/src/")
include_directories ("${PROJECT_SOURCE_DIR}/nmbr9")
include_directories("${GECODE}")
add_subdirectory (nmbr9)

add_executable(nmbr9-cli main.cpp nmbr9/base.h nmbr9/base.cpp)
target_link_libraries (nmbr9-cli ${GECODE} Nmbr9Lib)
