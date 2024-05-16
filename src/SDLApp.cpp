#include "SDLApp.h"
#include "FFmpegPlayer.h"

#define SDL_APP_EVENT_TIMEOUT (1)

static SDLApp* globalInstance = nullptr;

SDLApp::SDLApp()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	if (!globalInstance)
	{
		globalInstance = this;
	}
	else
	{
		fprintf(stderr, "only one instance allowed\n");
		exit(1);
	}
}

int SDLApp::exec()
{
	SDL_Event event;
	for (;;)
	{
		SDL_WaitEventTimeout(&event, SDL_APP_EVENT_TIMEOUT);
		switch(event.type)
		{
			//如果接收到了退出事件，即用户点击了关闭窗口按钮，就返回 0，退出程序。
		case SDL_QUIT:
			return 0;
		
			//如果接收到了用户自定义事件，则从事件数据中提取回调函数，并执行。
		case SDL_USEREVENT:
		{
			/*std::function<void()> cb = *(std::function<void()>*)event.user.data1;
			cb();*/
			// 将事件数据转换为指向 FFmpegPlayer 对象的指针
			FFmpegPlayer* player = static_cast<FFmpegPlayer*>(event.user.data1);

			// 调用 FFmpegPlayer 对象的 onRefreshEvent 成员函数
			player->onRefreshEvent(&event);
		}
			break;
			//对于其他类型的事件，首先查找该类型的事件是否有注册回调函数，如果有则执行注册的回调函数。
		default:
			//iter是根据键，查找到的键值对迭代器
			auto iter = m_userEventMaps.find(event.type);
			//在查找元素时，如果查找失败，迭代器会指向容器的末尾。
			if (iter != m_userEventMaps.end())
			{
				//iter->first 表示键，iter->second 表示值。
				//即iter->second表示该事件类型的回调函数
				auto onEventCb = iter->second;
				//调用回调函数
				onEventCb(&event);
			}
			break;

		}
	}
}

void SDLApp::quit()
{
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

void SDLApp::registerEvent(int type, const std::function<void(SDL_Event*)>& cb)
{
	m_userEventMaps[type] = cb;
}

SDLApp* SDLApp::instance()
{

	return globalInstance;
}
