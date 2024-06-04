cmake_minimum_required(VERSION 3.5)
 
project(ICM)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED INSTALL_DIR)
	message(STATUS "setting local install dir")
	set(INSTALL_DIR ${PROJECT_SOURCE_DIR}/libs)
endif()

message(STATUS "Install dir is ${INSTALL_DIR}")

find_package(pybind11 CONFIG)

add_subdirectory(source)
add_subdirectory(unittests)
add_subdirectory(misc)