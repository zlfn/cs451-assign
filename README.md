# CSED451: Computer Graphics
## Development Environment
```
Windows
- MSVC (Microsoft Visual C++ with Visual Studio 2022)
- PowerShell
- CMake (Optional)

macOS
- Clang
- CMake

- FreeGLUT (or GLUT)
- GLEW
```

## How to Build
### Windows
#### Method1: Using Visual Studio 2022 MSVC
```
cd 0_setup/msvc
& .\build.ps1
```
* Note: You have to install Visual Studio 2022 in C:\Program Files

#### Method2: Using CMake
```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
* Note: You have to install cmake

### macOS
```
mkdir build
cd build
cmake ..
make
```
* Note1: You may need to install GLEW using Homebrew
* Note2: macOS legacy GLUT explicitly supports **Immediate Mode** via a macOS legacy OpenGL context.
