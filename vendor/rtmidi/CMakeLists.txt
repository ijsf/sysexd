
file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

init_target (rtmidi)

build_vendor (${TARGET_NAME} ${SOURCE_FILES})

if (APPLE)
  add_definitions("-D__MACOSX_CORE__")
  target_link_libraries(${TARGET_NAME} ${COREMIDI} ${COREFOUNDATION} ${COREAUDIO})
endif()

link_boost ()
final_target ()
