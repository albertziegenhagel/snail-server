Test Apps
=========

This folder contains stand-alone applications that are used to generate testing data.

The CMake project in this folder builds all applications as external projects in both debug as well as release mode. It then executes the resulting executables in record mode (using `vsdiagnostics.exe` on Windows and `perf` on Linux) and copies both, the build executables (with `*.pdb` files if applicable) to the source current source tree.
