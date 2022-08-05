set(GLOBALS_DIR ${PROJECT_SOURCE_DIR}/source/charging_models)

option(ICM "compile caldera_ICM library" OFF)

if(ICM)
	pybind11_add_module(Caldera_ICM datatypes_module.cpp ac_to_dc_converter.cpp helper.cpp battery_calculate_limits.cpp
				battery_integrate_X_in_time.cpp battery.cpp vehicle_charge_model.cpp supply_equipment_load.cpp 
				supply_equipment_control.cpp supply_equipment.cpp supply_equipment_group.cpp charge_profile_library.cpp 
				SE_EV_factory_charge_profile.cpp charge_profile_downsample_fragments.cpp ICM_interface.cpp ICM_python_bind.cpp 
				${GLOBALS_DIR}/datatypes_global.cpp ${GLOBALS_DIR}/${PROJECT}/SE_EV_factory.cpp 
				${GLOBALS_DIR}/${PROJECT}/datatypes_global_SE_EV_definitions.cpp)

	message(Caldera_ICM " current source dir = ${CMAKE_CURRENT_SOURCE_DIR}")

	target_include_directories(Caldera_ICM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
	target_include_directories(Caldera_ICM PUBLIC ${GLOBALS_DIR})
	target_include_directories(Caldera_ICM PUBLIC ${GLOBALS_DIR}/${PROJECT})
	install(TARGETS Caldera_ICM
		DESTINATION ${INSTALL_DIR})
endif()

pybind11_add_module(Caldera_ICM_Aux datatypes_module.cpp ac_to_dc_converter.cpp helper.cpp battery_calculate_limits.cpp 
  			battery_integrate_X_in_time.cpp battery.cpp vehicle_charge_model.cpp supply_equipment_load.cpp 
  			supply_equipment_control.cpp supply_equipment.cpp charge_profile_library.cpp SE_EV_factory_charge_profile.cpp 
  			charge_profile_downsample_fragments.cpp Aux_interface.cpp Aux_python_bind.cpp 
  			${GLOBALS_DIR}/${PROJECT}/datatypes_global_SE_EV_definitions.cpp ${GLOBALS_DIR}/datatypes_global.cpp 
  			${GLOBALS_DIR}/${PROJECT}/SE_EV_factory.cpp)

message(Caldera_ICM_Aux " current source dir = ${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(Caldera_ICM_Aux PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Caldera_ICM_Aux PUBLIC ${GLOBALS_DIR})
target_include_directories(Caldera_ICM_Aux PUBLIC ${GLOBALS_DIR}/${PROJECT})

install(TARGETS Caldera_ICM_Aux
		DESTINATION ${INSTALL_DIR})