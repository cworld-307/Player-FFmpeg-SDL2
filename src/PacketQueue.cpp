#include "PacketQueue.h"

//// 初始化队列
int packet_queue_init(PacketQueue* q)
{
    memset(q, 0, sizeof(PacketQueue));
    q->pkt_list = av_fifo_alloc2(1, sizeof(MyAVPacketList), AV_FIFO_FLAG_AUTO_GROW);
    if (!q->pkt_list)
        return AVERROR(ENOMEM);
    q->mutex = SDL_CreateMutex();
    if (!q->mutex) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    q->cond = SDL_CreateCond();
    if (!q->cond) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    q->abort_request = 0;
    return 0;
}

int packet_queue_put_private(PacketQueue* q, AVPacket* pkt)
{
    MyAVPacketList pkt1;
    int ret;

    if (q->abort_request)
        return -1;


    pkt1.pkt = pkt;
    //pkt1.serial = q->serial;

    ret = av_fifo_write(q->pkt_list, &pkt1, 1);
    if (ret < 0)
        return ret;
    q->nb_packets++;
    q->size += pkt1.pkt->size + sizeof(pkt1);
    q->duration += pkt1.pkt->duration;
    /* XXX: should duplicate packet data in DV case */
    SDL_CondSignal(q->cond);
    return 0;
}

int packet_queue_put(PacketQueue* q, AVPacket* pkt)
{
    AVPacket* pkt1;
    int ret;

    pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    SDL_LockMutex(q->mutex);
    ret = packet_queue_put_private(q, pkt1);
    SDL_UnlockMutex(q->mutex);

    if (ret < 0)
        av_packet_free(&pkt1);

    return ret;
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block) //int* serial
{
    MyAVPacketList pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        if (av_fifo_read(q->pkt_list, &pkt1, 1) >= 0) {
            q->nb_packets--;
            q->size -= pkt1.pkt->size + sizeof(pkt1);
            q->duration -= pkt1.pkt->duration;
            av_packet_move_ref(pkt, pkt1.pkt);
            //if (serial)
            //    *serial = pkt1.serial;
            av_packet_free(&pkt1.pkt);
            ret = 1;
            break;
        }
        else if (!block) {
            ret = 0;
            break;
        }
        else {
            //SDL_CondWait函数的参数是条件变量和互斥锁，
            // 它会在等待时释放互斥锁并阻塞当前线程，
            // 直到其他线程通过调用SDL_CondSignal或SDL_CondBroadcast发送信号通知条件变量满足,
            // 然后再重新获取互斥锁并继续执行。
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

void packet_queue_flush(PacketQueue* q)
{
    MyAVPacketList pkt1;

    SDL_LockMutex(q->mutex);
    while (av_fifo_read(q->pkt_list, &pkt1, 1) >= 0)
        av_packet_free(&pkt1.pkt);
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    q->serial++;
    SDL_UnlockMutex(q->mutex);
}

