#pragma once
#ifndef FFMPEGPLAYCTX_H
#define FFMPEGPLAYCTX_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
}
#include "PacketQueue.h"
#include "RenderView.h"
#include <atomic>

#define MAX_AUDIO_FRAME_SIZE 192000 //channels(2) * data_size(2) * sample_rate(48000)
#define VIDEO_PICTURE_QUEUE_SIZE 1
#define PAUSE 1 
#define UNPAUSE 0


typedef struct FFmpegPlayerCtx
{
    //multi-media file
    char filename[1024];
    AVFormatContext* pFormatCtx = nullptr;
    int videoStream = -1;
    int audioStream = -1;

    int64_t video_current_pts_time = 0;///<time (av_gettime) at which we updated video_current_pts - used to have running video pts

    //audio
    AVStream* audio_st = nullptr;
    AVCodecContext* audio_ctx = nullptr;
    PacketQueue audioq;
    uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    unsigned int audio_buf_size = 0;
    unsigned int audio_buf_index = 0;
    AVFrame* audio_frame = nullptr;
    AVPacket audio_pkt;
    uint8_t* audio_pkt_data = nullptr;
    int audio_pkt_size = 0;
    int audio_hw_buf_size = 0;
    SwrContext* audio_swr_ctx = nullptr;
    std::atomic<int> pause{UNPAUSE};

    //video
    AVStream* video_st = nullptr;
    AVCodecContext* video_ctx = nullptr;
    PacketQueue videoq;
    SwsContext* video_sws_ctx = nullptr;

    VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
    int pictq_size = 0;
    int pictq_rindex = 0;
    int pictq_windex = 0;
    SDL_mutex* pictq_mutex = nullptr;
    SDL_cond* pictq_cond = nullptr;

    SDL_Thread* parse_tid = nullptr;
    SDL_Thread* video_tid = nullptr;

    //用以做音视频同步的参数
    double audio_clock = 0.0;
    double frame_timer = 0.0;
    double frame_last_pts = 0.0;
    double frame_last_delay = 0.0;
    double video_clock = 0.0;
    double video_current_pts = 0.0;

    //图像回调
    //Image_Cb imgCb = nullptr;
    void* cbData = nullptr;

    int quit = 0;

    void init()
    {
        
        audio_frame = av_frame_alloc();

        pictq_mutex = SDL_CreateMutex();
        pictq_cond = SDL_CreateCond();
    }

    void fini()
    {
        if (audio_frame)
        {
            av_frame_free(&audio_frame);
        }
        
        if (pictq_mutex)
        {
            SDL_DestroyMutex(pictq_mutex);
        }
        if (pictq_cond)
        {
            SDL_DestroyCond(pictq_cond);
        }
    }

}FFmpegPlayerCtx;
#endif