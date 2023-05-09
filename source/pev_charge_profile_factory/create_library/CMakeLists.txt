cmake_minimum_required(VERSION 3.5)
 
project(Charge_profile_library)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED INSTALL_DIR)
	message(STATUS "setting local install dir")
	set(INSTALL_DIR ${PROJECT_SOURCE_DIR}/../generate_charge_profiles)
endif()

message(STATUS "Install dir is ${INSTALL_DIR}")
find_package(pybind11 CONFIG)
add_executable(charge_profile_factory pev_charge_profile_factory.cpp main.cpp)
#pybind11_add_module(charge_profile_factory MODULE pev_charge_profile_factory.cpp python_bind.cpp main.cpp)

install(TARGETS charge_profile_factory
	DESTINATION ${INSTALL_DIR})
