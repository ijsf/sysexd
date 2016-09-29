
# Adds the given folder_name into the source files of the current project. 
# Use this macro when your module contains .cpp and .h files in several subdirectories.
# Your sources variable needs to be WSPP_SOURCE_FILES and headers variable WSPP_HEADER_FILES.
macro(add_source_folder folder_name)
    file(GLOB H_FILES_IN_FOLDER_${folder_name} ${folder_name}/*.hpp ${folder_name}/*.h)
    file(GLOB CPP_FILES_IN_FOLDER_${folder_name} ${folder_name}/*.cpp ${folder_name}/*.c)
    source_group("Header Files\\${folder_name}" FILES ${H_FILES_IN_FOLDER_${folder_name}})
    source_group("Source Files\\${folder_name}" FILES ${CPP_FILES_IN_FOLDER_${folder_name}})
    set(WSPP_HEADER_FILES ${WSPP_HEADER_FILES} ${H_FILES_IN_FOLDER_${folder_name}})
    set(WSPP_SOURCE_FILES ${WSPP_SOURCE_FILES} ${CPP_FILES_IN_FOLDER_${folder_name}})
endmacro()

# Initialize target.
macro (init_target NAME)
    set (TARGET_NAME ${NAME})
    message ("** " ${TARGET_NAME})

    # Include our own module path. This makes #include "x.h" 
    # work in project subfolders to include the main directory headers.
    include_directories (${CMAKE_CURRENT_SOURCE_DIR})
endmacro ()

# Build executable for executables
macro (build_executable TARGET_NAME)
    set (TARGET_LIB_TYPE "EXECUTABLE")
    message (STATUS "-- Build Type:")
    message (STATUS "       " ${TARGET_LIB_TYPE})

    add_executable (${TARGET_NAME} ${ARGN})

    include_directories (${SYSEXD_ROOT} ${SYSEXD_INCLUDE})

    target_link_libraries(${TARGET_NAME} ${SYSEXD_PLATFORM_LIBS})

    set_target_properties (${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SYSEXD_BIN})
    set_target_properties (${TARGET_NAME} PROPERTIES DEBUG_POSTFIX d)
endmacro ()

# Build executable and register as test
macro (build_test TARGET_NAME)
    build_executable (${TARGET_NAME} ${ARGN})

    if (${CMAKE_VERSION} VERSION_LESS 3)
        message(WARNING "CMake too old to register ${TARGET_NAME} as a test")
    else ()
        add_test(NAME ${TARGET_NAME} COMMAND $<TARGET_FILE:${TARGET_NAME}>)
    endif ()
endmacro ()

# Finalize target for all types
macro (final_target)
    if ("${TARGET_LIB_TYPE}" STREQUAL "EXECUTABLE")
        install (TARGETS ${TARGET_NAME} 
                 RUNTIME DESTINATION "bin" 
                 CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})
    endif ()

    # install headers, directly from current source dir and look for subfolders with headers
    file (GLOB_RECURSE TARGET_INSTALL_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hpp)
    foreach (hppfile ${TARGET_INSTALL_HEADERS})
      get_filename_component (currdir ${hppfile} PATH)
      install (FILES ${hppfile} DESTINATION "include/${TARGET_NAME}/${currdir}")
    endforeach()
endmacro ()

macro (link_boost)
    target_link_libraries (${TARGET_NAME} ${Boost_LIBRARIES})
endmacro ()

macro (link_openssl)
    target_link_libraries (${TARGET_NAME} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
endmacro ()

macro (link_zlib)
	target_link_libraries (${TARGET_NAME} ${ZLIB_LIBRARIES})
endmacro ()

macro (include_subdirs PARENT)
    file (GLOB SDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${PARENT}/*")
    foreach (SUBDIR ${SDIRS})
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt")
            add_subdirectory ("${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}")
        endif ()
    endforeach ()
endmacro()
