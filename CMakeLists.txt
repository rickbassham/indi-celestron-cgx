########### Celestron CGX INDI driver ##############
project(indi-celestron-cgx C CXX)
cmake_minimum_required(VERSION 2.8)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

find_package(INDI 1.8 REQUIRED)
find_package(Nova REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GSL REQUIRED)

set(CCGX_VERSION_MAJOR 2)
set(CCGX_VERSION_MINOR 0)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_celestron_cgx.xml.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/indi_celestron_cgx.xml
)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR})
include_directories( ${EV_INCLUDE_DIR})

include(CMakeCommon)

add_executable(
    indi_celestron_cgx
    auxproto.cpp
    celestroncgx.cpp
    simplealignment.cpp
)

target_link_libraries(
    indi_celestron_cgx
    ${INDI_LIBRARIES}
    ${NOVA_LIBRARIES}
    ${GSL_LIBRARIES}
)

install(TARGETS indi_celestron_cgx RUNTIME DESTINATION bin)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/indi_celestron_cgx.xml
    DESTINATION ${INDI_DATA_DIR}
)
