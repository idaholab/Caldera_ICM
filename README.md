# Caldera ICM

Infrastructure Charging Module (ICM) 

## Overview
Caldera ICM is a library of high-fidelity charging models for a wide variety of vehicles and supply equipments, validated by test data under a range of operating conditions. Caldera currently contains vehicle and supply equipment models for `DirectXFC`, `eMosaic` and `EVs_at_Risk` projects.

## Prerequisites

Caldera ICM depends on `C++ compiler`, `git`, `cmake`, `python` and `pybind11` to compile.

## Installation

Get a copy of Caldera ICM.
```
git clone https://hpcgitlab.hpc.inl.gov/caldera_charge/caldera_charge_icm.git
```
If you are interested in the most up to date version of Caldera ICM, the development version is available in the `develop` branch.    
```
git switch develop
```
To install Caldera ICM using cmake
```
cd caldera_charge_ICM
mkdir build
cmake -DPROJECT=<PROJECT_NAME> -DICM=<ON/OFF> ../
make
make install
```
If cmake cannot find C++ compiler, python or pybind11. They need to be pointed to its installed paths to find them. Refer to [pybind11](https://pybind11.readthedocs.io/en/stable/compiling.html#building-with-cmake) and [cmake](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) documentations. 


Caldera ICM typically installs three different compiled python libraries in the `libs/` folder `Caldera_ICM`, `Caldera_ICM_Aux` and, `Caldera_global`. 

`-DPROJECT` flag is used to specify the project specific EV-EVSE models to be used. Options are `DirectXFC`, `eMosaic` and `EVs_at_Risk`. `-DICM` flag turns ON/OFF compiling `Caldera_ICM` lib.

## Usage
Caldera ICM is recommended to be used with `Caldera Grid`. `Caldera Grid` also provides co-simulation support with `OpenDSS` and ability to load in inputs and control charging using setpoints in python.

## Authors
1. Don Scoffield, Senior Research Engineer, Idaho National Laboratory
2. Manoj Sundarrajan, Research Software Developer, Idaho National Laboratory

## License
For open source projects, say how it is licensed.
