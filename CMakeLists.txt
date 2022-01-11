cmake_minimum_required (VERSION 3.18)
project ("mlnserver")

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

INCLUDE (CheckIncludeFiles)
include(ExternalProject)
include(spdlog)

if(MSVC)
    add_definitions(/bigobj)
endif(MSVC)

include_directories(${SPDLOG_INCLUDE_DIR} 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/mlnserver/
    ${CMAKE_SOURCE_DIR}/net/
)


add_subdirectory("mlnserver")
add_dependencies(mlnserver spdlog)