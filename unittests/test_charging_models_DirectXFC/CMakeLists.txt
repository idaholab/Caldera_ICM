
add_executable(test_charging_models_DirectXFC main.cpp )
target_link_libraries(test_charging_models_DirectXFC Charging_models Load_inputs factory Base)
target_compile_features(test_charging_models_DirectXFC PUBLIC cxx_std_17)
target_include_directories(test_charging_models_DirectXFC PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(test_charging_models_DirectXFC PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(test_charging_models_DirectXFC PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(test_charging_models_DirectXFC PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

message("CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME "test_charging_models_DirectXFC" COMMAND "test_charging_models_DirectXFC" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
add_test(NAME "test_charging_models_DirectXFC_py" COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/validate.py WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )