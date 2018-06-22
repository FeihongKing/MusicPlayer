#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTime>
#include <QFileDialog>
#include <QProcessEnvironment>
#include <QDir>
#include <QFile>
#include <QDebug>

//TagLib头文件段
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/taglib.h>
#include <taglib/tpropertymap.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //setWindowFlags(Qt::FramelessWindowHint);        //去标题栏
    //showFullScreen();                               //全屏

    istimeSliderpressed = false;
    planMode = 0;                                                           //播放模式控制
    model = new QStandardItemModel(ui->treeView);
    ui->treeView->setModel(model);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);       //设置列表不可编辑
    model->setHorizontalHeaderLabels(QStringList() << "播放列表" << "路径");  //添加表头
    model->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignCenter);
    model->horizontalHeaderItem(0)->setBackground(QBrush(QColor(255,0,0,255)));
    //model->horizontalHeaderItem(0)->setForeground(QBrush(QColor(0,0,0,0)));
    ui->treeView->setColumnHidden(1,true);                                  //隐藏列，必须在表头添加之后设置才生效
    model->appendRow(new QStandardItem("添加路径"));                         //添加条目
    //model->item(model->rowCount()-1)->setChild(0,0,new QStandardItem("子列表"));
    ui->volumeSlider->setMaximum(100);                                      //设置音量横条最大值
    player = new QMediaPlayer(this);
    connect(player,SIGNAL(metaDataAvailableChanged(bool)),this,SLOT(is_mediaMeta_changed(bool)));
    connect(player,SIGNAL(stateChanged(QMediaPlayer::State)),this,SLOT(is_playerState_changed()));
    connect(player,SIGNAL(durationChanged(qint64)),this,SLOT(is_duration_changed(qint64)));
    connect(player,SIGNAL(positionChanged(qint64)),this,SLOT(is_position_changed(qint64)));
    connect(player,SIGNAL(volumeChanged(int)),this,SLOT(is_volume_changed(int)));
    connect(player,SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),this,SLOT(is_mediaState_changed()));
    ui->volumeSlider->setValue(player->volume());
    ui->volumeButton->setIcon(QIcon(":/image/image/volume_high_press.png"));
    ui->volumeButton->setStyleSheet("QToolButton:hover{background-color: rgba(255, 255, 255, 0);}");
    ui->forButton->setIcon(QIcon(":/image/image/next_press.png"));
    ui->forButton->setStyleSheet("QToolButton:hover{background-color: rgba(255, 255, 255, 0);}");
    ui->backButton->setIcon(QIcon(":/image/image/previous_press.png"));
    ui->backButton->setStyleSheet("QToolButton:hover{background-color: rgba(255, 255, 255, 0);}");
    ui->playButton->setIcon(QIcon(":/image/image/start_press.png"));
    ui->playButton->setStyleSheet("QToolButton:hover{background-color: rgba(255, 255, 255, 0);}");
    ui->planButton->setIcon(QIcon(":/image/image/sequence_press.png"));
    ui->planButton->setStyleSheet("QToolButton:hover{background-color: rgba(255, 255, 255, 0);}");

    lrcmodel = new QStandardItemModel(ui->lrcView);
    ui->lrcView->setModel(lrcmodel);

    audioProbe = new QAudioProbe();
    audioProbe->setSource(player);
    connect(audioProbe,SIGNAL(audioBufferProbed(QAudioBuffer)), this, SLOT(renderLevel(QAudioBuffer)));

    //ipstring = "192.168.2.1";

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::AnyIPv4,5555);
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(udpReced()));
    //udpSocket->writeDatagram("播放器已启动",QHostAddress(ipstring),5555);
    //udpSocket->writeDatagram("playerStart",QHostAddress::Broadcast,5555);

     ui->verticalStackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    ui->treeView->isExpanded(index)?ui->treeView->collapse(index):ui->treeView->expand(index);  //展开或收缩该节点

    if(model->itemFromIndex(index) == model->item(model->rowCount()-1))
    {
        QString mediaPath = QFileDialog::getExistingDirectory(this,"选择媒体路径",QProcessEnvironment::systemEnvironment().value("HOME"));
        if(mediaPath.isEmpty())
            return;
        QString listName = mediaPath.mid(mediaPath.lastIndexOf("/")+1);
        //qDebug() << listName;
        model->insertRow(model->rowCount()-1,new QStandardItem(listName));
        QDir mediaDir(mediaPath);
        QStringList fileList = mediaDir.entryList(QStringList() << "*.mp3" << "*.wav" << "*.ape" << "*.flac",QDir::Files|QDir::Readable,QDir::Name);
        foreach(QString fileName,fileList)
        {
            QStandardItem *item = model->item(model->rowCount()-2);
            int rowPosition = item->rowCount();
            item->setChild(rowPosition,new QStandardItem(fileName.mid(0,fileName.lastIndexOf("."))));   //文件名，不包含后缀
            item->setChild(rowPosition,1,new QStandardItem(mediaPath+"/"+fileName));
        }
        return;
    }

    if(model->index(index.row(),1,index.parent()).data().toString().isEmpty())   //若选中项目第二列为空，判定为一级节点，退出本函数
        return;

    playindex = index;
    playPosition = index.row();
    playCount = model->item(index.parent().row())->rowCount();
    QString filePath = model->index(index.row(),1,index.parent()).data().toString();
    player->setMedia(QUrl::fromLocalFile(filePath));
    player->play();
}

