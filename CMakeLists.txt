
############ Setup project and cmake

# Project name
project (sysexd)

# Minimum cmake requirement. We should require a quite recent
# cmake for the dependency find macros etc. to be up to date.
cmake_minimum_required (VERSION 2.6)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Include our cmake macros
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (CMakeHelpers)

set (SYSEXD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set (SYSEXD_INCLUDE ${SYSEXD_ROOT}/include)
set (SYSEXD_BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR})
set (SYSEXD_BIN ${SYSEXD_BUILD_ROOT}/bin)

set (Boost_FIND_REQUIRED TRUE)
set (Boost_FIND_QUIETLY TRUE)
set (Boost_DEBUG FALSE)
set (Boost_USE_MULTITHREADED TRUE)
set (Boost_ADDITIONAL_VERSIONS "1.39.0" "1.40.0" "1.41.0" "1.42.0" "1.43.0" "1.44.0" "1.46.1")

set (SYSEXD_BOOST_LIBS system thread)

find_package (Boost 1.39.0 COMPONENTS "${SYSEXD_BOOST_LIBS}")

if (Boost_FOUND)
    # Boost is a project wide global dependency.
    include_directories (${Boost_INCLUDE_DIRS})
    link_directories (${Boost_LIBRARY_DIRS})

    # Pretty print status
    message (STATUS "-- Include Directories")
    foreach (include_dir ${Boost_INCLUDE_DIRS})
        message (STATUS "       " ${include_dir})
    endforeach ()
    message (STATUS "-- Library Directories")
    foreach (library_dir ${Boost_LIBRARY_DIRS})
        message (STATUS "       " ${library_dir})
    endforeach ()
    message (STATUS "-- Libraries")
    foreach (boost_lib ${Boost_LIBRARIES})
        message (STATUS "       " ${boost_lib})
    endforeach ()
    message ("")
else ()
    message (FATAL_ERROR "Failed to find required dependency: boost")
endif ()

# vendor libraries
set (SYSEXD_VENDOR_LIBRARIES rtmidi)
add_subdirectory (vendor/rtmidi)

# application
add_subdirectory (src)