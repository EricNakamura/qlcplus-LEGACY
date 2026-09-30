project(midi)

add_subdirectory(src)
if(QLC_BUILD_TESTS AND NOT ANDROID AND NOT IOS)
    add_subdirectory(test)
endif()
