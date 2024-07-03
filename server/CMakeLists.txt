cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)

set (PROJECT 
	server)
 
set (SOURCES
	main.cpp
    ServerClass.cpp
	ClientThread.cpp)

set (HEADERS
    ../common.h
    ServerClass.h
	ClientThread.h)

set (BOOST_COMPONENTS 
	thread
	filesystem)
 
set (Boost_USE_STATIC_LIBS ON)
 
set (Boost_USE_MULTITHREADED ON)
 
project (${PROJECT})

find_package (Threads)
 
find_package (Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
 
find_package (glog 0.7.1 REQUIRED)

find_package(libconfig++ REQUIRED)

add_executable (${PROJECT} ${SOURCES} ${HEADERS})
 
target_link_libraries (${PROJECT} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_link_libraries (${PROJECT} glog::glog)
target_link_libraries(${PROJECT} ${LIBCONFIG_LIBRARIES})
include_directories(${PROJECT} ${LIBCONFIG_INCLUDE_DIR})