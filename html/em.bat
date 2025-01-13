::git clone https://github.com/emscripten-core/emsdk.git
::cd emsdk
::emsdk.bat install latest
::emsdk.bat activate latest
set currentPath=%CD% 
cd ..\..\..\..\emsdk
call emsdk_env.bat
cd %currentPath%
pause
emcmake cmake -GNinja ..
pause
ninja
pause