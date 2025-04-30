cmake_minimum_required(VERSION 3.10) 


project (media)
 
#set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/ThirdLib/media) #CMAKE_INSTALL_PREFIX 外面设置，内部设置无效
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    #子目录也会设置CMAKE_INSTALL_PREFIX，但是子目录退出后不生效，所以这里设置全局的，
    #这样这里生成的Makefile可以执行install到下面这个目录，缺点是子目录相关的库文件头文件也会拷贝过来
    set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/build/wasm/x64)
    set(AAC_LIB_PATH ${PROJECT_SOURCE_DIR}/AudioCodec/third/lib/wasm/fdk-aac/lib/libfdk-aac.a)
    set(HOST_PATH_NAME wasm)
    set(CODEC_PATH ${PROJECT_SOURCE_DIR}/MediaCodec/lib/wasm/x86)
    set(CODEC_NAME x86.tar.gz)
    set(XBASIC_LIB_PATH ${PROJECT_SOURCE_DIR}/third/XBasic/lib/wasm/libXBasic.a)
    set(XPROTOCOL_LIB_PATH ${PROJECT_SOURCE_DIR}/third/XProtocol/lib/wasm/libXProtocol.a)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows") #WIN32
    #子目录也会设置CMAKE_INSTALL_PREFIX，但是子目录退出后不生效，所以这里设置全局的，
    #这样这里生成的Makefile可以执行install到下面这个目录，缺点是子目录相关的库文件头文件也会拷贝过来
    set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/build/win/x64)
    set(SYSTEM_PLAT_LIB_PATH Ws2_32.lib)
    set(AAC_LIB_PATH ${PROJECT_SOURCE_DIR}/AudioCodec/third/lib/win/fdk-aac/lib/libfdk-aac.lib)
    set(HOST_PATH_NAME win)
    set(CODEC_PATH ${PROJECT_SOURCE_DIR}/MediaCodec/lib/win/x64msvc)
    set(CODEC_NAME x64msvc.zip)
    set(XBASIC_LIB_PATH ${PROJECT_SOURCE_DIR}/third/XBasic/lib/win/XBasic_static.lib)
    set(XPROTOCOL_LIB_PATH ${PROJECT_SOURCE_DIR}/third/XProtocol/lib/win/XProtocol_static.lib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #子目录也会设置CMAKE_INSTALL_PREFIX，但是子目录退出后不生效，所以这里设置全局的，
    #这样这里生成的Makefile可以执行install到下面这个目录，缺点是子目录相关的库文件头文件也会拷贝过来
    set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/build/linux/x64)
    set(SYSTEM_PLAT_LIB_PATH pthread)
    set(AAC_LIB_PATH ${PROJECT_SOURCE_DIR}/AudioCodec/third/lib/linux/fdk-aac/lib/libfdk-aac.a)
    set(HOST_PATH_NAME linux)
    set(CODEC_PATH ${PROJECT_SOURCE_DIR}/MediaCodec/lib/linux/x86)
    set(CODEC_NAME x86.tar.gz)
    set(XBASIC_LIB_PATH ${PROJECT_SOURCE_DIR}/third/XBasic/lib/linux/libXBasic.a 
    ${PROJECT_SOURCE_DIR}/third/XBasic/lib/linux/libimgui.a 
    ${PROJECT_SOURCE_DIR}/third/XBasic/lib/linux/libssl.a 
    ${PROJECT_SOURCE_DIR}/third/XBasic/lib/linux/libcrypto.a  
    ${PROJECT_SOURCE_DIR}/third/XBasic/lib/linux/libfmt.a
    ${PROJECT_SOURCE_DIR}/third/XBasic/lib/linux/libiconv.a  
    ${PROJECT_SOURCE_DIR}/third/XBasic/lib/linux/libz.a
    ${PROJECT_SOURCE_DIR}/third/XBasic/lib/linux/libspdlog.a)
    set(XPROTOCOL_LIB_PATH ${PROJECT_SOURCE_DIR}/third/XProtocol/lib/linux/libXProtocol.a )
else()  
    message("CMAKE_SYSTEM_NAME err is: ${CMAKE_SYSTEM_NAME}")
endif() 


