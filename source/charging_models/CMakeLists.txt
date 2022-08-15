
add_library(Caldera_global_lib datatypes_global.cpp ${PROJECT}/datatypes_global_SE_EV_definitions.cpp)
target_include_directories(Caldera_global_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Caldera_global_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT})

pybind11_add_module(Caldera_global MODULE ${PROJECT}/python_bind.cpp)
target_link_libraries(Caldera_global PRIVATE Caldera_global_lib)

message(Caldera_global " current source dir = ${CMAKE_CURRENT_SOURCE_DIR}")

#target_compile_options(Caldera_global PUBLIC -Wa,-mbig-obj)

install(TARGETS Caldera_global
		DESTINATION ${INSTALL_DIR})