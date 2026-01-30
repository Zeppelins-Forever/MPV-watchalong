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

class MpvController : public QWidget { // Renamed from MpvWidget to MpvController
    Q_OBJECT
public:
    mpv_handle *mpv;
    QLabel *statusLabel;
    QLabel *timeLabel;
    QTimer *pollTimer;
    bool isPlayerActive;

    explicit MpvController(QWidget *parent = nullptr);
    ~MpvController();

    void initializeMpv();
    void loadVideo(QString path);
    void setVolume(int value);
    void togglePause();
    void seek(double seconds);
    void shutdown();

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
    MpvController *player1;
    MpvController *player2;
};

#endif // MAINWINDOW_H
