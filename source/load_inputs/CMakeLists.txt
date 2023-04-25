cmake_minimum_required(VERSION 3.5)
 
project(test)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED INSTALL_DIR)
	message(STATUS "setting local install dir")
	set(INSTALL_DIR ${PROJECT_SOURCE_DIR})
endif()

message(STATUS "Install dir is ${INSTALL_DIR}")

add_executable(loader "main.cpp" "load_EV_EVSE_inventory.cpp" "load_EV_inventory.cpp" "load_EVSE_inventory.cpp" 
"load_helper.cpp" "../charging_models/EV_characteristics.cpp" "../charging_models/EVSE_characteristics.cpp" 
"../charging_models/EV_EVSE_inventory.cpp" "load_EV_EVSE_inventory.h" "load_EV_EVSE_inventory.cpp")

target_compile_features(loader PUBLIC cxx_std_17)
target_include_directories(loader PUBLIC ${PROJECT_SOURCE_DIR}/../charging_models)

install(TARGETS loader
	DESTINATION ${INSTALL_DIR})