void MainWindow::is_mediaMeta_changed(bool)
{
    /*
    ui->titleLabel->setText(player->metaData("Title").toString());
    ui->artistLable->setText("歌手："+player->metaData("AlbumArtist").toString());
    ui->albumLabel->setText("专辑："+player->metaData("AlbumTitle").toString());
    */
    QString mediafile = model->index(playPosition,1,playindex.parent()).data().toString();  //从歌单列表模型中获取文件路径
    TagLib::FileRef file(mediafile.toStdString().c_str());
    QString title = QString::fromWCharArray(file.tag()->title().toCWString());
    if(title.isEmpty())
        {
            //title = mediafile.mid(mediafile.lastIndexOf("/",mediafile.length()-1),mediafile.lastIndexOf(".",mediafile.length()-1));
            title = mediafile;
            title = title.mid(title.lastIndexOf("/")+1);
            title = title.mid(0,title.lastIndexOf("."));
        }
    ui->titleLabel->setText(title);
    QString t = "title:" + title;
    QByteArray s;
    s.append(t);
    //udpSocket->writeDatagram(s,QHostAddress(ipstring),5555);
    udpSend(s);

    QString type = mediafile;
    type = type.mid(type.lastIndexOf(".")+1).toUpper();
    if(type == "APE")
        ui->typeLabel->setText("类型：Monkey's Audio");
    else if(type == "FLAC")
        ui->typeLabel->setText("类型：raw FLAC");
    else if(type == "MP3")
        ui->typeLabel->setText("类型：MP3 (MPEG audio layer 3)");
    else if(type == "WAV")
        ui->typeLabel->setText("类型：WAV / WAVE (Waveform Audio)");
    else
        ui->typeLabel->setText("类型：未知");

    QString album = "专辑：" + QString::fromWCharArray(file.tag()->album().toCWString());
    if(album=="专辑：")
        album+="未知";
    ui->albumLabel->setText(album);
    QString artist = "歌手：" + QString::fromWCharArray(file.tag()->artist().toCWString());
    if(artist=="歌手：")
        artist+="未知";
    ui->artistLable->setText(artist);
    TagLib::AudioProperties *properties = file.audioProperties();
    QString bitrate = QString::number(properties->bitrate()) + "kbps";
    QString samplerate = QString::number(properties->sampleRate()/1000.0) + "kHz";
    qDebug() << properties->channels();   //通道数
    QString channels;
    if(properties->channels() == 1)
        channels = "单声道";
    else
        channels = "立体声";

    QFile qfile(mediafile);
    QString fsize;
    float f = qfile.size();
    if(f > 1024*1024*1024)
    {
        f/=1024*1024*1024.00;
        fsize = QString::number(f,'f',2) + "G" + "(" + QString::number(qfile.size()) + ")";
    }
    else if(f > 1024*1024)
    {
        f/=1024*1024.00;
        fsize = QString::number(f,'f',2) + "M" + "(" + QString::number(qfile.size()) + ")";
    }
    else if(f > 1024)
    {
        f/=1024.00;
        fsize = QString::number(f,'f',2) + "K" + "(" + QString::number(qfile.size()) + ")";
    }
    else
        fsize = QString::number(qfile.size()) + "B";

    ui->infoLabel->setText("<html><head/><body><p>路径：" + mediafile + "</p><p>声道：" + channels + "</p><p>大小：" + fsize + "</p><p>" + ui->typeLabel->text() + "</p><p>比特率：" + bitrate + "</p><p>采样率：" + samplerate + "</p></body></html>");
    //qDebug() << "AudioBitRate" << player->metaData("AudioBitRate").toInt();
    //qDebug() << "ChannelCount" << player->metaData("ChannelCount").toInt();
    //qDebug() << "SampleRate" << player->metaData("SampleRate").toInt();
    //ui->titleLabel->setText("<html><head/><body><p><span style=\" font-size:14pt;\">" + title + "  " + "</span><span style=\" color:#2CA7F8;font-size:8pt;\">" + bitrate + '/' + samplerate +"</span></p></body></html>");
    //ui->treeView->setCurrentIndex(playPosition,1,playindex.parent());
    //ui->treeView->setCurrentIndex(playindex);
    qDebug() << qfile.handle();
    qDebug() << qfile.size();

    lrcload();

    //TagLib::FileRef f(mediafile.toStdString().c_str());
    //TagLib::Tag *tag = f.tag();
    //qDebug() << QString::fromWCharArray(tag->title().toCWString());
    ui->waveform->clearWave();
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::is_playerState_changed()
{
    int i;
    switch(player->state())
    {
    case QMediaPlayer::PlayingState:
        ui->playButton->setIcon(QIcon(":/image/image/pause_press.png"));
        ui->treeView->setCurrentIndex(model->index(playPosition,0,playindex.parent()));  //指示播放位置
        i=0;
        //is_mediaMeta_changed(true);         //为了兼容开发板的QT5.3，出此下策;开发板QT版本升级，废弃
        //ui->playButton->setText("Pause");
        break;
    default:
        ui->playButton->setIcon(QIcon(":/image/image/start_press.png"));
        i=1;
        //ui->playButton->setText("Play");
    }
    QByteArray s;
    QString t = "playingState:" + QString::number(i);
    s.append(t);
    //udpSocket->writeDatagram(s,QHostAddress(ipstring),5555);
    udpSend(s);
}

void MainWindow::on_playButton_clicked()
{
    switch(player->state())
    {
    case QMediaPlayer::PlayingState:
        player->pause();
        break;
    default:
        player->play();
    }
}

void MainWindow::is_duration_changed(qint64 duration)
{
    ui->timeSlider->setMaximum(duration);
    ui->durationLable->setText(QTime::fromMSecsSinceStartOfDay(duration).toString("mm:ss"));
    QByteArray s;
    QString t = "duration:" + QString::number(duration);
    s.append(t);
    //udpSocket->writeDatagram(s,QHostAddress(ipstring),5555);
    udpSend(s);
}

void MainWindow::is_position_changed(qint64 position)
{
    ui->positionLable->setText(QTime::fromMSecsSinceStartOfDay(position).toString("mm:ss"));
    if(istimeSliderpressed)
        return;
    ui->timeSlider->setValue(position);
    //qDebug() << QTime::fromMSecsSinceStartOfDay(player->position()).toString("mm:ss.zzz");
    //qDebug() << "AudioBitRate" << player->metaData("AudioBitRate").toInt();
    //qDebug() << "ChannelCount" << player->metaData("ChannelCount").toInt();
    //qDebug() << "SampleRate" << player->metaData("SampleRate").toInt();
    QByteArray s;
    QString t = "position:" + QString::number(position);
    s.append(t);
    udpSend(s);
}

void MainWindow::is_volume_changed(int volume)
{
    ui->volumeSlider->setValue(volume); //设置声音横条值
}

void MainWindow::on_volumeSlider_valueChanged(int value)
{
    player->setVolume(value);   //设置播放器声音
}

void MainWindow::on_timeSlider_sliderReleased() //时间进度条释放
{
    istimeSliderpressed = false;   //时间进度条是否按下，用于控制时间进度条数值。当按下时，数值不由播放器控制
    player->setPosition(ui->timeSlider->value());   //设置播放器播放进度
    lrcviewcontrol();   //调用歌词显示控制槽，切换歌词位置
}

void MainWindow::on_volumeButton_clicked() //静音切换
{
    if(player->isMuted())       //判断播放器是否静音
    {
        ui->volumeButton->setIcon(QIcon(":/image/image/volume_high_press.png"));
        player->setMuted(false);    //设置有音
        }
    else
    {
        ui->volumeButton->setIcon(QIcon(":/image/image/volume_mute_press.png"));
        player->setMuted(true);     //设置静音
    }
}

void MainWindow::on_forButton_clicked() //下一曲
{
    switch(planMode)        //判断播放模式
    {
    case 0:
    case 3:
        playPosition = ++playPosition%playCount;   //播放位置加1，对列表数目取余，防止超过列表最大数，还可以做到当超过列表最大数时从头开始
        break;
    case 1:
        playPosition = qrand()%playCount;   //随机播放取值，对列表最大数取余
        break;
    default:
        break;
    }
    QString filePath = model->index(playPosition,1,playindex.parent()).data().toString();       //取文件路径
    player->setMedia(QUrl::fromLocalFile(filePath));        //设置播放器播放文件路径
    player->play(); //开始播放
}

void MainWindow::on_backButton_clicked()    //上一曲，与下一曲基本相同
{
    switch(planMode)
    {
    case 0:
        if(--playPosition<0)                //如果小于0则加到播放列表总数上。排除意外情况，加起来后的值应当在播放列表数目范围内
            playPosition+=playCount;
        break;
    case 1:
        playPosition = qrand()%playCount;
        break;
    default:
        break;
    }
    QString filePath = model->index(playPosition,1,playindex.parent()).data().toString();
    player->setMedia(QUrl::fromLocalFile(filePath));
    player->play();
}

void MainWindow::is_mediaState_changed()
{
    if(player->mediaStatus() == QMediaPlayer::EndOfMedia)   //播放完毕切歌
        if(!((planMode == 3) && ((playPosition+1) == playCount)))  //如果是顺序播放模式，当播放到列表尾时停止播放
            on_forButton_clicked();     //调用下一曲按钮槽切歌
}

void MainWindow::on_planButton_clicked()
{
    planMode = ++planMode%4;    //播放模式切换
    switch(planMode)            //指定按钮图标
    {
    case 0:
        //ui->planButton->setText("循环");
        ui->planButton->setIcon(QIcon(":/image/image/sequence_press.png")); //列表循环
        break;
    case 1:
        //ui->planButton->setText("随机");
        ui->planButton->setIcon(QIcon(":/image/image/shuffle_press.png"));  //随机播放
        break;
    case 2:
        //ui->planButton->setText("单曲");
        ui->planButton->setIcon(QIcon(":/image/image/repeat_single_press.png"));    //单曲循环
        break;
    case 3:
        ui->planButton->setIcon(QIcon(":/image/image/repeat_all_press.png"));       //顺序播放
    default:
        break;
    }
    QString t = "planMode:" + QString::number(planMode);
    QByteArray s;
    s.append(t);
    //udpSocket->writeDatagram(s,QHostAddress(ipstring),5555);
    udpSend(s);
}

void MainWindow::renderLevel(const QAudioBuffer &buffer)        //振幅显示控件调用
{
    QVector<qreal> levels = Waveform::getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i) {
        ui->waveform->updateWave(levels.at(i));
    }
}

