cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)

set (PROJECT 
	server)
 
set (SOURCES
	main.cpp
    ServerClass.cpp)

set (HEADERS
    ../common.h
    ServerClass.h)

set (BOOST_COMPONENTS 
	thread
	filesystem)
 
set (Boost_USE_STATIC_LIBS ON)
 
set (Boost_USE_MULTITHREADED ON)
 
project (${PROJECT})

find_package (Threads)
 
find_package (Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
 

add_executable (${PROJECT} ${SOURCES} ${HEADERS})
 
target_link_libraries (${PROJECT} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})