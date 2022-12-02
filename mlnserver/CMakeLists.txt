if(NOT DEFINED MYSQL_INCLUDE_DIR)
    if(WIN32)
        set(MYSQL_CONN_ROOT "C:/local/mysql-connector-c++-8.0.25-winx64/")
        set(MYSQL_CONN_INCLUDE_DIR "${MYSQL_CONN_ROOT}/include/")
        set(MYSQL_CONN_LIB_DIR "${MYSQL_CONN_ROOT}/lib64/vs14/debug/")
    else()
        set(MYSQL_CONN_INCLUDE_DIR "/usr/include/mysql-cppconn-8")
    endif()

endif(NOT DEFINED MYSQL_INCLUDE_DIR)

if(NOT DEFINED OPEN_SSL_BIN_DIR)
    if(WIN32)
        set(OPEN_SSL_BIN_DIR "C:/local/vcpkg/packages/openssl_x64-windows/debug/bin/")
    endif()
endif(NOT DEFINED OPEN_SSL_BIN_DIR)


add_compile_definitions(BOOST_ASIO_HAS_MOVE)
add_compile_definitions(BOOST_THREAD_PROVIDES_FUTURE)
add_compile_definitions(BOOST_THREAD_PROVIDES_FUTURE_UNWRAP)
add_compile_definitions(BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION)

add_compile_definitions(MLN_NET_USE_JSONPARSER_BOOSTJSON)
add_compile_definitions(MLN_USE_BEAST_WEBSOCKET)


#ADD_DEFINITIONS(-DBoost_USE_STATIC_LIBS=ON)
#SET(Boost_ADDITIONAL_VERSIONS "1.78" "1.78.0")

if(WIN32)
    #SET(Boost_ROOT "C:\\local\\boost_1_78_0")
    #SET(Boost_LIBRARY_DIR "${Boost_ROOT}\\lib64-msvc-14.3")
    #SET(Boost_INCLUDE_DIR ${Boost_ROOT})
    #SET(Boost_USE_STATIC_LIBS        ON)  # only find static libs
endif()
# set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
# set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
# set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost 1.80 REQUIRED system date_time thread chrono json)
if(Boost_FOUND)
    message(STATUS "---------------------")
    message(STATUS "Boost_FOUND: ${Boost_FOUND}")
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "---------------------")

    include_directories(
        ${MYSQL_CONN_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS} 
        "${CMAKE_SOURCE_DIR}/mlnserver/"
        "${CMAKE_SOURCE_DIR}/include/"
    )

if(WIN32)
    link_directories(
        ${MYSQL_CONN_LIB_DIR}
        "C:\\Program Files (x86)\\aws-cpp-sdk-all\\bin"
    )
endif()

    add_executable(mlnserver
        main.cpp
        serviceEventReceiver.cpp
    )

    target_link_libraries(mlnserver
        #mysqlcppconn8
        ${Boost_LIBRARIES}
    )
    
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/netconfig.json
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin )
    
elseif()
    message("Boost NOT Found!")
endif()

