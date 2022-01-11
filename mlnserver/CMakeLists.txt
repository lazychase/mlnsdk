find_package(cpprestsdk REQUIRED)


add_compile_definitions(BOOST_ASIO_HAS_MOVE)
add_compile_definitions(BOOST_THREAD_PROVIDES_FUTURE)
add_compile_definitions(BOOST_THREAD_PROVIDES_FUTURE_UNWRAP)
add_compile_definitions(BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION)

add_compile_definitions(MLN_NET_USE_JSONPARSER_CPPREST)

#ADD_DEFINITIONS(-DBoost_USE_STATIC_LIBS=ON)
SET(Boost_ADDITIONAL_VERSIONS "1.78" "1.78.0")

if(WIN32)
    SET(Boost_ROOT "C:\\local\\boost_1_78_0")
    SET(Boost_LIBRARY_DIR "${Boost_ROOT}\\lib64-msvc-14.2")
    SET(Boost_INCLUDE_DIR ${Boost_ROOT})
    SET(Boost_USE_STATIC_LIBS        ON)  # only find static libs
endif()
# set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
# set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
# set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost REQUIRED system date_time thread chrono)
if(Boost_FOUND)
    message(STATUS "---------------------")
    message(STATUS "Boost_FOUND: ${Boost_FOUND}")
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "---------------------")

    include_directories(${Boost_INCLUDE_DIRS} 
        "${CMAKE_SOURCE_DIR}/mlnserver/"
        "${CMAKE_SOURCE_DIR}/include/"
    )
    message("${CMAKE_SOURCE_DIR}/include/")

    add_executable(mlnserver
        main.cpp
        serviceEventReceiver.cpp
    )

    target_link_libraries(mlnserver
        ${Boost_LIBRARIES}
        cpprestsdk::cpprest
    )
    
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/netconfig.json
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin )
    
elseif()
    message("Boost NOT Found!")
endif()

