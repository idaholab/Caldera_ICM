
add_executable(run_models run_models.cpp )
target_link_libraries(run_models Charging_models Load_inputs factory Base)
target_compile_features(run_models PUBLIC cxx_std_17)
target_include_directories(run_models PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(run_models PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(run_models PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(run_models PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

install(TARGETS run_models
		DESTINATION ${PROJECT_SOURCE_DIR})

