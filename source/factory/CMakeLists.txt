set(FACTORY_FILES	"factory_charging_transitions.cpp"
					"factory_P2_vs_battery_efficiency.cpp"
					"factory_puVrms_vs_P2.cpp"
					"factory_SOC_vs_P2.cpp"
					"factory_EV_charge_model.cpp"
					"factory_supply_equipment_model.cpp"
					"factory_ac_to_dc_converter.cpp")

add_library(factory STATIC ${FACTORY_FILES})
target_compile_features(factory PUBLIC cxx_std_17)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(factory PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)



#[===[
add_library(Caldera_global_lib STATIC datatypes_global.cpp ${PROJECT}/datatypes_global_SE_EV_definitions.cpp)
target_include_directories(Caldera_global_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Caldera_global_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT})

pybind11_add_module(Caldera_global MODULE ${PROJECT}/python_bind.cpp)
target_link_libraries(Caldera_global PRIVATE Caldera_global_lib)

message(Caldera_global " current source dir = ${CMAKE_CURRENT_SOURCE_DIR}")

#target_compile_options(Caldera_global PUBLIC -Wa,-mbig-obj)

install(TARGETS Caldera_global
		DESTINATION ${INSTALL_DIR})

]===]