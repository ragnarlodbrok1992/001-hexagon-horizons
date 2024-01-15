@echo off
SET compiler_dir="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.31.31103\bin\Hostx64\x64\cl.exe"

echo Building...
echo Creating build directory...

if not exist "build" mkdir build

pushd build

%compiler_dir% /EHsc /Zi^
  /Fe:"main.exe"^
  ../src/main.cpp^
  /link User32.lib Shell32.lib

echo "Done building!"

popd build

