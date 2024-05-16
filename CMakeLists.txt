cmake_minimum_required(VERSION 3.17)
project(SIMPLE-PLAYER)

# ����ffmpeg�����⼰ͷ�ļ�����Ŀ¼�������ָ������
set(ffmpeg_libs_DIR /usr/local/ffmpeg/lib)
set(ffmpeg_headers_DIR /usr/local/ffmpeg/include)

#����sdl�����⼰ͷ�ļ�����Ŀ¼��������ָ������
set(sdl_libs_DIR /usr/lib/x86_64-linux-gnu)
set(sdl_headers_DIR /usr/include/SDL2)

# ���Ӷ��cpp�ļ�
aux_source_directory(src SRC)

# ����ͷ�ļ�·������������ͷ�ļ�����·���£����·���Կո�ָ�
include_directories(include ${ffmpeg_headers_DIR} ${sdl_headers_DIR})
link_directories(${ffmpeg_libs_DIR} ${sdl_libs_DIR})
# ��̬������
# link_libraries(
#     avformat
#     avcodec
#     avutil
#     swresample
#     swscale
#     SDL2
#     )

# ��CMAKE_CXX_FLAGS����ѡ���׷��-std=c++11
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(Player main.cpp ${SRC})

# ����̬���ļ����ӵ�Player
target_link_libraries(Player
    avformat
    avcodec
    avutil
    swresample
    swscale
    SDL2
)