void MainWindow::lrcviewcontrol()
{
    int i = 0;                  //从歌词时间链表头查询
    int p = player->position(); //播放器当前进度
    int next;                   //下一段歌词计时
    foreach(int t,lrcTimeList)
    {
        if(t>p)                 //选定当前歌词行
        {
            next = t-p;         //下一行歌词计时
            break;              //对应时间跳出
        }
        i++;                    //当前歌词行计数
    }
    if(player->state() == QMediaPlayer::PlayingState)           //若处于播放状态执行下
        QTimer::singleShot(next,this,SLOT(lrcviewcontrol()));   //计时切换歌词行
    ui->lrcView->setCurrentIndex(lrcmodel->index(i-1,0));   //设置选中歌词行
    ui->lrcView->verticalScrollBar()->setValue(i-3);        //设置垂直滚动条位置，适当靠前
}

void MainWindow::lrcload()          //加载lrc文件
{
    lrcmodel->clear();              //清理歌词显示控件模型
    lrcTimeList.clear();            //清理歌词时间控制链表
    /*
    if(player->state() != QMediaPlayer::PlayingState)   //如果不是正在播放，则跳出函数
        return;
    */
    QString mediafile = model->index(playPosition,1,playindex.parent()).data().toString();  //从歌单列表模型中获取文件路径
    QString lrcpath = mediafile.mid(0,mediafile.lastIndexOf(".")) + ".lrc";                 //假设歌词文件路径
    QFileInfo fileinfo(lrcpath);        //建立歌词文件信息类
    if(fileinfo.exists())               //判断歌词文件是否存在，存在则执行下面语句
    {
        QFile lrcFile(lrcpath);         //建立歌词文件类
        lrcFile.open(QFile::ReadOnly|QIODevice::Text);  //以只读的文本方式打开
        QString lrc = QString(lrcFile.readAll());       //文件内容导入字符串类
        QStringList lrclist = lrc.split("\n");          //按行拆分建立字符串链表
        int i=0;                    //统计循环次数，确定当前行数
        foreach(QString lrcline,lrclist)    //遍历字符串链表，导入歌词显示控件模型
        {
            int p = lrcline.lastIndexOf("]");       //确定行‘]’所在位置
            QString t = lrcline.mid(1,p-1);         //从1开始到‘]’位置为时间
            if(t.length()<9)
                t+='0';
            QTime time = QTime::fromString(t,"mm:ss.zzz");  //将字符串时间转为时间类
            lrcTimeList << time.msecsSinceStartOfDay();     //将时间类转为整数类，并导入到时间链表

            lrcmodel->appendRow(new QStandardItem(lrcline.mid(p+1)));   //‘]’之后内容导入歌词模型
            lrcmodel->item(i++,0)->setTextAlignment(Qt::AlignCenter);   //设置居中显示
        }


        ui->lrcView->verticalScrollBar()->setMaximum(i);      //设置垂直滚动条最大值，不必太过精确
        //setlrcchanged();        //调用歌词控件控制槽
        this->lrcviewcontrol();
    }
    else        //若无歌词文件，则歌词模型为无歌词
    {
        lrcmodel->appendRow(new QStandardItem("无歌词"));
        lrcmodel->item(0,0)->setTextAlignment(Qt::AlignCenter);     //居中
    }
    //TagLib::FileRef f(mediafile.toStdString().c_str());
    //TagLib::Tag *tag = f.tag();
    //qDebug() << QString::fromWCharArray(tag->title().toCWString());
    /*
    TagLib_File *file;
    TagLib_Tag *tag;
    const TagLib_AudioProperties *properties;
    file = taglib_file_new(mediafile.toStdString().c_str());
    tag = taglib_file_tag(file);
    properties = taglib_file_audioproperties(file);
    qDebug() << taglib_tag_title(tag);
    */
}

