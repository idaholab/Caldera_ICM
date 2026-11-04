if (NOT TARGET test_charge_start_time)
	add_executable(test_charge_start_time test_charge_start_time.cpp )

	target_link_libraries(test_charge_start_time Globals Charging_models Load_inputs factory Base)
	target_compile_features(test_charge_start_time PUBLIC cxx_std_17)
	target_include_directories(test_charge_start_time PUBLIC ${PROJECT_SOURCE_DIR}/source/globals)
	target_include_directories(test_charge_start_time PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
	target_include_directories(test_charge_start_time PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
	target_include_directories(test_charge_start_time PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
	target_include_directories(test_charge_start_time PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

	message("CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

	add_test(NAME "test_charge_start_time" COMMAND "test_charge_start_time" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
endif()
