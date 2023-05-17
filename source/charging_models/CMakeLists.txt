set(CHARGING_MODEL_FILES	"datatypes_global.cpp"
							"EV_characteristics.cpp"
							"EVSE_characteristics.cpp"
							"EV_EVSE_inventory.cpp")

add_library(Charging_models STATIC ${CHARGING_MODEL_FILES})
target_compile_features(Charging_models PUBLIC cxx_std_17)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(Charging_models PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)