void MainWindow::on_action_exit_triggered()
{
    close();
}

void MainWindow::udpReced()
{
    QByteArray byteArray;
    QHostAddress hostAddress;
    quint16 port;
    byteArray.resize(udpSocket->bytesAvailable());
    udpSocket->readDatagram(byteArray.data(),byteArray.size(),&hostAddress,&port);
    //qDebug() << byteArray;
    if(byteArray == "forButton")
        on_forButton_clicked();
    else if(byteArray == "backButton")
        on_backButton_clicked();
    else if(byteArray == "playButton")
        on_playButton_clicked();
    else if(byteArray == "planButton")
        on_planButton_clicked();
    else if(byteArray == "conReady")
    {
        //ipstring = hostAddress.toString();
        ipstring = hostAddress.toString();
        udpSend("playerReady");
        qDebug() << "发送playerReady";
        //udpSocket->writeDatagram("playerReady",ipstring,5555);
    }
    else if(byteArray.mid(0,10) == "timeSlider")
        player->setPosition(byteArray.mid(11).toInt());
    else if(byteArray == "refresh")
    {
        QString title = "title:" + ui->titleLabel->text();
        QByteArray t;
        t.append(title);
        udpSend(t);
        QString duration = "duration:" + QString::number(player->duration());
        QByteArray d;
        d.append(duration);
        udpSend(d);
    }
    qDebug() << "源：" << hostAddress.toString() << port << byteArray;
}

void MainWindow::on_timeSlider_sliderPressed()
{
    istimeSliderpressed = true;   //时间进度条是否按下，用于控制时间进度条数值。当按下时，数值不由播放器控制
}

void MainWindow::udpSend(QByteArray inf)
{
    if(ipstring.isEmpty())
        return;
    udpSocket->writeDatagram(inf,QHostAddress(ipstring),5555);
    qDebug() <<  "发送" << inf << "到" << ipstring;
}

void MainWindow::on_switchButton_clicked()
{
    if(ui->verticalStackedWidget->currentIndex() == 1)
        ui->verticalStackedWidget->setCurrentIndex(0);
    else
        ui->verticalStackedWidget->setCurrentIndex(1);
}

void MainWindow::on_verticalStackedWidget_currentChanged(int arg1)
{
    if(arg1 == 1)
        ui->switchButton->setText("←");
    else
        ui->switchButton->setText("→");
}
