#include "FFmpegPlayer.h"
#include "RenderView.h"
#include <functional>
#include "SDLApp.h"

#undef  main;

int main()
{
    // 初始化SDLApp实例
    new SDLApp();

    //// 初始化视图和定时器
    //RenderView view;
    //view.initSDL();

    //Timer ti;
    //std::function<void()> cb = std::bind(&RenderView::onRefresh, &view);
    //ti.start(&cb, 30);


    // 初始化FFmpeg播放器
    FFmpegPlayer player;
    player.setFilePath("d:/t.mp4");
    if (player.initPlayer() != 0)
    {
        return -1;
    }

    // 开始播放视频
    player.start();

    // 启动SDL应用程序事件循环
    sdlApp->exec();

    return 0;
}
