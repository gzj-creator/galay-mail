cmake_minimum_required(VERSION 3.20)
project(galay-mail VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(GALAY_MAIL_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
set(GALAY_MAIL_SIBLING_PREFIX_CANDIDATES
    "${GALAY_MAIL_REPO_ROOT}/../.galay-prefix/latest"
    "${GALAY_MAIL_REPO_ROOT}/../galay-kernel/install-local"
    "${GALAY_MAIL_REPO_ROOT}/../galay-kernel/_install-smoke"
)

foreach(_galay_mail_prefix IN LISTS GALAY_MAIL_SIBLING_PREFIX_CANDIDATES)
    if(EXISTS "${_galay_mail_prefix}")
        list(PREPEND CMAKE_PREFIX_PATH "${_galay_mail_prefix}")
    endif()
endforeach()
list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

if(NOT DEFINED galay-kernel_DIR OR galay-kernel_DIR STREQUAL "")
    foreach(_galay_mail_prefix IN LISTS GALAY_MAIL_SIBLING_PREFIX_CANDIDATES)
        if(EXISTS "${_galay_mail_prefix}/lib/cmake/galay-kernel/galay-kernel-config.cmake")
            set(galay-kernel_DIR
                "${_galay_mail_prefix}/lib/cmake/galay-kernel"
                CACHE PATH "Preferred sibling galay-kernel package directory" FORCE)
            break()
        endif()
    endforeach()
endif()

if(NOT DEFINED galay-ssl_DIR OR galay-ssl_DIR STREQUAL "")
    foreach(_galay_mail_prefix IN LISTS GALAY_MAIL_SIBLING_PREFIX_CANDIDATES)
        if(EXISTS "${_galay_mail_prefix}/lib/cmake/galay-ssl/galay-ssl-config.cmake")
            set(galay-ssl_DIR
                "${_galay_mail_prefix}/lib/cmake/galay-ssl"
                CACHE PATH "Preferred sibling galay-ssl package directory" FORCE)
            break()
        endif()
    endforeach()
endif()

include(cmake/option.cmake)

if(DEFINED BUILD_TESTS AND NOT DEFINED BUILD_TESTING)
    set(BUILD_TESTING ${BUILD_TESTS} CACHE BOOL "Build the testing tree." FORCE)
elseif(NOT DEFINED BUILD_TESTING)
    set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree." FORCE)
endif()

include(CTest)

configure_io_backend()

add_subdirectory(galay-mail)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
