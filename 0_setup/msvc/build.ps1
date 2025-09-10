$currentDir = Get-Location
if (Test-Path build) {
    Remove-Item -Recurse -Force build
}
New-Item -ItemType Directory -Path build

Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files\Microsoft Visual Studio\2022\Community" -DevCmdArguments "-arch=amd64 -host_arch=amd64"

Set-Location $currentDir

cl testbed.cpp /EHsc /std:c++17 /D "NDEBUG" /I ..\..\win-x64-msvc\include /Fo"build\\" /Fe"build\testbed.exe" /Fd"build\vc.pdb" /link /LIBPATH:..\..\win-x64-msvc\lib freeglut.lib glew32.lib opengl32.lib
$env:Path = $env:Path + ";$currentDir\..\..\win-x64-msvc\bin"
Write-Host "Testbed built successfully. Running..."
.\build\testbed.exe
Write-Host "Testbed exited."
