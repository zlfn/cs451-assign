# CSED451: Computer Graphics
<img width="1226" height="825" alt="image" src="https://github.com/user-attachments/assets/82df29e0-9d50-4aba-8462-22f8e97971f5" />

Our Term Project Result (Realistic Ocean Rendering)

[project presentation](https://postechackr-my.sharepoint.com/:p:/g/personal/hyunseong_postech_ac_kr/IQAPa9F0AbntSKJ9V9-peJ1hAaONp5qY7rz6VVSiRFpNv4M?e=Axj0OJ)

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