execute_process(
    COMMAND rm -rf ${CODEC_PATH}
    COMMAND tar -xvf ${PROJECT_SOURCE_DIR}/MediaCodec/lib/${HOST_PATH_NAME}/${CODEC_NAME}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/MediaCodec/lib/${HOST_PATH_NAME} #如果不设置那么会解压到cmake的执行目录
)
set(CODEC_FFMPEG_INC_PATH ${CODEC_PATH}/ffmpeg-7.0.1/include)
set(CODEC_FFMPEG_LIB_PATH ${CODEC_PATH}/ffmpeg-7.0.1/lib/libavcodec.a ${CODEC_PATH}/ffmpeg-7.0.1/lib/libavdevice.a ${CODEC_PATH}/ffmpeg-7.0.1/lib/libavfilter.a 
${CODEC_PATH}/ffmpeg-7.0.1/lib/libavformat.a ${CODEC_PATH}/ffmpeg-7.0.1/lib/libavutil.a ${CODEC_PATH}/ffmpeg-7.0.1/lib/libpostproc.a ${CODEC_PATH}/ffmpeg-7.0.1/lib/libswresample.a ${CODEC_PATH}/ffmpeg-7.0.1/lib/libswscale.a)
set(CODEC_FONTCONF_INC_PATH ${CODEC_PATH}/libfontconfig-5.1.0/include)
set(CODEC_FONTCONF_LIB_PATH ${CODEC_PATH}/libfontconfig-5.1.0/lib/libfontconfig.a)
set(CODEC_HARFBUZZ_INC_PATH ${CODEC_PATH}/harfbuzz-8.5.0/include)
set(CODEC_HARFBUZZ_LIB_PATH ${CODEC_PATH}/harfbuzz-8.5.0/lib/libharfbuzz.a)
set(CODEC_FREETYPE_INC_PATH ${CODEC_PATH}/freetype-2.13.2/include)
set(CODEC_FREETYPE_LIB_PATH ${CODEC_PATH}/freetype-2.13.2/lib/libfreetype.a)
set(CODEC_XML2_INC_PATH ${CODEC_PATH}/libxml2-2.9.12/include)
set(CODEC_XML2_LIB_PATH ${CODEC_PATH}/libxml2-2.9.12/lib/libxml2.a)
set(CODEC_X264_INC_PATH ${CODEC_PATH}/x264-stable/include)
set(CODEC_X264_LIB_PATH ${CODEC_PATH}/x264-stable/lib/libx264.a)
set(CODEC_X265_INC_PATH ${CODEC_PATH}/x265_2.7/include)
set(CODEC_X265_LIB_PATH ${CODEC_PATH}/x265_2.7/lib/libx265.a)
set(CODEC_AAC_INC_PATH ${CODEC_PATH}/fdk-aac-2.0.3/include)
set(CODEC_AAC_LIB_PATH ${CODEC_PATH}/fdk-aac-2.0.3/lib/libfdk-aac.a)
# 添加 lib 子目录  
add_subdirectory(AudioCodec) 
add_subdirectory(MediaCodec) 
 

# 设置C++标准为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message("PROJECT_SOURCE_DIR is: ${PROJECT_SOURCE_DIR} ${CMAKE_INSTALL_PREFIX}")

# 收集所有源文件  
file(GLOB src_list 
    src/MediaHandle.cpp
    src/TS/TsInterface.cpp
    src/TS/TsPack.cpp
    src/FMP4/FMP4HandleInterface.cpp
    src/FMP4/FMP4Handle.cpp
    src/FMP4/FMP4.cpp
    src/FLV/FlvHandleInterface.cpp
    src/FLV/FlvParseHandle.cpp
    src/FLV/FlvPackHandle.cpp
    src/FLV/FlvHandle.cpp
    src/RawVideoHandle.cpp
    src/RawAudioHandle.cpp
    )  # 可以根据你的目录结构调整 

#set(src_list
#    src/xxx.cpp
#)

# 定义库  
add_library(media ${src_list}) 

