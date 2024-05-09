#include "VideoDecodeThread.h"

VideoDecodeThread::VideoDecodeThread()
{
}

void VideoDecodeThread::setPlayerCtx(FFmpegPlayerCtx* ctx)
{
	playCtx = ctx;
}

void VideoDecodeThread::run()
{
	int err_code;
	char errors[1024] = { 0, };
	if ((err_code = video_entry()) < 0)
	{
		av_strerror(err_code, errors, 1024);
		fprintf(stderr, "VideoDecodeThread error：%d,%s\n", err_code, errors);
	}
}

void VideoDecodeThread::stop()
{
	if (!playCtx)
		return;

	FFmpegPlayerCtx* is = playCtx;

	// 在这里添加停止视频解码的逻辑
	
	// 释放视频解码相关资源
	if (is->video_ctx) {
		avcodec_free_context(&is->video_ctx);
		is->video_ctx = NULL;
	}

	// 清空视频队列
	packet_queue_flush(&is->videoq);
}

int VideoDecodeThread::video_entry()
{
	FFmpegPlayerCtx* is = playCtx;
	//AVPacket* pkt = av_packet_alloc();
	//AVCodecContext* pCodecCtx = is->video_ctx;
	//int ret = -1;
	//double pts = 0;

	//AVFrame* pFrame = av_frame_alloc();
	//AVFrame* pFrameRGB = av_frame_alloc();

	//for (;;)
	//{	
	//	//从视频队列中取一个packet
	//	if (packet_queue_get(&is->videoq, pkt, 1) < 0)
	//	{
	//		//error
	//		break;
	//	}

	//	//解码视频
	//	ret = avcodec_send_packet(pCodecCtx, pkt);
	//	if (ret < 0)
	//	{
	//		//error
	//		break;
	//	}
	//	while (avcodec_receive_frame(is->video_ctx, pFrame) == 0) {
	//		if ((pts = pFrame->best_effort_timestamp) != AV_NOPTS_VALUE)
	//		{
	//		}
	//		else
	//		{
	//			pts = 0;
	//		}
	//		//将时间戳转换为秒数。
	//		pts *= av_q2d(is->video_st->time_base);
	//		//printf("videoFrame PTS -> %f\n", pts);
	//		pts = synchronize_video(is, pFrame, pts);
	//		if (queue_picture(is, pFrame, pts) < 0) {
	//			break;
	//		}
	//		//将pkt重置为初始状态
	//		av_packet_unref(pkt);
	//	}
	//	
	//}

	//return 0;
	AVPacket pkt1, * packet = &pkt1;
	AVFrame* pFrame;
	double pts;

	pFrame = av_frame_alloc();

	for (;;) {
		//从视频队列中获取一个数据包并存储在packet变量中。
		if (packet_queue_get(&is->videoq, packet, 1) < 0) {
			// means we quit getting packets
			break;
		}

		// Decode video frame
		//avcodec_send_packet()函数将数据包发送给解码器进行解码。
		avcodec_send_packet(is->video_ctx, packet);
		//判断avcodec_receive_frame()函数是否成功接收到解码后的视频帧。如果返回值等于0，表示成功接收到解码后的帧
		while (avcodec_receive_frame(is->video_ctx, pFrame) == 0) {
			if ((pts = pFrame->best_effort_timestamp) != AV_NOPTS_VALUE)
			{
			}
			else
			{
				pts = 0;
			}
			//printf("转换前pts -> %f\n", pts);

			//三种时间基：tbr，帧率时间基。tbn，流的时间基。tbc，解码器时间基。
			//将时间戳 pts 乘以视频流的时间基(tbn)的秒数表示，将时间戳转换为秒数。
			pts = pts * av_q2d(is->video_st->time_base);
			/*printf("时间基的秒数表示：%f\n", av_q2d(is->video_st->time_base));
			printf("时间基的分母：%d,时间基的分子：%d\n", is->video_st->time_base.den, is->video_st->time_base.num);

			printf("转换后pts -> %f\n", pts);*/
			pts = synchronize_video(is, pFrame, pts);
			//printf("同步pts -> %f\n", pts);
			if (queue_picture(is, pFrame, pts) < 0) {
				break;
			}
			av_packet_unref(packet);
		}
	}
	av_frame_free(&pFrame);
	return 0;
}

double VideoDecodeThread::synchronize_video(FFmpegPlayerCtx* is, AVFrame* src_frame, double pts)
{
	double frame_delay;

	if (pts != 0) {
		//如果有pts，则将视频时钟设置为pts值
		is->video_clock = pts;
	}
	else {
		//如果没有设置过pts，则将pts设置为时钟的值
		pts = is->video_clock;
	}
	//更新视频计时器
	frame_delay = av_q2d(is->video_ctx->time_base);
	//如果重复1帧，就相应调整一下时钟
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	is->video_clock += frame_delay;
	
	return pts;
}

int VideoDecodeThread::queue_picture(FFmpegPlayerCtx* is, AVFrame* pFrame, double pts)
{
	VideoPicture* vp;

	/* wait until we have space for a new pic */
	SDL_LockMutex(is->pictq_mutex);
	while (is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !is->quit) {
		SDL_CondWait(is->pictq_cond, is->pictq_mutex);
	}
	SDL_UnlockMutex(is->pictq_mutex);

	if (is->quit)
		return -1;

	// windex is set to 0 initially
	vp = &is->pictq[is->pictq_windex];

	//    /* allocate or resize the buffer! */
	if (!vp->frame ||
		vp->width != is->video_ctx->width ||
		vp->height != is->video_ctx->height) 
	{
		vp->frame = av_frame_alloc();
		if (is->quit) 
		{
			return -1;
		}
	}

	/* We have a place to put our picture on the queue */
	if (vp->frame) {

		vp->pts = pts;

		vp->frame = pFrame;
		/* now we inform our display thread that we have a pic ready */
		if (++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) {
			is->pictq_windex = 0;
		}

		SDL_LockMutex(is->pictq_mutex);
		is->pictq_size++;
		SDL_UnlockMutex(is->pictq_mutex);
	}

	return 0;
}
