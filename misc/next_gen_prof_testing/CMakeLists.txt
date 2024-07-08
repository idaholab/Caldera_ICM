add_executable(ICM_Full ICM_Full.cpp)
target_link_libraries(ICM_Full Charging_models Load_inputs factory Base)
target_compile_features(ICM_Full PUBLIC cxx_std_17)
target_include_directories(ICM_Full PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(ICM_Full PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(ICM_Full PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(ICM_Full PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

install(TARGETS ICM_Full
		DESTINATION ${PROJECT_SOURCE_DIR}/misc/next_gen_prof_testing)

add_executable(ICM_Aux ICM_Aux.cpp )
target_link_libraries(ICM_Aux Charging_models Load_inputs factory Base_aux)
target_compile_features(ICM_Aux PUBLIC cxx_std_17)
target_include_directories(ICM_Aux PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(ICM_Aux PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(ICM_Aux PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(ICM_Aux PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

install(TARGETS ICM_Aux
		DESTINATION ${PROJECT_SOURCE_DIR}/misc/next_gen_prof_testing)

add_executable(ICM_NextGen ICM_NextGen.cpp )
target_link_libraries(ICM_NextGen Charging_models Load_inputs factory Base_aux)
target_compile_features(ICM_NextGen PUBLIC cxx_std_17)
target_include_directories(ICM_NextGen PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(ICM_NextGen PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(ICM_NextGen PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(ICM_NextGen PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

install(TARGETS ICM_NextGen
		DESTINATION ${PROJECT_SOURCE_DIR}/misc/next_gen_prof_testing)