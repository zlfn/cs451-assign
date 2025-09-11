$currentDir = Get-Location
if (Test-Path build) {
    Remove-Item -Recurse -Force build
}
New-Item -ItemType Directory -Path build

Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files\Microsoft Visual Studio\2022\Community" -DevCmdArguments "-arch=amd64 -host_arch=amd64"

Set-Location $currentDir

# Build main executable
cl src\main.cpp /EHsc /std:c++20 /D "NDEBUG" /I ..\win-x64-msvc\include /I src /Fo"build\\" /Fe"build\1_2d_game.exe" /Fd"build\vc.pdb" /link /LIBPATH:..\win-x64-msvc\lib freeglut.lib glew32.lib opengl32.lib

$env:Path = $env:Path + ";$currentDir\..\win-x64-msvc\bin"

# Run main program
Write-Host "Running 1_2d_game..."
.\build\1_2d_game.exe
Write-Host "1_2d_game exited."
