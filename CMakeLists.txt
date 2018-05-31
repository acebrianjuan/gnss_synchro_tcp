include_directories(
        ${CMAKE_SOURCE_DIR}/src/libs
        ${Boost_INCLUDE_DIRS}
     )
     
ADD_EXECUTABLE(tcp_client ${CMAKE_SOURCE_DIR}/tcp_client_example/client.cc)
target_link_libraries(tcp_client pthread ${Boost_LIBRARIES}) 

ADD_EXECUTABLE(tcp_server ${CMAKE_SOURCE_DIR}/tcp_client_example/server.cc)
target_link_libraries(tcp_server pthread ${Boost_LIBRARIES}) 