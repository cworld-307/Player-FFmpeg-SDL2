#include "FFmpegPlayer.h"
#include "SDLApp.h"
#include <libavutil/time.h>
#include "AudioDecodeThread.h"

static int screen_left = SDL_WINDOWPOS_CENTERED;
static int screen_top = SDL_WINDOWPOS_CENTERED;
static int screen_width = 0;
static int screen_height = 0;
static int resize = 1;

FFmpegPlayer::FFmpegPlayer()
{
}

void FFmpegPlayer::setFilePath(const char* filePath)
{
	m_filePath = filePath;
}

//void FFmpegPlayer::setImageCb(Image_Cb cb, void* userData)
//{
//	playerCtx.imgCb = cb;
//	playerCtx.cbData = userData;
//}


void FFmpegPlayer::AudioDecodeThread_callback(void* userdata, unsigned char* stream, int len)
{
	// 将 userdata 转换为 AudioDecodeThread 实例指针
	AudioDecodeThread* audioDecodeThread = static_cast<AudioDecodeThread*>(userdata);
	// 调用成员函数 getAudioData
	audioDecodeThread->getAudioData(stream, len);
}


#define FF_REFRESH_EVENT (SDL_USEREVENT)

//// 定时器回调函数，发送FF_REFRESH_EVENT事件，更新显示视频帧
//event.user.data1 = is
static Uint32 sdl_refresh_timer_cb(Uint32 interval, void* opaque) {
	SDL_Event event;
	event.type = FF_REFRESH_EVENT;
	event.user.data1 = opaque;
	SDL_PushEvent(&event);
	return 0;
}

//// 设置定时器
static void schedule_refresh(FFmpegPlayerCtx* is, int delay) {
	SDL_AddTimer(delay, sdl_refresh_timer_cb, is);
}


SDL_mutex* text_mutex;
SDL_Window* win = NULL;
SDL_Texture* texture;
SDL_Renderer* renderer;

//// 视频播放
static void video_display(FFmpegPlayerCtx* is) {

	SDL_Rect rect;
	VideoPicture* vp;

	//if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
	//	fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
	//	exit(1);
	//}

	screen_width = is->video_ctx->width;
	screen_height = is->video_ctx->height;

	if (screen_width && resize) {
		SDL_SetWindowSize(win, screen_width, screen_height);
		SDL_SetWindowPosition(win, screen_left, screen_top);
		SDL_ShowWindow(win);

		Uint32 pixformat = SDL_PIXELFORMAT_IYUV;

		//create texture for render
		texture = SDL_CreateTexture(renderer,
			pixformat,
			SDL_TEXTUREACCESS_STREAMING,
			screen_width,
			screen_height);
		resize = 0;
	}

	vp = &is->pictq[is->pictq_rindex];

	// 渲染播放
	if (vp->frame) {
		SDL_UpdateYUVTexture(texture, NULL,
			vp->frame->data[0], vp->frame->linesize[0],
			vp->frame->data[1], vp->frame->linesize[1],
			vp->frame->data[2], vp->frame->linesize[2]);

		rect.x = 0;
		rect.y = 0;
		rect.w = is->video_ctx->width;
		rect.h = is->video_ctx->height;
		SDL_LockMutex(text_mutex);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, &rect);
		SDL_RenderPresent(renderer);
		SDL_UnlockMutex(text_mutex);
	}
}



int FFmpegPlayer::initPlayer()
{
	//创建SDL Window
	win = SDL_CreateWindow("Media Player",
		100,
		100,
		1280, 720,
		SDL_WINDOW_RESIZABLE);
	if (!win) {
		fprintf(stderr, "\nSDL: could not set video mode:%s - exiting\n", SDL_GetError());
		exit(1);
	}
	//创建渲染器
	renderer = SDL_CreateRenderer(win, -1, 0);


	//初始化播放器上下文
	playerCtx.init();
	m_filePath.copy(playerCtx.filename, m_filePath.size());
	playerCtx.filename[m_filePath.size()] = '\0';

	//创建demux线程
	m_demuxThread = new DemuxThread;
	m_demuxThread->setPlayerCtx(&playerCtx);
	if (m_demuxThread->initDemuxThread() != 0)
	{
		fprintf(stderr, "DemuxThread init Failed");
		return -1;
	}

	//创建音频解码线程
	m_audioDecodeThread = new AudioDecodeThread;
	m_audioDecodeThread->setPlayerCtx(&playerCtx);
	
	//创建视频解码线程
	m_videoDecodeThread = new VideoDecodeThread;
	m_videoDecodeThread->setPlayerCtx(&playerCtx);

	//创建Timer定时器
	//m_timer = new Timer;
	
	//设置音频播放参数
	audio_wanted_spec.freq = 48000;
	audio_wanted_spec.format = AUDIO_S16SYS;
	audio_wanted_spec.channels = 2;
	audio_wanted_spec.silence = 0;
	audio_wanted_spec.samples = playerCtx.audio_ctx->frame_size;
	audio_wanted_spec.callback = AudioDecodeThread_callback;
	audio_wanted_spec.userdata = m_audioDecodeThread;
	//创建并打开音频播放设备
	m_audioPlay = new AudioPlay;
	if (m_audioPlay->openDevice(&audio_wanted_spec) < 0)
	{
		fprintf(stderr, "open audio device Failed");
		return -1;
	}

	//设置播放器事件
	auto refreshEvent = [this](SDL_Event* e)
		{
			onRefreshEvent(e);
		};

	auto keyEvent = [this](SDL_Event* e)
		{
			onKeyEvent(e);
		};

	sdlApp->registerEvent(FF_REFRESH_EVENT, refreshEvent);
	sdlApp->registerEvent(SDL_KEYDOWN, keyEvent);


	return 0;
}

