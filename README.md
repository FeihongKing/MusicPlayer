# 音乐播放器

#### 项目介绍
基于Qt开发的音乐播放器，可解码多种音频文件格式，可通过网络控制播放器状态。本程序在Linux系统下开发完成，使用了部分Linux系统下的共享库文件，其它操作系统编译请更改位于pro文件下面的库文件地址。

编译方式

    $ git clone https://github.com/FeihongKing/MusicPlayer.git
    $ cd MusicPlayer
    $ mkdir build
    $ cd build
    $ qmake ..
    $ make 
    $ ./MusicPlayer
