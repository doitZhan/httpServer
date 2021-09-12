add_definitions("-Wall -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

aux_source_directory(. DIR_LIB_SRCS)
add_library(libNet ${DIR_LIB_SRCS})
find_package(Threads)
target_link_libraries(libNet ${CMAKE_THREAD_LIBS_INIT})