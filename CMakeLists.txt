cmake_minimum_required(VERSION 3.10) 


project (media)
 
#set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/ThirdLib/media) #CMAKE_INSTALL_PREFIX 外面设置，内部设置无效

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


# 收集源文件  
#file(GLOB demo_list demo/main.cpp)
#add_executable(MediaConvert ${demo_list})
 #set(CMAKE_EXECUTABLE_SUFFIX ".html") # 编译生成.html
#target_include_directories(MediaConvert PUBLIC ${PROJECT_SOURCE_DIR}/src/include)
#target_link_libraries(MediaConvert PUBLIC media) 
#install(TARGETS MediaConvert DESTINATION .) #安装目录 目录MediaConvert换成目录. 也可以，这样就在当前目录生成不会生成在MediaConvert目录下

#SET_TARGET_PROPERTIES(base64_static PROPERTIES OUTPUT_NAME base64)
#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)

#TARGET_LINK_LIBRARIES(webrtc pthread -lm)

#TARGET_LINK_LIBRARIES(crnode log4cpp thrift)










