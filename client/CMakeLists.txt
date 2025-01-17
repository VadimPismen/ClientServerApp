cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)

set (PROJECT 
	client)
 
set (SOURCES
	main.cpp
    ClientClass.cpp)

set (HEADERS
    ../common.h
    ClientClass.h)

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

add_executable (${PROJECT} ${SOURCES} ${HEADERS})
 
target_link_libraries (${PROJECT} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_link_libraries (${PROJECT} glog::glog)

find_package(libconfig REQUIRED)
find_package(libconfig++ REQUIRED)
if (LIBCONFIG_FOUND)
    target_link_libraries(${PROJECT} config++)
    include_directories(${PROJECT} ${LIBCONFIG_INCLUDE_DIR})
endif()