#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileInfo>
#include <QApplication>
#include <QDebug>

// ---------------------------------------------------------
// IMPLEMENTATION: MpvController
// ---------------------------------------------------------

MpvController::MpvController(QWidget *parent)
    : QWidget(parent)
    , mpv(nullptr)
    , statusLabel(nullptr)
    , timeLabel(nullptr)
    , pollTimer(nullptr)
    , isPlayerActive(false)
{
    // NO MORE WA_NativeWindow!
    // This widget is now just a container for buttons, not a video surface.

    // We initialize immediately
    initializeMpv();
}

MpvController::~MpvController() {
    shutdown();
}

void MpvController::initializeMpv() {
    if (isPlayerActive) return;

    mpv = mpv_create();
    if (!mpv) {
        qDebug() << "Failed to create MPV instance!";
        return;
    }

    // --- THE FIX: DO NOT SET "wid" ---
    // By skipping the 'wid' option, MPV will create its own independent window
    // when we load a file. This prevents the macOS deadlock.

    // Optional: Set a title for the floating window so we know which is which
    // Note: MPV might overwrite this with the filename later, but it helps init.
    // mpv_set_option_string(mpv, "title", "MPV Player Window");

    mpv_initialize(mpv);

    // Setup Timer
    if (pollTimer) delete pollTimer;
    pollTimer = new QTimer(this);
    pollTimer->setInterval(500);
    connect(pollTimer, &QTimer::timeout, this, &MpvController::onTimerTick);

    isPlayerActive = true;

    if (statusLabel) statusLabel->setText("Ready (Window will pop up on Load)");
}

void MpvController::shutdown() {
    if (!isPlayerActive) return;

    if (pollTimer) {
        pollTimer->stop();
        delete pollTimer;
        pollTimer = nullptr;
    }

    if (mpv) {
        // Just send quit. Since we don't own the window, we don't need
        // the complex detach logic anymore.
        const char *cmd[] = {"quit", NULL};
        mpv_command(mpv, cmd);

        mpv_terminate_destroy(mpv);
        mpv = nullptr;
    }

    isPlayerActive = false;

    if (statusLabel) statusLabel->setText("Player Closed");
    if (timeLabel) timeLabel->setText("--:--:--");
}

void MpvController::loadVideo(QString path) {
    if (!isPlayerActive) initializeMpv();

    if (pollTimer) pollTimer->stop();

    QByteArray pathBytes = path.toUtf8();
    const char *cmd[] = {"loadfile", pathBytes.data(), NULL};
    mpv_command(mpv, cmd);

    // If you want to force the floating window to resize, you can use:
    // const char *geomCmd[] = {"set", "geometry", "640x480", NULL};
    // mpv_command(mpv, geomCmd);

    if (statusLabel) {
        QFileInfo fileInfo(path);
        statusLabel->setText(fileInfo.fileName());
    }

    if (pollTimer) pollTimer->start();
}

void MpvController::onTimerTick() {
    if (!mpv || !isPlayerActive) return;

    double timePos = 0;
    double duration = 0;

    // Check if the MPV core is still alive (e.g. if user closed the popup window)
    // If mpv_get_property fails heavily, it might mean the window is gone.
    int status = mpv_get_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &timePos);
    mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &duration);

    if (status < 0) {
        // MPV might have been closed by the user clicking "X" on the popup
        // You could handle that here, but for now let's just ignore it.
        return;
    }

    if (timeLabel) {
        QString text = QString("%1 / %2")
        .arg(formatTime(timePos))
            .arg(formatTime(duration));
        timeLabel->setText(text);
    }
}

QString MpvController::formatTime(double totalSeconds) {
    if (totalSeconds < 0) totalSeconds = 0;
    QTime t(0, 0, 0);
    t = t.addSecs(static_cast<int>(totalSeconds));
    return t.toString("HH:mm:ss");
}

void MpvController::setVolume(int value) {
    if (!mpv || !isPlayerActive) return;
    double v = static_cast<double>(value);
    mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &v);
}

