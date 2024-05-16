#pragma once
#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

extern "C" {
#include <SDL.h>
#include <libavformat/avformat.h>
#include <libavutil/fifo.h>
}

typedef struct MyAVPacketList {
    AVPacket* pkt = nullptr;
    int serial;
} MyAVPacketList;

typedef struct VideoPicture {
    AVFrame* frame = nullptr;
    int width=0, height=0; /* source height & width */
    double pts;
} VideoPicture;

typedef struct PacketQueue {
    AVFifo* pkt_list = nullptr;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    SDL_mutex* mutex = nullptr;
    SDL_cond* cond = nullptr;
} PacketQueue;

int packet_queue_init(PacketQueue* q);
int packet_queue_put(PacketQueue* q, AVPacket* pkt);
int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block);
void packet_queue_flush(PacketQueue* q);
#endif /* PACKET_QUEUE_H */
