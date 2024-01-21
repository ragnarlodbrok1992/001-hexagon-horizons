@echo off
SET compiler_dir="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.31.31103\bin\Hostx64\x64\cl.exe"

echo Building...
echo Creating build directory...

if not exist "build" mkdir build

pushd build

%compiler_dir% /EHsc /Zi^
  /Fe:"main.exe"^
  ../src/main.cpp^
  /I"../lib/DirectX-Headers-main/include/directx/"^
  /link User32.lib Shell32.lib dxguid.lib d3d12.lib d3dcompiler.lib dxgi.lib

echo "Done building!"

popd build

