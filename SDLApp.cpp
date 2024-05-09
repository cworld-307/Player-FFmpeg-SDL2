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
			//������յ����˳��¼������û�����˹رմ��ڰ�ť���ͷ��� 0���˳�����
		case SDL_QUIT:
			return 0;
		
			//������յ����û��Զ����¼�������¼���������ȡ�ص���������ִ�С�
		case SDL_USEREVENT:
		{
			/*std::function<void()> cb = *(std::function<void()>*)event.user.data1;
			cb();*/
			// ���¼�����ת��Ϊָ�� FFmpegPlayer �����ָ��
			FFmpegPlayer* player = static_cast<FFmpegPlayer*>(event.user.data1);

			// ���� FFmpegPlayer ����� onRefreshEvent ��Ա����
			player->onRefreshEvent(&event);
		}
			break;
			//�����������͵��¼������Ȳ��Ҹ����͵��¼��Ƿ���ע��ص��������������ִ��ע��Ļص�������
		default:
			//iter�Ǹ��ݼ������ҵ��ļ�ֵ�Ե�����
			auto iter = m_userEventMaps.find(event.type);
			//�ڲ���Ԫ��ʱ���������ʧ�ܣ���������ָ��������ĩβ��
			if (iter != m_userEventMaps.end())
			{
				//iter->first ��ʾ����iter->second ��ʾֵ��
				//��iter->second��ʾ���¼����͵Ļص�����
				auto onEventCb = iter->second;
				//���ûص�����
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
