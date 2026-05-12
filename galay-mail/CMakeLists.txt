find_package(galay-kernel 4.0.0 CONFIG REQUIRED)
find_package(galay-ssl 2.0.0 CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
include(CMakePackageConfigHelpers)

file(GLOB SOURCES CONFIGURE_DEPENDS "base/*.cc" "protocol/*.cc" "async/*.cc")

if(NOT TARGET galay-kernel::galay-kernel)
    message(FATAL_ERROR "galay-kernel::galay-kernel target not found.")
endif()

if(NOT TARGET galay-ssl::galay-ssl)
    message(FATAL_ERROR "galay-ssl::galay-ssl target not found.")
endif()

add_library(${PROJECT_NAME} ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY NO_SYSTEM_FROM_IMPORTED ON)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        galay-kernel::galay-kernel
        galay-ssl::galay-ssl
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install(TARGETS ${PROJECT_NAME}
    EXPORT galay-mail-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

foreach(GALAY_MAIL_PUBLIC_SUBDIR async base protocol module)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${GALAY_MAIL_PUBLIC_SUBDIR}/
        DESTINATION include/galay-mail/${GALAY_MAIL_PUBLIC_SUBDIR}
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.inl"
    )
endforeach()

if(GALAY_MAIL_INSTALL_MODULE_INTERFACE)
    file(GLOB GALAY_MAIL_MODULE_INTERFACE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/module/*.cppm")
    if(GALAY_MAIL_MODULE_INTERFACE_FILES)
        install(FILES ${GALAY_MAIL_MODULE_INTERFACE_FILES}
            DESTINATION include/galay-mail/module
        )
    endif()
endif()

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/galay-mail-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/galay-mail-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/galay-mail-config.cmake"
    INSTALL_DESTINATION lib/cmake/galay-mail
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/galay-mail-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/galay-mail-config-version.cmake"
    DESTINATION lib/cmake/galay-mail
)

install(EXPORT galay-mail-targets
    FILE galay-mail-targets.cmake
    NAMESPACE galay-mail::
    DESTINATION lib/cmake/galay-mail
)

export(
    EXPORT galay-mail-targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/galay-mail-targets-build.cmake"
    NAMESPACE galay-mail::
)