target_include_directories(media PUBLIC
    ${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/src/include
    ${PROJECT_SOURCE_DIR}/src/FLV/include
    ${PROJECT_SOURCE_DIR}/src/FLV
    ${PROJECT_SOURCE_DIR}/src/TS/include
    ${PROJECT_SOURCE_DIR}/src/TS
    ${PROJECT_SOURCE_DIR}/src/FMP4/include
    ${PROJECT_SOURCE_DIR}/src/FMP4
)





# 安装库文件  
install(TARGETS media  
        DESTINATION ThirdLib/media/lib)  # 库文件的安装目录  

# 安装头文件  
file(GLOB PUBLIC_HEADERS "${PROJECT_SOURCE_DIR}/src/include/*.h")
install(FILES ${PUBLIC_HEADERS}  
        DESTINATION ThirdLib/media/include)  # 头文件的安装目录

#-s WASM=1 -s USE_SDL=2 -s TOTAL_MEMORY=268435456 通过设置 WASM=1，编译器会生成适用于 WebAssembly 的输出 定义 WebAssembly 模块的总内存大小256 MB 用于_mallo等内存拷贝
#-s EXPORTED_FUNCTIONS="['_fnInitDecoder', '_fnUninitDecoder', '_main', '_malloc', '_free']" js中无用定义和使用数组以及指针，必须要_malloc
#-s EXTRA_EXPORTED_RUNTIME_METHODS="['addFunction']" 这些都是链接选项，不是编译选项
#-s RESERVED_FUNCTION_POINTERS=15 指定要保留的函数指针（Function Pointers）数量 Emscripten 将为最多 15 个函数指针分配存储空间
#-s FORCE_FILESYSTEM=1 参数强制 Emscripten 在 WebAssembly 模块中启用文件系统支持
# 为目标添加编译选项  
#target_compile_options(MyExecutable PRIVATE -Wall -Wextra)  # 例如，添加警告选项  
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s TOTAL_MEMORY=268435456 -s RESERVED_FUNCTION_POINTERS=15 -s FORCE_FILESYSTEM=1 -s EXPORTED_FUNCTIONS=[_malloc,_free]")  
#target_link_options(MyExecutable PRIVATE ) 
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")  
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1 -s TOTAL_MEMORY=314572800 -s RESERVED_FUNCTION_POINTERS=20 -s FORCE_FILESYSTEM=1 -s EXPORTED_FUNCTIONS=[_malloc,_free] -s EXPORTED_RUNTIME_METHODS=[FS,ccall,cwrap]") 
endif() 

# 收集源文件  
file(GLOB demo_list demo/MediaConvert.cpp demo/MediaConvertMain.cpp)
add_executable(MediaConvert ${demo_list})
 #set(CMAKE_EXECUTABLE_SUFFIX ".html") # 编译生成.html
set(MC_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src/include ${PROJECT_SOURCE_DIR}/demo ${PROJECT_SOURCE_DIR}/AudioCodec/src/include)
set(MC_LINK_LIBS media AudioCodec ${AAC_LIB_PATH} ${SYSTEM_PLAT_LIB_PATH})
if(DEFINED PRI)  
    add_definitions(-DSUPPORT_PRI)
    set(MC_INCLUDE_DIRS ${MC_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/third/XBasic/include ${PROJECT_SOURCE_DIR}/third/XBasic/include/XBasic
    ${PROJECT_SOURCE_DIR}/third/XProtocol/include ${PROJECT_SOURCE_DIR}/third/XProtocol/include/XProtocol/StreamParser)
    set(MC_LINK_LIBS ${MC_LINK_LIBS} ${XPROTOCOL_LIB_PATH} ${XBASIC_LIB_PATH}) 
else()  
endif()  
if(DEFINED CODEC)  
    add_definitions(-DSUPPORT_CODEC)
    set(MC_INCLUDE_DIRS ${MC_INCLUDE_DIRS} ${CODEC_FFMPEG_INC_PATH})
    set(MC_LINK_LIBS ${MC_LINK_LIBS} MediaCodec ${CODEC_FFMPEG_LIB_PATH} ${CODEC_FONTCONF_LIB_PATH} ${CODEC_HARFBUZZ_LIB_PATH} ${CODEC_FREETYPE_LIB_PATH} 
    ${CODEC_XML2_LIB_PATH} ${CODEC_X264_LIB_PATH} ${CODEC_X265_LIB_PATH}) 
else()  
endif()  
target_include_directories(MediaConvert PUBLIC
    ${MC_INCLUDE_DIRS}
)
target_link_libraries(MediaConvert PUBLIC ${MC_LINK_LIBS}) 
install(TARGETS MediaConvert DESTINATION .) #安装目录 目录MediaConvert换成目录. 也可以，这样就在当前目录生成不会生成在MediaConvert目录下


if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
add_subdirectory(test/cJSON-1.7.12) 
add_subdirectory(test/http) 
add_subdirectory(test/net) 
# 收集源文件  
file(GLOB test_list 
    test/main.cpp
    test/HttpServerDemo.cpp
    test/HttpServerIO.cpp
    test/HttpServerSession.cpp
    test/HttpTestCase.cpp
    demo/MediaConvert.cpp
    )  # 可以根据你的目录结构调整 
add_executable(MediaConvertTest ${test_list})

target_include_directories(MediaConvertTest PUBLIC
    ${MC_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/test
    ${PROJECT_SOURCE_DIR}/test/http/include
    ${PROJECT_SOURCE_DIR}/test/net/include
    ${PROJECT_SOURCE_DIR}/test/cJSON-1.7.12
)
target_link_libraries(MediaConvertTest PUBLIC
    ${MC_LINK_LIBS}
    http
    net
    cJSON
) 
install(TARGETS MediaConvertTest DESTINATION .) #安装目录 目录MediaConvert换成目录. 也可以，这样就在当前目录生成不会生成在MediaConvert目录下
endif()  



#SET_TARGET_PROPERTIES(base64_static PROPERTIES OUTPUT_NAME base64)
#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)

#TARGET_LINK_LIBRARIES(webrtc pthread -lm)

#TARGET_LINK_LIBRARIES(crnode log4cpp thrift)










