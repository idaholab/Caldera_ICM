pybind11_add_module(Caldera_global datatypes_global.cpp ${PROJECT}/datatypes_global_SE_EV_definitions.cpp ${PROJECT}/python_bind.cpp)

message(Caldera_global " current source dir = ${CMAKE_CURRENT_SOURCE_DIR}")

#target_compile_options(Caldera_global PUBLIC -Wa,-mbig-obj)

target_include_directories(Caldera_global PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Caldera_global PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT})

install(TARGETS Caldera_global
		DESTINATION ${INSTALL_DIR})