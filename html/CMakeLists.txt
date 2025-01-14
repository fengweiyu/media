cmake_minimum_required(VERSION 3.10) 


project (MediaConvert)
 
#set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/ThirdLib/media) #CMAKE_INSTALL_PREFIX 外面设置，内部设置无效

# 设置C++标准为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message("PROJECT_SOURCE_DIR is: ${PROJECT_SOURCE_DIR} ${CMAKE_INSTALL_PREFIX}")

#-s WASM=1 -s USE_SDL=2 -s TOTAL_MEMORY=268435456 通过设置 WASM=1，编译器会生成适用于 WebAssembly 的输出 定义 WebAssembly 模块的总内存大小256 MB 用于_mallo等内存拷贝
#-s EXPORTED_FUNCTIONS="['_fnInitDecoder', '_fnUninitDecoder', '_main', '_malloc', '_free']" js中无用定义和使用数组以及指针，必须要_malloc
#-s EXTRA_EXPORTED_RUNTIME_METHODS="['addFunction']" 这些都是链接选项，不是编译选项
#-s RESERVED_FUNCTION_POINTERS=15 指定要保留的函数指针（Function Pointers）数量 Emscripten 将为最多 15 个函数指针分配存储空间
#-s FORCE_FILESYSTEM=1 参数强制 Emscripten 在 WebAssembly 模块中启用文件系统支持
# 为目标添加编译选项  
#target_compile_options(MyExecutable PRIVATE -Wall -Wextra)  # 例如，添加警告选项  
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s TOTAL_MEMORY=268435456 -s RESERVED_FUNCTION_POINTERS=15 -s FORCE_FILESYSTEM=1 -s EXPORTED_FUNCTIONS=[_malloc,_free]")  
#target_link_options(MyExecutable PRIVATE ) 
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1 -s TOTAL_MEMORY=268435456 -s RESERVED_FUNCTION_POINTERS=15 -s FORCE_FILESYSTEM=1 -s EXPORTED_FUNCTIONS=[_malloc,_free]") 


# 收集源文件  
file(GLOB demo_list ${PROJECT_SOURCE_DIR}/MediaConvert.cpp)
add_executable(MediaConvert ${demo_list})
 #set(CMAKE_EXECUTABLE_SUFFIX ".html") # 编译生成.html
target_include_directories(MediaConvert PUBLIC ${PROJECT_SOURCE_DIR}/../src/include)
target_link_libraries(MediaConvert PUBLIC ${PROJECT_SOURCE_DIR}/libmedia.a) 
install(TARGETS MediaConvert DESTINATION .) #安装目录 目录MediaConvert换成目录. 也可以，这样就在当前目录生成不会生成在MediaConvert目录下

#SET_TARGET_PROPERTIES(base64_static PROPERTIES OUTPUT_NAME base64)
#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)

#TARGET_LINK_LIBRARIES(webrtc pthread -lm)

#TARGET_LINK_LIBRARIES(crnode log4cpp thrift)










