set(CHARGING_MODEL_FILES	"EV_characteristics.cpp"
							"EVSE_characteristics.cpp"
							"EV_EVSE_inventory.cpp")

add_library(Charging_models STATIC ${CHARGING_MODEL_FILES})
target_compile_features(Charging_models PUBLIC cxx_std_17)
target_link_libraries(Charging_models PRIVATE Globals)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/globals)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

pybind11_add_module(Caldera_models MODULE charging_models_python_bind.cpp)
target_link_libraries(Caldera_models PRIVATE Charging_models)
install(TARGETS Caldera_models
		DESTINATION ${INSTALL_DIR})