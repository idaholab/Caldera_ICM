if (NOT TARGET test_ToU_RANDOM_M2_control_strategy)
	add_executable(test_ToU_RANDOM_M2_control_strategy test_ToU_RANDOM_M2_control_strategy.cpp )

	target_link_libraries(test_ToU_RANDOM_M2_control_strategy Globals Charging_models Load_inputs factory Base)
	target_compile_features(test_ToU_RANDOM_M2_control_strategy PUBLIC cxx_std_17)
	target_include_directories(test_ToU_RANDOM_M2_control_strategy PUBLIC ${PROJECT_SOURCE_DIR}/source/globals)
	target_include_directories(test_ToU_RANDOM_M2_control_strategy PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
	target_include_directories(test_ToU_RANDOM_M2_control_strategy PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
	target_include_directories(test_ToU_RANDOM_M2_control_strategy PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
	target_include_directories(test_ToU_RANDOM_M2_control_strategy PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

	message("CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

	add_test(NAME "test_ToU_RANDOM_M2_control_strategy" COMMAND "test_ToU_RANDOM_M2_control_strategy" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
endif()
