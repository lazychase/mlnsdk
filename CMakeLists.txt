cmake_minimum_required (VERSION 3.18)
project ("mlnserver")

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)
set(Boost_NO_WARN_NEW_VERSIONS 1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

INCLUDE (CheckIncludeFiles)
include(ExternalProject)
include(spdlog)
include(struct-mapping)

if(MSVC)
    add_definitions(/bigobj)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif(MSVC)

include_directories(
    ${SPDLOG_INCLUDE_DIR} 
    ${STRUCT_MAPPING_INCLUDE_DIR} 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/mlnserver/
    ${CMAKE_SOURCE_DIR}/net/
)


add_subdirectory("mlnserver")
add_dependencies(mlnserver spdlog struct_mapping)
