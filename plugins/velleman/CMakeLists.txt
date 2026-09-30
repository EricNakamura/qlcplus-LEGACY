project(velleman)

add_subdirectory(src)
if(QLC_BUILD_TESTS)
    add_subdirectory(test)
endif()
