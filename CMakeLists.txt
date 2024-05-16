cmake_minimum_required(VERSION 3.17)
project(SIMPLE-PLAYER)

# 设置ffmpeg依赖库及头文件所在目录，并存进指定变量
set(ffmpeg_libs_DIR /usr/local/ffmpeg/lib)
set(ffmpeg_headers_DIR /usr/local/ffmpeg/include)

#设置sdl依赖库及头文件所在目录，并存入指定变量
set(sdl_libs_DIR /usr/lib/x86_64-linux-gnu)
set(sdl_headers_DIR /usr/include/SDL2)

# 添加多个cpp文件
aux_source_directory(src SRC)

# 添加头文件路径到编译器的头文件搜索路径下，多个路径以空格分隔
include_directories(include ${ffmpeg_headers_DIR} ${sdl_headers_DIR})
link_directories(${ffmpeg_libs_DIR} ${sdl_libs_DIR})
# 静态库链接
# link_libraries(
#     avformat
#     avcodec
#     avutil
#     swresample
#     swscale
#     SDL2
#     )

# 在CMAKE_CXX_FLAGS编译选项后追加-std=c++11
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(Player main.cpp ${SRC})

# 将动态库文件链接到Player
target_link_libraries(Player
    avformat
    avcodec
    avutil
    swresample
    swscale
    SDL2
)

