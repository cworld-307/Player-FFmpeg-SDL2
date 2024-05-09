#include "AudioDecodeThread.h"
#include <assert.h>

double get_audio_clock(FFmpegPlayerCtx* is)
{
    double pts;
    int hw_buf_size, bytes_per_sec, n;

    //上一步获取的PTS
    pts = is->audio_clock;
    // 音频缓冲区还没有播放的数据
    hw_buf_size = is->audio_buf_size - is->audio_buf_index;
    // 每秒钟音频播放的字节数
    bytes_per_sec = 0;
    n = is->audio_ctx->channels * 2;
    if (is->audio_st) {
        bytes_per_sec = is->audio_ctx->sample_rate * n;
    }
    if (bytes_per_sec) {
        pts -= (double)hw_buf_size / bytes_per_sec;
    }
    return pts;
}

AudioDecodeThread::AudioDecodeThread()
{
}

void AudioDecodeThread::setPlayerCtx(FFmpegPlayerCtx* ctx)
{
    playCtx = ctx;
}

void AudioDecodeThread::run()
{
}

void AudioDecodeThread::stop()
{
    if (!playCtx)
        return;

    FFmpegPlayerCtx* is = playCtx;

    // 关闭音频解码器
    if (is->audio_ctx) {
        avcodec_free_context(&is->audio_ctx);
        is->audio_ctx = NULL;
    }

    // 清空音频队列
    packet_queue_flush(&is->audioq);
}


void AudioDecodeThread::getAudioData(unsigned char* stream, int len)
{
    FFmpegPlayerCtx* is = playCtx;
    //解码器未准备好或处于暂停状态，输出静音
    if (!is->audio_ctx || is->pause == PAUSE)
    {
        memset(stream, 0, len);
        return;
    }
    int len1, audio_size;
    double pts;

    SDL_memset(stream, 0, len);

    while (len > 0) {
        if (is->audio_buf_index >= is->audio_buf_size) {
            // 音频解码
            audio_size = audio_decode_frame(is, &pts);
            if (audio_size < 0) {
                // 音频解码错误，播放静音      
                is->audio_buf_size = 1024;
                memset(is->audio_buf, 0, is->audio_buf_size);
            }
            else {
                is->audio_buf_size = audio_size;
            }
            is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len)
            len1 = len;
        memcpy(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, len1);
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
}


int AudioDecodeThread::audio_decode_frame(FFmpegPlayerCtx* is, double* pts_ptr)
{
    int len1 = 0, data_size = 0, n;
    AVPacket* pkt = &is->audio_pkt;
    double pts;
    int ret = 0;

    for (;;) {
        while (is->audio_pkt_size > 0)
        {
            ret = avcodec_send_packet(is->audio_ctx, pkt);
            if (ret != 0)
            {
                //出错的时候跳过帧
                is->audio_pkt_size = 0;
                break;
            }
            av_frame_unref(is->audio_frame);
            ret = avcodec_receive_frame(is->audio_ctx, is->audio_frame);
            if (ret != 0)
            {
                //出错的时候跳过帧
                is->audio_pkt_size = 0;
                break;
            }

            if (ret == 0)
            {
                int upper_bound_samples = swr_get_out_samples(is->audio_swr_ctx, is->audio_frame->nb_samples);
                uint8_t* out[4] = { 0 };
                out[0] = (uint8_t*)av_malloc(upper_bound_samples * 2 * 2);

                //每个通道输出的采样
                int samples = swr_convert(is->audio_swr_ctx, out, upper_bound_samples,
                    (const uint8_t**)is->audio_frame->data, is->audio_frame->nb_samples);
                if (samples > 0)
                {
                    memcpy(is->audio_buf, out[0], samples * 2 * 2);
                }

                av_free(out[0]);
                data_size = samples * 2 * 2;
            }

            len1 = pkt->size;
            is->audio_pkt_data += len1;
            is->audio_pkt_size -= len1;

            if (data_size <= 0) {
                /* 没有获得数据 */
                printf("No data yet, get more frames\n");
                continue;
            }

            pts = is->audio_clock;
            *pts_ptr = pts;
            n = 2 * is->audio_ctx->ch_layout.nb_channels;
            is->audio_clock += (double)data_size / (double(n * (is->audio_ctx->sample_rate)));
            return data_size;

        }

        if (pkt->data)
            av_packet_unref(pkt);

        if (is->quit) {
            return -1;
        }

        if (packet_queue_get(&is->audioq, pkt, 1) < 0)
        {
            return -1;
        }

        is->audio_pkt_data = pkt->data;
        is->audio_pkt_size = pkt->size;

        if (pkt->pts != AV_NOPTS_VALUE)
        {
            is->audio_clock = av_q2d(is->audio_st->time_base) * pkt->pts;
        }
    }
}

