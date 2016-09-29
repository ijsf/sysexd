
file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

init_target (rtmidi)

build_vendor (${TARGET_NAME} ${SOURCE_FILES})

link_boost ()
final_target ()
