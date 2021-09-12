add_definitions("-Wall -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
aux_source_directory(. HTTP_SRCS)
# 指定生成目标
add_executable(${PROJECT_NAME} ${HTTP_SRCS})
# 添加链接库
target_link_libraries(${PROJECT_NAME} libNet)