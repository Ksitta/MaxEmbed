cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fmax-errors=2 -Wall -O3 -g")

find_package(cachelib CONFIG REQUIRED)
find_package(PkgConfig)
find_package(argparse)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

pkg_search_module(SPDK REQUIRED IMPORTED_TARGET GLOBAL spdk_nvme spdk_env_dpdk)
message(STATUS SPDK_LIBRARIES ${SPDK_LIBRARIES})
message(STATUS SPDK_INCLUDE_DIR ${SPDK_INCLUDE_DIR})

set(SPDK_POS "/home/frw/spdk")

set(SPDK_LIB "-L/usr/local/lib -L${SPDK_POS}/build/lib \ 
 -lspdk_nvme -lspdk_sock -lspdk_sock_posix -lspdk_trace \ 
 -lspdk_rpc -lspdk_jsonrpc -lspdk_json -lspdk_vfio_user -lspdk_vmd -lspdk_env_dpdk \ 
 -lspdk_util -lspdk_log \
 -lssl
")

set(DPDK_LIB "-L${SPDK_POS}/dpdk/build/lib \
    -lrte_eal -lrte_eal -lrte_mempool -lrte_ring -lrte_mbuf \ 
    -lrte_bus_pci -lrte_pci -lrte_mempool_ring -lrte_telemetry -lrte_kvargs \ 
    -lrte_power -lrte_ethdev -lrte_net -lrte_vhost -lrte_net -lrte_dmadev -lrte_cryptodev -lrte_hash -lrte_rcu \ "
)

separate_arguments(SPDK_LIB UNIX_COMMAND "${SPDK_LIB}")
separate_arguments(DPDK_LIB UNIX_COMMAND "${DPDK_LIB}")

include_directories(third_party)
include_directories(include)
add_library(spdk_wrapper src/spdk_wrapper.cpp)
target_link_libraries(spdk_wrapper PUBLIC Folly::folly)
target_link_options(spdk_wrapper INTERFACE -Wl,--whole-archive -Wl,--no-as-needed)
target_link_options(spdk_wrapper INTERFACE ${SPDK_LIB} ${DPDK_LIB})
target_link_options(spdk_wrapper INTERFACE -Wl,--no-whole-archive)
target_link_options(spdk_wrapper INTERFACE -lnuma -ldl  -L/home/frw/spdk/isa-l/.libs -lisal -pthread -lrt -luuid -lcrypto -lm -laio -lfuse3)

add_executable(client src/client.cpp)
target_link_libraries(client mapper)

add_library(mapper src/mapper.cpp)
target_link_libraries(mapper 
PUBLIC 
Folly::folly 
cachelib tbb 
spdk_wrapper 
)

