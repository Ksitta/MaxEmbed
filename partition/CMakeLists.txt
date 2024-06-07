cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fmax-errors=2 -Wall -O3 -g")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(folly REQUIRED)
find_package(gflags REQUIRED)

add_library(partition STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/partition.cpp
)

target_link_libraries(partition PUBLIC
${FOLLY_LIBRARIES}
OpenMP::OpenMP_CXX
tbb
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(OpenMP REQUIRED)
find_package(TBB CONFIG REQUIRED)
add_executable(partition_step
./src/partition_step.cpp
)

target_link_libraries(partition_step PRIVATE
tbb
OpenMP::OpenMP_CXX
partition
TBB::tbb
)

add_executable(replication_step src/replication_step.cpp)

target_link_libraries(replication_step PUBLIC
Folly::folly
OpenMP::OpenMP_CXX
TBB::tbb
partition
)

add_subdirectory(preprocess/criteoTB_script)

add_executable(shrink_replication src/shrink_replication.cpp)
target_link_libraries(shrink_replication PUBLIC partition)

add_executable(trans_query src/trans_query.cpp)