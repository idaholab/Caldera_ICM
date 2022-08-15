set(GLOBALS_DIR ${PROJECT_SOURCE_DIR}/source/charging_models)

option(ICM "compile caldera_ICM library" OFF)

if(ICM)
	add_library(Caldera_ICM_lib datatypes_module.cpp ac_to_dc_converter.cpp helper.cpp battery_calculate_limits.cpp
				battery_integrate_X_in_time.cpp battery.cpp vehicle_charge_model.cpp supply_equipment_load.cpp 
				supply_equipment_control.cpp supply_equipment.cpp supply_equipment_group.cpp charge_profile_library.cpp 
				SE_EV_factory_charge_profile.cpp charge_profile_downsample_fragments.cpp ICM_interface.cpp  
				${GLOBALS_DIR}/datatypes_global.cpp ${GLOBALS_DIR}/${PROJECT}/SE_EV_factory.cpp 
				${GLOBALS_DIR}/${PROJECT}/datatypes_global_SE_EV_definitions.cpp)

	target_include_directories(Caldera_ICM_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
	target_include_directories(Caldera_ICM_lib PUBLIC ${GLOBALS_DIR})
	target_include_directories(Caldera_ICM_lib PUBLIC ${GLOBALS_DIR}/${PROJECT})
	
	pybind11_add_module(Caldera_ICM MODULE ICM_python_bind.cpp)
	target_link_libraries(Caldera_ICM PRIVATE Caldera_ICM_lib)

	message(Caldera_ICM " current source dir = ${CMAKE_CURRENT_SOURCE_DIR}")

	install(TARGETS Caldera_ICM
		DESTINATION ${INSTALL_DIR})
endif()

add_library(Caldera_ICM_Aux_lib datatypes_module.cpp ac_to_dc_converter.cpp helper.cpp battery_calculate_limits.cpp 
  			battery_integrate_X_in_time.cpp battery.cpp vehicle_charge_model.cpp supply_equipment_load.cpp 
  			supply_equipment_control.cpp supply_equipment.cpp charge_profile_library.cpp SE_EV_factory_charge_profile.cpp 
  			charge_profile_downsample_fragments.cpp Aux_interface.cpp 
  			${GLOBALS_DIR}/${PROJECT}/datatypes_global_SE_EV_definitions.cpp ${GLOBALS_DIR}/datatypes_global.cpp 
  			${GLOBALS_DIR}/${PROJECT}/SE_EV_factory.cpp)

target_include_directories(Caldera_ICM_Aux_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Caldera_ICM_Aux_lib PUBLIC ${GLOBALS_DIR})
target_include_directories(Caldera_ICM_Aux_lib PUBLIC ${GLOBALS_DIR}/${PROJECT})

pybind11_add_module(Caldera_ICM_Aux MODULE Aux_python_bind.cpp)
target_link_libraries(Caldera_ICM_Aux PRIVATE Caldera_ICM_Aux_lib)

message(Caldera_ICM_Aux " current source dir = ${CMAKE_CURRENT_SOURCE_DIR}")

install(TARGETS Caldera_ICM_Aux
		DESTINATION ${INSTALL_DIR})