cmake_minimum_required(VERSION 2.8)

project(my_buffer_test)
#sset_target_properties(my_buffer_test PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")

add_executable(${PROJECT_NAME} "main.cpp")

include_directories(../../include)

set_target_properties(my_buffer_test PROPERTIES
            CXX_STANDARD 98
            CXX_EXTENSIONS OFF
            )
