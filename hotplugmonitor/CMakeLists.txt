project(hotplugmonitor)

if(NOT ANDROID AND NOT IOS)
    add_subdirectory(src)
    if(QLC_BUILD_TESTS)
        add_subdirectory(test)
    endif()
endif()