void FFmpegPlayer::start()
{
	m_demuxThread->start();
	m_videoDecodeThread->start();
	m_audioDecodeThread->start();
	m_audioPlay->start();

	//m_timer->start(&playerCtx, 40);
	schedule_refresh(&playerCtx, 40);
	m_stop = false;
}

#define FREE(x) delete x; x = nullptr

void FFmpegPlayer::stop()
{
	m_stop = true;
	//停止音频解码线程
	printf("audio decode thread clean...");
	if (m_audioDecodeThread)
	{
		m_audioDecodeThread->stop();
		FREE(m_audioDecodeThread);
	}
	printf("audio decode thread finished...");

	//停止音频处理线程
	printf("audio play thread clean...");
	if (m_audioPlay)
	{
		m_audioPlay->stop();
		FREE(m_audioPlay);
	}
	printf("audio device finished...");

	//停止视频解码线程
	printf("video decode thread clean...");
	if (m_videoDecodeThread)
	{
		m_videoDecodeThread->stop();
		FREE(m_videoDecodeThread);
	}
	printf("video decode thread finished...");

	//停止demux线程
	printf("demux thread clean...");
	if (m_demuxThread)
	{
		m_demuxThread->stop();
		m_demuxThread->finiDemuxThread();
		FREE(m_demuxThread);
	}
	printf("demux thread finished...");

	printf("player ctx clean...");
	playerCtx.fini();
	printf("player ctx finished...");
}

//void FFmpegPlayer::pause(PauseState state)
//{
//	playerCtx.pause = state;
//	playerCtx.frame_timer = av_gettime() / 1000000.0;
//}

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0

void FFmpegPlayer::onRefreshEvent(SDL_Event* e)
{
	if(m_stop)
	{
		return;
	}

	FFmpegPlayerCtx* is = (FFmpegPlayerCtx*)e->user.data1;
	VideoPicture* vp;
	double actual_delay, delay, sync_threshold, ref_clock, diff;

	if (is->video_st) {
		if (is->pictq_size == 0) {
			//m_timer->start(is, 1);
			schedule_refresh(is, 1);
		}
		else {
			// 从数组中取出一帧视频帧
			vp = &is->pictq[is->pictq_rindex];

			is->video_current_pts = vp->pts;
			//printf("video_current_pts -> %f\n", is->video_current_pts);
			is->video_current_pts_time = av_gettime();
			// 当前Frame时间减去上一帧的时间，获取两帧间的时差
			delay = vp->pts - is->frame_last_pts;
			if (delay <= 0 || delay >= 1.0) {
				// 延时小于0或大于1秒（太长）都是错误的，将延时时间设置为上一次的延时时间
				delay = is->frame_last_delay;
			}
			// 保存延时和PTS，等待下次使用
			is->frame_last_delay = delay;
			is->frame_last_pts = vp->pts;

			// 获取音频Audio_Clock
			ref_clock = get_audio_clock(is);
			// 得到当前PTS和Audio_Clock的差值
			diff = vp->pts - ref_clock;

			/* Skip or repeat the frame. Take delay into account
			   FFPlay still doesn't "know if this is the best guess." */
			sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
			if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
				if (diff <= -sync_threshold) {
					delay = 0;
				}
				else if (diff >= sync_threshold) {
					delay = 2 * delay;
				}
			}
			is->frame_timer += delay;
			// 最终真正要延时的时间
			actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
			if (actual_delay < 0.010) {
				// 延时时间过小就设置最小值
				actual_delay = 0.010;
			}
			// 根据延时时间重新设置定时器，刷新视频
			//m_timer->start(is, (int)(actual_delay * 1000 + 0.5));
			schedule_refresh(is, (int)(actual_delay * 1000 + 0.5));
			// 视频帧显示
			video_display(is);

			// 更新视频帧数组下标
			if (++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) {
				is->pictq_rindex = 0;
			}
			SDL_LockMutex(is->pictq_mutex);
			// 视频帧数组减一
			is->pictq_size--;
			SDL_CondSignal(is->pictq_cond);
			SDL_UnlockMutex(is->pictq_mutex);
		}
	}
	else {
		//m_timer->start(is, 100);
		schedule_refresh(is, 100);
	}
}

void FFmpegPlayer::onKeyEvent(SDL_Event* e)
{
}

