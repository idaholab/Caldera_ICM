add_subdirectory(base)
add_subdirectory(charging_models)
add_subdirectory(factory)
add_subdirectory(load_inputs)


#[===[
cmake_minimum_required(VERSION 3.5)
 
project(CalderaICM)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED INSTALL_DIR)
	message(STATUS "setting local install dir")
	set(INSTALL_DIR ${PROJECT_SOURCE_DIR})
endif()

message(STATUS "Install dir is ${INSTALL_DIR}")

set(LOAD_SOURCE_FILES		"load_inputs/load_helper.cpp"
							"load_inputs/load_EV_EVSE_inventory.cpp"
							"load_inputs/load_EV_inventory.cpp"
							"load_inputs/load_EVSE_inventory.cpp")

set(FACTORY_SOURCE_FILES	"factory/EV_charge_model_factory.cpp"
							"factory/charging_transitions_factory.cpp"
							"factory/P2_vs_battery_efficiency_factory.cpp"
							"factory/puVrms_vs_P2_factory.cpp"
							"factory/SOC_vs_P2_factory.cpp"
)

set(MODELS_SOURCE_FILES		"charging_models/EV_characteristics.cpp"
							"charging_models/EV_EVSE_inventory.cpp"
							"charging_models/EVSE_characteristics.cpp"
							"charging_models/datatypes_global.cpp")

set(BASE_SOURCE_FILES		"base/battery_calculate_limits.cpp"
							"base/battery_integrate_X_in_time.cpp"
							"base/vehicle_charge_model.cpp"
							"base/battery.cpp"
							"base/helper.cpp")

add_executable(models_test ${MODELS_SOURCE_FILES} ${LOAD_SOURCE_FILES} ${FACTORY_SOURCE_FILES} ${BASE_SOURCE_FILES} "main.cpp")

target_compile_features(models_test PUBLIC cxx_std_17)
target_include_directories(models_test PUBLIC ${PROJECT_SOURCE_DIR}/charging_models)
target_include_directories(models_test PUBLIC ${PROJECT_SOURCE_DIR}/factory)
target_include_directories(models_test PUBLIC ${PROJECT_SOURCE_DIR}/load_inputs)
target_include_directories(models_test PUBLIC ${PROJECT_SOURCE_DIR}/base)

install(TARGETS models_test
	DESTINATION ${INSTALL_DIR})


]===]