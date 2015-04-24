cmake_minimum_required(VERSION 3.1)
project(hardinfo2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/includes
	${CMAKE_SOURCE_DIR}/includes/${HARDINFO_ARCH}
	${CMAKE_BINARY_DIR}
	${GTK_INCLUDE_DIRS}
)
link_directories(
	${GTK_LIBRARY_DIRS}
)
set(SOURCE_FILES hardinfo.c binreloc.c report.c benchmark.c)
add_executable(hardinfo_mod ${SOURCE_FILES})