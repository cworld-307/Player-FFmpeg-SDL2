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
		fprintf(stderr, "VideoDecodeThread error��%d,%s\n", err_code, errors);
	}
}

void VideoDecodeThread::stop()
{
	if (!playCtx)
		return;

	FFmpegPlayerCtx* is = playCtx;

	// ���������ֹͣ��Ƶ������߼�
	
	// �ͷ���Ƶ���������Դ
	if (is->video_ctx) {
		avcodec_free_context(&is->video_ctx);
		is->video_ctx = NULL;
	}

	// �����Ƶ����
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
	//	//����Ƶ������ȡһ��packet
	//	if (packet_queue_get(&is->videoq, pkt, 1) < 0)
	//	{
	//		//error
	//		break;
	//	}

	//	//������Ƶ
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
	//		//��ʱ���ת��Ϊ������
	//		pts *= av_q2d(is->video_st->time_base);
	//		//printf("videoFrame PTS -> %f\n", pts);
	//		pts = synchronize_video(is, pFrame, pts);
	//		if (queue_picture(is, pFrame, pts) < 0) {
	//			break;
	//		}
	//		//��pkt����Ϊ��ʼ״̬
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
		//����Ƶ�����л�ȡһ�����ݰ����洢��packet�����С�
		if (packet_queue_get(&is->videoq, packet, 1) < 0) {
			// means we quit getting packets
			break;
		}

		// Decode video frame
		//avcodec_send_packet()���������ݰ����͸����������н��롣
		avcodec_send_packet(is->video_ctx, packet);
		//�ж�avcodec_receive_frame()�����Ƿ�ɹ����յ���������Ƶ֡���������ֵ����0����ʾ�ɹ����յ�������֡
		while (avcodec_receive_frame(is->video_ctx, pFrame) == 0) {
			if ((pts = pFrame->best_effort_timestamp) != AV_NOPTS_VALUE)
			{
			}
			else
			{
				pts = 0;
			}
			//printf("ת��ǰpts -> %f\n", pts);

			//����ʱ�����tbr��֡��ʱ�����tbn������ʱ�����tbc��������ʱ�����
			//��ʱ��� pts ������Ƶ����ʱ���(tbn)��������ʾ����ʱ���ת��Ϊ������
			pts = pts * av_q2d(is->video_st->time_base);
			/*printf("ʱ�����������ʾ��%f\n", av_q2d(is->video_st->time_base));
			printf("ʱ����ķ�ĸ��%d,ʱ����ķ��ӣ�%d\n", is->video_st->time_base.den, is->video_st->time_base.num);

			printf("ת����pts -> %f\n", pts);*/
			pts = synchronize_video(is, pFrame, pts);
			//printf("ͬ��pts -> %f\n", pts);
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
		//�����pts������Ƶʱ������Ϊptsֵ
		is->video_clock = pts;
	}
	else {
		//���û�����ù�pts����pts����Ϊʱ�ӵ�ֵ
		pts = is->video_clock;
	}
	//������Ƶ��ʱ��
	frame_delay = av_q2d(is->video_ctx->time_base);
	//����ظ�1֡������Ӧ����һ��ʱ��
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
