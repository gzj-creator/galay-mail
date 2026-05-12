project(galay-mail-test)

file(GLOB SRC_LIST CONFIGURE_DEPENDS *.cc)

foreach(SRC ${SRC_LIST})
    get_filename_component(FILE_NAME ${SRC} NAME_WE)
    add_executable(${FILE_NAME} ${SRC})
    set_property(TARGET ${FILE_NAME} PROPERTY NO_SYSTEM_FROM_IMPORTED ON)
    target_link_libraries(${FILE_NAME} PRIVATE galay-mail)
    target_include_directories(${FILE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/galay-mail)
    set_target_properties(${FILE_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        INSTALL_RPATH $ORIGIN/../lib
    )
    add_test(NAME ${FILE_NAME} COMMAND ${FILE_NAME})
    set_tests_properties(${FILE_NAME} PROPERTIES LABELS "unit;mail")
endforeach()
