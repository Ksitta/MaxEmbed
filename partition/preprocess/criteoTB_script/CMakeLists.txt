cmake_minimum_required(VERSION 3.0)
project(preprocess)
set(CMAKE_CXX_STANDARD 11)
# add -O3
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(read_and_trans read_and_trans.cpp)
add_executable(merge_result merge_result.cpp)
add_executable(split_train_test split_train_test.cpp)