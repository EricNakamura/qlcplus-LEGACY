project(ui)

add_subdirectory(src)
if(QLC_BUILD_TESTS)
    add_subdirectory(test)
endif()

add_library(qlcplusui_ui STATIC
)
target_link_libraries(qlcplusui_ui PUBLIC Qt6::Widgets)

target_include_directories(qlcplusui_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)