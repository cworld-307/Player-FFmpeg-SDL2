#include "FFmpegPlayer.h"
#include "RenderView.h"
#include <functional>
#include "SDLApp.h"

#undef  main;

int main()
{
    // ��ʼ��SDLAppʵ��
    new SDLApp();

    //// ��ʼ����ͼ�Ͷ�ʱ��
    //RenderView view;
    //view.initSDL();

    //Timer ti;
    //std::function<void()> cb = std::bind(&RenderView::onRefresh, &view);
    //ti.start(&cb, 30);


    // ��ʼ��FFmpeg������
    FFmpegPlayer player;
    player.setFilePath("d:/t.mp4");
    if (player.initPlayer() != 0)
    {
        return -1;
    }

    // ��ʼ������Ƶ
    player.start();

    // ����SDLӦ�ó����¼�ѭ��
    sdlApp->exec();

    return 0;
}
