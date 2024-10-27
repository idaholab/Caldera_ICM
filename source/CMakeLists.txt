if (NOT TARGET Base)
	add_subdirectory(base)
endif()

if (NOT TARGET Charging_models)
	add_subdirectory(charging_models)
endif()

if (NOT TARGET factory)
	add_subdirectory(factory)
endif()

if (NOT TARGET Load_inputs)
	add_subdirectory(load_inputs)
endif()

if (NOT TARGET Globals)
	add_subdirectory(globals)
endif()

if (NOT TARGET Base)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_executable(tests main.cpp )
		target_link_libraries(tests Charging_models Load_inputs factory Base)
		target_compile_features(tests PUBLIC cxx_std_17)
		target_include_directories(tests PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
		target_include_directories(tests PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
		target_include_directories(tests PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
		target_include_directories(tests PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

		install(TARGETS tests
			DESTINATION ${PROJECT_SOURCE_DIR}/source)
	endif()
endif()