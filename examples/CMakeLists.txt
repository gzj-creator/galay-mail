project(galay-mail-examples)

file(GLOB INCLUDE_EXAMPLES CONFIGURE_DEPENDS include/*.cc)

foreach(SRC ${INCLUDE_EXAMPLES})
    get_filename_component(FILE_NAME ${SRC} NAME_WE)
    add_executable(${FILE_NAME} ${SRC})
    target_link_libraries(${FILE_NAME} PRIVATE galay-mail)
    target_include_directories(${FILE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/galay-mail)
    set_target_properties(${FILE_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
    )
endforeach()
