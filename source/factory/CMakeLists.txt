set(FACTORY_FILES	"factory_charging_transitions.cpp"
					"factory_P2_vs_battery_efficiency.cpp"
					"factory_puVrms_vs_P2.cpp"
					"factory_SOC_vs_P2.cpp"
					"factory_EV_charge_model.cpp"
					"factory_supply_equipment_model.cpp"
					"factory_ac_to_dc_converter.cpp")

add_library(factory STATIC ${FACTORY_FILES})
target_compile_features(factory PUBLIC cxx_std_17)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/globals)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)
