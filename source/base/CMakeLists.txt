set(BASE_FILES		"datatypes_module.cpp"
					"ac_to_dc_converter.cpp"
					"battery_calculate_limits.cpp"
					"battery_integrate_X_in_time.cpp"
					"battery.cpp"
					"vehicle_charge_model.cpp"
					"supply_equipment_load.cpp"
					"supply_equipment_control.cpp"
					"supply_equipment.cpp"
					"supply_equipment_group.cpp"
					"charge_profile_library.cpp"
					"SE_EV_factory_charge_profile.cpp"
					"charge_profile_downsample_fragments.cpp"
					"ICM_interface.cpp")

add_library(Base STATIC ${BASE_FILES})
target_link_libraries(Base PRIVATE Charging_models)
target_link_libraries(Base PRIVATE factory)
target_link_libraries(Base PRIVATE Load_inputs)
target_link_libraries(Base PUBLIC OpenMP::OpenMP_CXX)
target_include_directories(Base PUBLIC ${PROJECT_SOURCE_DIR}/source/globals)
target_include_directories(Base PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(Base PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(Base PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(Base PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

pybind11_add_module(Caldera_ICM MODULE ICM_python_bind.cpp)
target_link_libraries(Caldera_ICM PRIVATE Base)
install(TARGETS Caldera_ICM
		DESTINATION ${INSTALL_DIR})

set(BASE_AUX_FILES		"datatypes_module.cpp"
						"ac_to_dc_converter.cpp"
						"battery_calculate_limits.cpp"
						"battery_integrate_X_in_time.cpp"
						"battery.cpp"
						"vehicle_charge_model.cpp"
						"supply_equipment_load.cpp"
						"supply_equipment_control.cpp"
						"supply_equipment.cpp"
						"charge_profile_library.cpp"
						"SE_EV_factory_charge_profile.cpp"
						"charge_profile_downsample_fragments.cpp"
						"Aux_interface.cpp")

add_library(Base_aux STATIC ${BASE_AUX_FILES})
target_link_libraries(Base_aux PRIVATE Charging_models)
target_link_libraries(Base_aux PRIVATE factory)
target_link_libraries(Base_aux PRIVATE Load_inputs)
target_link_libraries(Base_aux PUBLIC OpenMP::OpenMP_CXX)
target_include_directories(Base_aux PUBLIC ${PROJECT_SOURCE_DIR}/source/globals)
target_include_directories(Base_aux PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(Base_aux PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(Base_aux PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(Base_aux PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

pybind11_add_module(Caldera_ICM_Aux MODULE Aux_python_bind.cpp)
target_link_libraries(Caldera_ICM_Aux PRIVATE Base_aux)
install(TARGETS Caldera_ICM_Aux
		DESTINATION ${INSTALL_DIR})