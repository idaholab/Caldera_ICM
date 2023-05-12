set(LOAD_INPUTS_FILES	"load_EV_EVSE_inventory.cpp"
						"load_EV_inventory.cpp"
						"load_EVSE_inventory.cpp"
						"load_helper.cpp")

add_library(Load_inputs STATIC ${LOAD_INPUTS_FILES})
target_compile_features(Load_inputs PUBLIC cxx_std_17)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)



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