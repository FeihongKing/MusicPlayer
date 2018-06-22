#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTime>
#include <QNetworkInterface>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    isTimeSliderPress = false;
    planMode = 0;

    //ipstring = "192.168.2.1";

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::AnyIPv4,5555);
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(udpReced()));
    
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach(QHostAddress address,list)
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol && address!=QHostAddress::Null && address!=QHostAddress::LocalHost)
            myAddress = address;
    }
    //QByteArray byteArray;
    //byteArray.append("conStart");
    //byteArray.append(myAddress.toString());
    udpSocket->writeDatagram("conReady",QHostAddress::Broadcast,5555);      //
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::udpReced()
{
    QByteArray byteArray;
    QHostAddress hostAddress;
    quint16 port;
    byteArray.resize(udpSocket->bytesAvailable());
    udpSocket->readDatagram(byteArray.data(),byteArray.size(),&hostAddress,&port);
    //ui->label->setText(byteArray);
    //ui->titleLable->setText(byteArray);
    int p = byteArray.indexOf(":");
    QString l = byteArray.mid(0,p);
    if(l == "title")
        ui->titleLable->setText(byteArray.mid(p+1));
    else if(l == "duration")
    {
        qint32 time = byteArray.mid(p+1).toInt();
        ui->timeSlider->setMaximum(time);
        ui->durationLable->setText(QTime::fromMSecsSinceStartOfDay(time).toString("mm:ss"));
    }
    else if(l == "position")
    {
        qint32 time = byteArray.mid(p+1).toInt();
        if(!isTimeSliderPress)
            ui->timeSlider->setValue(time);
        ui->positionLable->setText(QTime::fromMSecsSinceStartOfDay(time).toString("mm:ss"));
    }
    else if(l == "planMode")
    {
        switch(byteArray.mid(p+1).toInt())            //指定按钮图标
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
    }
    else if(l == "playingState")
    {

        if(byteArray.mid(p+1).toInt() == 0)
            ui->playButton->setIcon(QIcon(":/image/image/pause_press.png"));
        else
            ui->playButton->setIcon(QIcon(":/image/image/start_press.png"));
    }
    else if(byteArray == "playerReady")
        ipstring = hostAddress.toString();
    ui->infoLabel->setText("IP:" + ipstring);
}

void MainWindow::on_forButton_clicked()
{
    //udpSocket->writeDatagram("forButton",QHostAddress(ipstring),5555);
    udpSend("forButton");
}

void MainWindow::on_playButton_clicked()
{
    //udpSocket->writeDatagram("playButton",QHostAddress(ipstring),5555);
    udpSend("playButton");
}

void MainWindow::on_backButton_clicked()
{
    //udpSocket->writeDatagram("backButton",QHostAddress(ipstring),5555);
    udpSend("backButton");
}

void MainWindow::on_timeSlider_sliderReleased()
{
    isTimeSliderPress = false;
    QString t = "timeSlider:" + QString::number(ui->timeSlider->value());
    QByteArray s;
    s.append(t);
    //udpSocket->writeDatagram(s,QHostAddress(ipstring),5555);
    udpSend(s);
}


void MainWindow::on_timeSlider_sliderPressed()
{
    isTimeSliderPress = true;
}

void MainWindow::on_planButton_clicked()
{
    //udpSocket->writeDatagram("planButton",QHostAddress(ipstring),5555);
    udpSend("planButton");
}

void MainWindow::udpSend(QByteArray inf)
{
    if(ipstring.isEmpty())
        udpSocket->writeDatagram("conReady",QHostAddress::Broadcast,5555);
    if(ipstring.isEmpty())
        return;
    udpSocket->writeDatagram(inf,QHostAddress(ipstring),5555);
}

void MainWindow::on_toolButton_clicked()
{
    udpSend("refresh");
}
