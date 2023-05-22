cmake_minimum_required(VERSION 3.5)
 
project(ICM)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED PROJECT)
	message(WARNING "PROJECT cmake variable not set defaulting to emosiac project")
endif()

SET(PROJECT "eMosaic" CACHE STRING "Project to compile for")

if (NOT (${PROJECT} STREQUAL "eMosaic" OR ${PROJECT} STREQUAL "EVs_at_Risk" OR ${PROJECT} STREQUAL "DirectXFC"))
	message(FATAL_ERROR "PROJECT cmake variable must be equal to eMosaic or EVs_at_Risk or DirectXFC")
endif()

if (NOT DEFINED INSTALL_DIR)
	message(STATUS "setting local install dir")
	set(INSTALL_DIR ${PROJECT_SOURCE_DIR}/libs)
endif()

message(STATUS "Install dir is ${INSTALL_DIR}")

find_package(pybind11 CONFIG)

add_subdirectory(source)
add_subdirectory(unittests)