void MpvController::togglePause() {
    if (!mpv || !isPlayerActive) return;
    const char *cmd[] = {"cycle", "pause", NULL};
    mpv_command(mpv, cmd);
}

void MpvController::seek(double seconds) {
    if (!mpv || !isPlayerActive) return;
    std::string timeStr = std::to_string(seconds);
    const char *cmd[] = {"seek", timeStr.c_str(), "relative", NULL};
    mpv_command(mpv, cmd);
    onTimerTick();
}

// ---------------------------------------------------------
// IMPLEMENTATION: MainWindow
// ---------------------------------------------------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Make the Main Window smaller since it doesn't hold videos anymore
    this->resize(600, 400);

    QWidget *centralContainer = new QWidget(this);
    setCentralWidget(centralContainer);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralContainer);
    QHBoxLayout *videoArea = new QHBoxLayout();

    auto createPlayerColumn = [this](MpvController*& playerRef, QString title) -> QVBoxLayout* {
        QVBoxLayout *col = new QVBoxLayout();

        QLabel *header = new QLabel(title);
        header->setStyleSheet("font-weight: bold; font-size: 14px;");
        header->setAlignment(Qt::AlignCenter);
        col->addWidget(header);

        // We no longer add playerRef as a giant widget.
        // We just create it in memory to manage the logic.
        playerRef = new MpvController(this); // Parent is Main Window now
        playerRef->hide(); // Hide the empty container widget, we just need logic

        // Info Box (Visible on Dashboard)
        QFrame *infoFrame = new QFrame();
        infoFrame->setStyleSheet("background-color: #f0f0f0; border-radius: 5px;");
        QVBoxLayout *infoLayout = new QVBoxLayout(infoFrame);

        QLabel *fileLabel = new QLabel("Ready");
        fileLabel->setStyleSheet("color: #333; font-weight: bold;");
        fileLabel->setAlignment(Qt::AlignCenter);

        QLabel *timeLabel = new QLabel("--:--:-- / --:--:--");
        timeLabel->setStyleSheet("color: #0055aa; font-family: monospace; font-size: 14px;");
        timeLabel->setAlignment(Qt::AlignCenter);

        infoLayout->addWidget(fileLabel);
        infoLayout->addWidget(timeLabel);
        col->addWidget(infoFrame);

        playerRef->statusLabel = fileLabel;
        playerRef->timeLabel = timeLabel;

        // Seek Controls
        QHBoxLayout *seekRow = new QHBoxLayout();
        QPushButton *btnBack1m  = new QPushButton("<< 1m");
        QPushButton *btnBack10s = new QPushButton("< 10s");
        QPushButton *btnFwd10s  = new QPushButton("10s >");
        QPushButton *btnFwd1m   = new QPushButton("1m >>");
        seekRow->addWidget(btnBack1m);
        seekRow->addWidget(btnBack10s);
        seekRow->addWidget(btnFwd10s);
        seekRow->addWidget(btnFwd1m);
        col->addLayout(seekRow);

        // Main Controls
        QHBoxLayout *controls = new QHBoxLayout();
        QPushButton *btnLoad  = new QPushButton("Load");
        QPushButton *btnClose = new QPushButton("Close");
        QPushButton *btnPlay  = new QPushButton("Play"); // Compact Text

        btnClose->setStyleSheet("color: white; background-color: #d9534f; border: none; padding: 5px;");
        btnLoad->setStyleSheet("padding: 5px;");

        controls->addWidget(btnLoad);
        controls->addWidget(btnClose);
        controls->addWidget(btnPlay);
        col->addLayout(controls);

        // Volume
        QHBoxLayout *volRow = new QHBoxLayout();
        QSlider *volSlider = new QSlider(Qt::Horizontal);
        volSlider->setRange(0, 100);
        volSlider->setValue(50);
        volRow->addWidget(new QLabel("Vol:"));
        volRow->addWidget(volSlider);
        col->addLayout(volRow);

        // Wiring
        connect(btnBack1m,  &QPushButton::clicked, [=]() { playerRef->seek(-60.0); });
        connect(btnBack10s, &QPushButton::clicked, [=]() { playerRef->seek(-10.0); });
        connect(btnFwd10s,  &QPushButton::clicked, [=]() { playerRef->seek(10.0); });
        connect(btnFwd1m,   &QPushButton::clicked, [=]() { playerRef->seek(60.0); });

        connect(btnClose, &QPushButton::clicked, [=]() { playerRef->shutdown(); });

        connect(btnLoad, &QPushButton::clicked, this, [=]() {
            QString fileName = QFileDialog::getOpenFileName(this, "Select Video", "", "Videos (*.mp4 *.mkv *.avi *.mov)");
            if (!fileName.isEmpty()) {
                playerRef->loadVideo(fileName);
            }
        });

        connect(btnPlay, &QPushButton::clicked, [=]() { playerRef->togglePause(); });
        connect(volSlider, &QSlider::valueChanged, [=](int value) { playerRef->setVolume(value); });

        return col;
    };

    QVBoxLayout *leftCol = createPlayerColumn(player1, "Left Player");
    videoArea->addLayout(leftCol);

    // Add a vertical line separator
    QFrame *vLine = new QFrame();
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFrameShadow(QFrame::Sunken);
    videoArea->addWidget(vLine);

    QVBoxLayout *rightCol = createPlayerColumn(player2, "Right Player");
    videoArea->addLayout(rightCol);

    mainLayout->addLayout(videoArea);

    // Global Controls
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    QHBoxLayout *globalSeek = new QHBoxLayout();
    globalSeek->addWidget(new QLabel("Global Seek:"));
    QPushButton *gBack1m  = new QPushButton("<< 1m");
    QPushButton *gBack10s = new QPushButton("< 10s");
    QPushButton *gFwd10s  = new QPushButton("10s >");
    QPushButton *gFwd1m   = new QPushButton("1m >>");

    globalSeek->addWidget(gBack1m);
    globalSeek->addWidget(gBack10s);
    globalSeek->addWidget(gFwd10s);
    globalSeek->addWidget(gFwd1m);
    mainLayout->addLayout(globalSeek);

    QHBoxLayout *globalControls = new QHBoxLayout();
    QPushButton *btnGlobalPause = new QPushButton("Global Pause");
    QPushButton *btnGlobalPlay  = new QPushButton("Global Play");

    // Styling Global Buttons
    btnGlobalPause->setMinimumHeight(40);
    btnGlobalPlay->setMinimumHeight(40);

    globalControls->addWidget(btnGlobalPause);
    globalControls->addWidget(btnGlobalPlay);
    mainLayout->addLayout(globalControls);

    // Global Logic
    connect(gBack1m,  &QPushButton::clicked, [=]() { player1->seek(-60); player2->seek(-60); });
    connect(gBack10s, &QPushButton::clicked, [=]() { player1->seek(-10); player2->seek(-10); });
    connect(gFwd10s,  &QPushButton::clicked, [=]() { player1->seek(10);  player2->seek(10); });
    connect(gFwd1m,   &QPushButton::clicked, [=]() { player1->seek(60);  player2->seek(60); });

    connect(btnGlobalPause, &QPushButton::clicked, this, [=]() {
        if(player1->isPlayerActive) { int f=1; mpv_set_property(player1->mpv, "pause", MPV_FORMAT_FLAG, &f); }
        if(player2->isPlayerActive) { int f=1; mpv_set_property(player2->mpv, "pause", MPV_FORMAT_FLAG, &f); }
    });

    connect(btnGlobalPlay, &QPushButton::clicked, this, [=]() {
        if(player1->isPlayerActive) { int f=0; mpv_set_property(player1->mpv, "pause", MPV_FORMAT_FLAG, &f); }
        if(player2->isPlayerActive) { int f=0; mpv_set_property(player2->mpv, "pause", MPV_FORMAT_FLAG, &f); }
    });
}

MainWindow::~MainWindow()
{
    if (player1) player1->shutdown();
    if (player2) player2->shutdown();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (player1) player1->shutdown();
    if (player2) player2->shutdown();
    event->accept();
}
