from setuptools import Extension, setup
import pybind11
import platform
import subprocess
import sys

#---------------------------------------------------------------------------

path_to_anaconda_environment = sys.base_prefix

if platform.system() == "Windows":
    path_to_libraries = path_to_anaconda_environment + r"\Library"

    inc_dirs = [pybind11.get_include(), path_to_libraries+r"\include"]
    lib_dirs = [path_to_libraries + r"\lib"]
    # libs = ["libprotobuf"]
    libs = []
    ex_comp_args = ["/MT"]
    
    #protoc_cmd = path_to_libraries + r"\bin\protoc.exe"
    
else: #Linux
    inc_dirs = [pybind11.get_include(), path_to_anaconda_environment + r"/include"]
    lib_dirs = [path_to_anaconda_environment + r"/lib"]
    #libs = ["protobuf"]
    libs = []
    ex_comp_args = ["-Wall", "-std=c++11", "-pthread"]
    
    #protoc_cmd = "protoc"

#---------------------------------------------------------------------------

#protobuf_files = ["protobuf_datatypes.proto"]
#protobuf_files = []
#for f in protobuf_files: subprocess.run([protoc_cmd, f, "--cpp_out=."])

#---------------------------------------------------------------------------

exts=[
    Extension(
        name="Caldera_ICM_Aux",
        sources=["datatypes_module.cpp", "ac_to_dc_converter.cpp", "helper.cpp", "battery_calculate_limits.cpp", "battery_integrate_X_in_time.cpp",
                 "battery.cpp", "datatypes_global_SE_EV_definitions.cpp", "datatypes_global.cpp", "vehicle_charge_model.cpp", "supply_equipment_load.cpp",
                 "supply_equipment_control.cpp", "supply_equipment.cpp", "charge_profile_library.cpp",
                 "SE_EV_factory.cpp", "SE_EV_factory_charge_profile.cpp", "charge_profile_downsample_fragments.cpp", "Aux_interface.cpp", "Aux_python_bind.cpp"],
        include_dirs=inc_dirs,
        library_dirs=lib_dirs,
        libraries=libs,
        extra_compile_args=ex_comp_args,
        language="c++"
    )
]

#---------------------------------------------------------------------------

setup(
    name="Caldera_ICM_Aux_Obj",
    ext_modules = exts,
    version="0.1.0"
)

