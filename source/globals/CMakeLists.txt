set(GLOBAL_FILES	"datatypes_global.cpp"
					"helper.cpp")

add_library(Globals STATIC ${GLOBAL_FILES})
target_compile_features(Globals PUBLIC cxx_std_17)
target_include_directories(Globals PUBLIC ${PROJECT_SOURCE_DIR}/source/globals)
target_include_directories(Globals PUBLIC ${PROJECT_SOURCE_DIR}/source/base)
target_include_directories(Globals PUBLIC ${PROJECT_SOURCE_DIR}/source/charging_models)
target_include_directories(Globals PUBLIC ${PROJECT_SOURCE_DIR}/source/factory)
target_include_directories(Globals PUBLIC ${PROJECT_SOURCE_DIR}/source/load_inputs)

pybind11_add_module(Caldera_globals MODULE globals_python_bind.cpp)
target_link_libraries(Caldera_globals PRIVATE Globals)
install(TARGETS Caldera_globals
		DESTINATION ${INSTALL_DIR})