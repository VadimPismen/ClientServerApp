cmake_minimum_required(VERSION 3.7)
project(ClientServerApp)

add_subdirectory(server)
add_subdirectory(client)

add_custom_target(ClientServerApp)
add_dependencies(ClientServerApp server client)

