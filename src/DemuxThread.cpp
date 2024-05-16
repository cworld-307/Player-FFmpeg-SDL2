#include "DemuxThread.h"
#include "FFmpegPlayer.h"
#include <functional>


DemuxThread::DemuxThread()
{

}

void DemuxThread::setPlayerCtx(FFmpegPlayerCtx* ctx)
{
	is = ctx;
}

int DemuxThread::initDemuxThread()
{
    int err_code;
    char errors[1024] = { 0, };

    AVFormatContext* pFormatCtx = NULL;

    //avformat_open_input(),打开媒体文件，并将解析到的格式信息存储在is->pFormatCtx中
    if ((err_code = avformat_open_input(&pFormatCtx, is->filename, NULL, NULL)) < 0) {
        av_strerror(err_code, errors, 1024);
        fprintf(stderr, "Could not open source file %s, %d(%s)\n", is->filename, err_code, errors);
        return -1;
    }

    is->pFormatCtx = pFormatCtx;

    //avformat_find_stream_info函数用于获取媒体文件中的流信息，包括视频流、音频流等。
    // 它会读取媒体文件中的各个流，并解析出流的相关参数，例如编解码器类型、分辨率、码率等信息，
    // 并将这些信息填充到AVFormatContext结构体中。
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        return -1; // Couldn't find stream information
    }
        
    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, is->filename, 0);

    if (stream_open(is, AVMEDIA_TYPE_AUDIO) < 0)
    {
        fprintf(stderr, "%s: could not open audio\n", is->filename);
        return -1;
    }

    if (stream_open(is, AVMEDIA_TYPE_VIDEO) < 0)
    {
        fprintf(stderr, "%s: could not open video\n", is->filename);
        return -1;
    }
    return 0;
}

void DemuxThread::finiDemuxThread()
{
    if (is->pFormatCtx)
    {
        avformat_close_input(&is->pFormatCtx);
        is->pFormatCtx = nullptr;
    }
    if (is->audio_ctx)
    {
        avcodec_free_context(&is->audio_ctx);
        is->audio_ctx = nullptr;
    }
    if (is->video_ctx)
    {
        avcodec_free_context(&is->video_ctx);
        is->video_ctx = nullptr;
    }
    if (is->audio_swr_ctx)
    {
        swr_free(&is->audio_swr_ctx);
        is->audio_swr_ctx = nullptr;
    }
    if (is->video_sws_ctx)
    {
        sws_freeContext(is->video_sws_ctx);
        is->video_sws_ctx = nullptr;
    }
}

void DemuxThread::run()
{
    decode_loop();
}

void DemuxThread::stop()
{
    //停止解封装线程的逻辑
}

int DemuxThread::decode_loop()
{
    AVPacket* packet = av_packet_alloc();

    for (;;) {
        if (is->quit) {
            break;
        }
        // seek stuff goes here
        if (is->audioq.size > MAX_AUDIOQ_SIZE ||
            is->videoq.size > MAX_VIDEOQ_SIZE) {
            SDL_Delay(10);
            continue;
        }
        //从is->pFormatCtx（包含了输入媒体文件的信息）中读取下一个数据包，并将其存储在提供的AVPacket结构中。
        if (av_read_frame(is->pFormatCtx, packet) < 0) {
            if (is->pFormatCtx->pb->error == 0) {
                SDL_Delay(100); /* no error; wait for user input */
                continue;
            }
            else {
                break;
            }
        }
        // Is this a packet from the video stream?
        if (packet->stream_index == is->videoStream) {
            //视频包送入视频队列
            packet_queue_put(&is->videoq, packet);
        }
        else if (packet->stream_index == is->audioStream) {
            //音频包送入音频队列
            packet_queue_put(&is->audioq, packet);
        }
        else {
            av_packet_unref(packet);
        }
    }
    /* all done - wait for it */
    while (!is->quit) {
        SDL_Delay(100);
    }
    av_packet_free(&packet);

    return 0;
}

int DemuxThread::audio_decode_frame(FFmpegPlayerCtx* is, double* pts_ptr)
{
    return 0;
}

int DemuxThread::stream_open(FFmpegPlayerCtx* is, int media_type)
{
    AVFormatContext* pFormatCtx = is->pFormatCtx;
    AVCodecContext* codecCtx = NULL;
    const AVCodec* codec = NULL;

    int stream_index = av_find_best_stream(pFormatCtx, (AVMediaType)media_type, -1, -1, &codec, 0);

    if (stream_index < 0 || stream_index >= pFormatCtx->nb_streams) {
        return -1;
    }

    codecCtx = avcodec_alloc_context3(codec);
    int ret = avcodec_parameters_to_context(codecCtx, pFormatCtx->streams[stream_index]->codecpar);
    if (ret < 0)
        return -1;

    // 打开解码器
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }

    switch (codecCtx->codec_type) {
        // 音频流配置AVFormatContext,并对音频重采样
    case AVMEDIA_TYPE_AUDIO:

        is->audioStream = stream_index;
        is->audio_st = pFormatCtx->streams[stream_index];
        is->audio_ctx = codecCtx;
        is->audio_buf_size = 0;

        is->audio_buf_index = 0;
        memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
        packet_queue_init(&is->audioq);

        //Out Audio Param
        uint64_t out_channel_layout;
        out_channel_layout = AV_CH_LAYOUT_STEREO;

        int out_nb_samples;
        out_nb_samples = is->audio_ctx->frame_size;

        int out_sample_rate;
        out_sample_rate = is->audio_ctx->sample_rate;
        int out_channels;
        out_channels = av_get_channel_layout_nb_channels(out_channel_layout);


        int64_t in_channel_layout;
        in_channel_layout = av_get_default_channel_layout(is->audio_ctx->channels);

        // 音频重采样
        is->audio_swr_ctx = swr_alloc();
        //mp3:is->audio_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP（8）
        //aac:                            AV_SAMPLE_FMT_FLTP（8）
        swr_alloc_set_opts(is->audio_swr_ctx,
            out_channel_layout,
            AV_SAMPLE_FMT_S16,
            out_sample_rate,
            in_channel_layout,
            is->audio_ctx->sample_fmt,
            is->audio_ctx->sample_rate,
            0,
            NULL);

        swr_init(is->audio_swr_ctx);
        break;

    case AVMEDIA_TYPE_VIDEO:
        is->videoStream = stream_index;
        is->video_st = pFormatCtx->streams[stream_index];
        is->video_ctx = codecCtx;

        is->frame_timer = (double)av_gettime() / 1000000.0;
        is->frame_last_delay = 40e-3;
        is->video_current_pts_time = av_gettime();
        packet_queue_init(&is->videoq);

        is->video_sws_ctx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
            codecCtx->width, codecCtx->height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, NULL, NULL, NULL);
        break;
    default:
        break;
    }
    return 0;
}
