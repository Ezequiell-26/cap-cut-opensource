# Building CCOS

## Windows

Install Visual Studio 2022 with the Desktop C++ workload, CMake and Qt 6.5+. Configure from a Developer PowerShell using the repository's `CMakePresets.json` when available, or pass the Qt CMake package path explicitly.

## Linux

Install a C++20 compiler, CMake, Qt 6 Widgets development packages and GoogleTest development packages. On Debian/Ubuntu-based systems the package names may be `qt6-base-dev` and `libgtest-dev`.

## macOS

Install Xcode Command Line Tools, CMake and Qt 6. The Qt CMake package path must be discoverable by CMake.

## Configure

```sh
cmake -S . -B build -DCCOS_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

The project intentionally does not bundle third-party SDK binaries in this repository.
