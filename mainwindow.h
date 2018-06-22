#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QMediaPlayer>
#include <QAudioProbe>
#include <QStringListModel>
#include <QTimer>
#include <QScrollBar>
#include <QUdpSocket>

#include "waveform.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_treeView_clicked(const QModelIndex &index);

    void is_mediaMeta_changed(bool);

    void is_playerState_changed();

    void is_mediaState_changed();

    void on_playButton_clicked();

    void is_duration_changed(qint64 duration);

    void is_position_changed(qint64 position);

    void is_volume_changed(int volume);

    void on_volumeSlider_valueChanged(int value);

    void on_timeSlider_sliderReleased();

    void on_volumeButton_clicked();

    void on_forButton_clicked();

    void on_backButton_clicked();

    void on_planButton_clicked();

    void renderLevel(const QAudioBuffer &buffer);

    void lrcviewcontrol();

    void lrcload();

    void on_action_exit_triggered();

    void udpReced();

    void udpSend(QByteArray inf);

    void on_timeSlider_sliderPressed();

    void on_switchButton_clicked();

    void on_verticalStackedWidget_currentChanged(int arg1);

private:
    Ui::MainWindow *ui;

    QStandardItemModel *model;

    bool istimeSliderpressed;

    QAudioProbe *audioProbe;

    QModelIndex playindex;

    int playCount;

    QMediaPlayer *player;

    qint64 playPosition;

    int planMode;

    QStandardItemModel *lrcmodel;

    QScrollBar *scrollBar;

    QList<qint64> lrcTimeList;

    QString playPath;

    QUdpSocket *udpSocket;

    QString ipstring;

    int port;

    /*
    QStringListModel *lrcListModel;

    QList<qint64> lrcTime;
    */
};

#endif // MAINWINDOW_H
