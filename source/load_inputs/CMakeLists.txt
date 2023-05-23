set(LOAD_INPUTS_FILES	"load_EV_EVSE_inventory.cpp"
						"load_EV_inventory.cpp"
						"load_EVSE_inventory.cpp"
						"load_helper.cpp")

add_library(Load_inputs STATIC ${LOAD_INPUTS_FILES})
target_compile_features(Load_inputs PUBLIC cxx_std_17)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/globals)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(Load_inputs PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)