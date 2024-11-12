cmake_minimum_required(VERSION 3.5)
 
project(ICM)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED INSTALL_DIR)
	message(STATUS "setting local install dir")
	set(INSTALL_DIR ${PROJECT_SOURCE_DIR}/libs)
endif()

message(STATUS "Install dir is ${INSTALL_DIR}")

find_package(pybind11 CONFIG)

find_package(OpenMP REQUIRED)

# The link and include directories for OpenMP
# (assumes they're located where homebrew would install them)
if(APPLE)
    message(CMAKE_HOST_SYSTEM_PROCESSOR="${CMAKE_HOST_SYSTEM_PROCESSOR}")
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
        # On my M2 MacBook Pro the homebrew dir for libomp was this:
        # include_directories("/opt/homebrew/opt/libomp/include")
        # link_directories("/opt/homebrew/opt/libomp/lib")
        
        include_directories("/opt/homebrew/opt/libomp/include")
        link_directories("/opt/homebrew/opt/libomp/lib")
    else()
        # But on Intel it was this:
        # include_directories("/usr/local/opt/libomp/include")
        # link_directories("/usr/local/opt/libomp/lib")
        
        include_directories("/usr/local/opt/libomp/include")
        link_directories("/usr/local/opt/libomp/lib")
    endif()
endif()

add_subdirectory(source)
add_subdirectory(unittests)
add_subdirectory(misc)