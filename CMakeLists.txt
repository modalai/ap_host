cmake_minimum_required(VERSION 3.16)

project(ap_host)

set(triple aarch64-linux-gnu)

set(CMAKE_C_COMPILER $ENV{AARCH64_SDK_DIR}/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER $ENV{AARCH64_SDK_DIR}/bin/aarch64-linux-gnu-g++)
set(CMAKE_LD $ENV{AARCH64_SDK_DIR}/bin/aarch64-linux-gnu-ld)
set(CMAKE_LINKER ${CMAKE_LD})
set(CMAKE_LIBRARY_ARCHITECTURE ${triple})

add_library(slpi_link SHARED
	libslpi-link-api/src/slpi_link_stub.c
	)

add_executable(ap_host src/main.cpp)

target_include_directories(ap_host PUBLIC
    libslpi-link-api/inc
)

target_link_libraries( ap_host
                        libstdc++.so
                        libpthread.so
                        ${CMAKE_SOURCE_DIR}/build/libslpi_link.so)