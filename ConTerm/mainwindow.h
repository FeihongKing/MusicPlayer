#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>

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
    void udpReced();

    void on_forButton_clicked();

    void on_playButton_clicked();

    void on_backButton_clicked();

    void on_timeSlider_sliderReleased();

    void on_timeSlider_sliderPressed();

    void on_planButton_clicked();

    void udpSend(QByteArray inf);

    void on_toolButton_clicked();

private:
    Ui::MainWindow *ui;

    int planMode;

    bool isTimeSliderPress;

    QUdpSocket *udpSocket;
    
    QHostAddress myAddress;
    
    QHostAddress otAddress;

    QString ipstring;
};

#endif // MAINWINDOW_H
