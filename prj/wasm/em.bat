::git clone https://github.com/emscripten-core/emsdk.git
::cd emsdk
::emsdk.bat install latest
::emsdk.bat activate latest
set currentPath=%CD% 
cd ..\..\..\..\emsdk
call emsdk_env.bat
cd %currentPath%
::D:\code\ThirdSources\emsdk\emsdk_env.bat
pause
cd ..\..\html
mkdir build
cd build
copy ..\..\prj\wasm\ninja.exe ninja.exe
::emcmake cmake -GNinja ..\..\..\ -DPRI=SUPPORT
emcmake cmake -GNinja ..\..\
pause
ninja
pause