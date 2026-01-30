#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QSlider>
#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QTimer>
#include <QTime>
#include <mpv/client.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MpvWidget : public QWidget {
    Q_OBJECT
public:
    mpv_handle *mpv;
    QLabel *statusLabel;
    QLabel *timeLabel;
    QTimer *pollTimer;

    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget();

    void loadVideo(QString path);
    void closeVideo();  // Unload video without destroying MPV
    void setVolume(int value);
    void togglePause();
    void seek(double seconds);
    void shutdown(); // The most important function

    QString formatTime(double time);

public slots:
    void onTimerTick();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    MpvWidget *player1;
    MpvWidget *player2;
};

#endif // MAINWINDOW_H
