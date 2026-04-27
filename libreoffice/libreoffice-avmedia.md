# LibreOffice 幻灯片播放内嵌视频

部分 LibreOffice 源码，

https://github.com/LibreOffice/core/blob/master/avmedia/source/gstreamer/gstplayer.cxx

``` C++
void Player::preparePlaybin( std::u16string_view rURL, GstElement *pSink )
{
    mpPlaybin = gst_element_factory_make( "playbin", nullptr );
    // ...
    GstElement *pAudioSink = gst_element_factory_make( "autoaudiosink", nullptr );
    //...
    g_object_set(G_OBJECT(mpPlaybin), "audio-sink", pAudioOutput, nullptr);
    // ...
}
```

播放 MP4 文件时大概等价于 gstreamer 命令行

``` Sh
gst-launch-1.0 playbin uri="file:///path/to/media.mp4" \
    audio-sink="autoaudiosink" \
    video-sink="autovideosink"
```

`autoaudiosink` 自动选择当前系统最好的可用音频输出，不依赖特定后端
