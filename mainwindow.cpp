#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileInfo>
#include <QApplication>
#include <QDebug>

// ---------------------------------------------------------
// IMPLEMENTATION: MpvWidget
// ---------------------------------------------------------

MpvWidget::MpvWidget(QWidget *parent) : QWidget(parent), mpv(nullptr), statusLabel(nullptr), timeLabel(nullptr) {
    // 1. Setup Window Attributes for Embedding
    // WA_NativeWindow is crucial. We removed the others to be safer on macOS.
    setAttribute(Qt::WA_NativeWindow);
    setStyleSheet("background-color: black;");

    // 2. Create MPV
    mpv = mpv_create();
    if (!mpv) {
        qDebug() << "Failed to create MPV instance!";
        return;
    }

    // 3. Configure MPV (Window Embedding)
    int64_t wid = this->winId();
    mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);

    // Standard settings
    mpv_set_option_string(mpv, "keep-open", "yes");
    mpv_set_option_string(mpv, "input-default-bindings", "no");
    mpv_set_option_string(mpv, "input-vo-keyboard", "no");

    // Disable terminal output which can cause issues
    mpv_set_option_string(mpv, "terminal", "no");

    mpv_initialize(mpv);

    // 4. Setup Timer (BUT DO NOT START IT YET)
    // Starting it now causes the "Freeze on Load" because it polls
    // an empty player while the player is trying to initialize.
    pollTimer = new QTimer(this);
    pollTimer->setInterval(500);
    connect(pollTimer, &QTimer::timeout, this, &MpvWidget::onTimerTick);
}

MpvWidget::~MpvWidget() {
    shutdown();
}

// --- SAFE SHUTDOWN (Fixes the App Close Hang) ---
void MpvWidget::shutdown() {
    // 1. Stop the Timer immediately so it stops asking for time
    if (pollTimer) {
        pollTimer->stop();
        pollTimer->deleteLater();
        pollTimer = nullptr;
    }

    if (mpv) {
        // 2. Pause playback immediately
        int flag = 1;
        mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &flag);

        // 3. Stop playback - unloads the file, releases video output resources
        const char *stopCmd[] = {"stop", NULL};
        mpv_command(mpv, stopCmd);

        // 4. Send async quit command - tells MPV to shut down its threads
        const char *quitCmd[] = {"quit", NULL};
        mpv_command_async(mpv, 0, quitCmd);

        // 5. Use mpv_destroy instead of mpv_terminate_destroy
        // mpv_terminate_destroy waits for MPV to finish, which can deadlock
        // mpv_destroy just releases our handle; MPV cleans up in background
        mpv_destroy(mpv);
        mpv = nullptr;
    }
}

// --- SAFE LOAD (Fixes the Load Hang) ---
void MpvWidget::loadVideo(QString path) {
    if (!mpv) return;

    // 1. PAUSE THE TIMER
    // We must stop polling while we perform the heavy "load" operation.
    pollTimer->stop();

    // 2. Load the file
    QByteArray pathBytes = path.toUtf8();
    const char *cmd[] = {"loadfile", pathBytes.data(), NULL};
    mpv_command(mpv, cmd); // This blocks until the file is probed

    // 3. Update Filename
    if (statusLabel) {
        QFileInfo fileInfo(path);
        statusLabel->setText(fileInfo.fileName());
    }

    // 4. RESUME THE TIMER
    // Now that the file is loaded, it is safe to ask "What time is it?"
    pollTimer->start();
}

void MpvWidget::closeVideo() {
    if (!mpv) return;

    // 1. Stop the timer so we don't poll during cleanup
    pollTimer->stop();

    // 2. Stop playback and unload the file
    const char *cmd[] = {"stop", NULL};
    mpv_command(mpv, cmd);

    // 3. Reset the UI labels
    if (statusLabel) {
        statusLabel->setText("No file loaded");
    }
    if (timeLabel) {
        timeLabel->setText("--:--:-- / --:--:--");
    }
}

void MpvWidget::onTimerTick() {
    if (!mpv) return;

    double timePos = 0;
    double duration = 0;

    // We use MPV_FORMAT_DOUBLE to get precise seconds
    mpv_get_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &timePos);
    mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &duration);

    if (timeLabel) {
        QString text = QString("%1 / %2")
        .arg(formatTime(timePos))
            .arg(formatTime(duration));
        timeLabel->setText(text);
    }
}

QString MpvWidget::formatTime(double totalSeconds) {
    if (totalSeconds < 0) totalSeconds = 0;
    QTime t(0, 0, 0);
    t = t.addSecs(static_cast<int>(totalSeconds));
    return t.toString("HH:mm:ss");
}

void MpvWidget::setVolume(int value) {
    if (!mpv) return;
    double v = static_cast<double>(value);
    mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &v);
}

void MpvWidget::togglePause() {
    if (!mpv) return;
    const char *cmd[] = {"cycle", "pause", NULL};
    mpv_command(mpv, cmd);
}

void MpvWidget::seek(double seconds) {
    if (!mpv) return;
    std::string timeStr = std::to_string(seconds);
    const char *cmd[] = {"seek", timeStr.c_str(), "relative", NULL};
    mpv_command(mpv, cmd);

    // Update timestamp immediately for better UI feel
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

    QWidget *centralContainer = new QWidget(this);
    setCentralWidget(centralContainer);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralContainer);
    mainLayout->setSpacing(4);  // Reduce vertical spacing between elements

    QHBoxLayout *videoArea = new QHBoxLayout();
    videoArea->setSpacing(0);  // No spacing - we'll use a divider

    auto createPlayerColumn = [this](MpvWidget*& playerRef, QString title) -> QVBoxLayout* {
        QVBoxLayout *col = new QVBoxLayout();
        col->setSpacing(4);  // Tighter spacing within each player column

        QLabel *header = new QLabel(title);
        header->setStyleSheet("font-weight: bold; font-size: 14px;");
        col->addWidget(header);

        playerRef = new MpvWidget();
        playerRef->setMinimumSize(400, 20);
        col->addWidget(playerRef, 1);

        // Info Row
        QHBoxLayout *infoRow = new QHBoxLayout();
        QLabel *fileLabel = new QLabel("No file loaded");
        fileLabel->setStyleSheet("color: #333; font-weight: bold;");

        QLabel *timeLabel = new QLabel("--:--:-- / --:--:--");
        timeLabel->setStyleSheet("color: #0055aa; font-family: monospace;");
        timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        timeLabel->setFixedWidth(130);  // Fixed width prevents layout shift when time changes

        infoRow->addWidget(fileLabel);
        infoRow->addStretch();
        infoRow->addWidget(timeLabel);
        col->addLayout(infoRow);

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
        QPushButton *btnLoad = new QPushButton("Load");
        QPushButton *btnClose = new QPushButton("Close");
        QPushButton *btnPlay = new QPushButton("Play/Pause");
        QSlider *volSlider = new QSlider(Qt::Horizontal);
        volSlider->setRange(0, 100);
        volSlider->setValue(50);

        btnClose->setStyleSheet("color: #aa0000;"); // Red text to indicate "stop" action

        controls->addWidget(btnLoad);
        controls->addWidget(btnClose);
        controls->addWidget(btnPlay);
        controls->addWidget(new QLabel("Vol:"));
        controls->addWidget(volSlider);
        col->addLayout(controls);

        // Wiring
        connect(btnBack1m,  &QPushButton::clicked, [=]() { playerRef->seek(-60.0); });
        connect(btnBack10s, &QPushButton::clicked, [=]() { playerRef->seek(-10.0); });
        connect(btnFwd10s,  &QPushButton::clicked, [=]() { playerRef->seek(10.0); });
        connect(btnFwd1m,   &QPushButton::clicked, [=]() { playerRef->seek(60.0); });

        connect(btnLoad, &QPushButton::clicked, this, [=]() {
            QString fileName = QFileDialog::getOpenFileName(this, "Select Video", "", "Videos (*.mp4 *.mkv *.avi *.mov)");
            if (!fileName.isEmpty()) {
                playerRef->loadVideo(fileName);
            }
        });

        connect(btnClose, &QPushButton::clicked, [=]() { playerRef->closeVideo(); });
        connect(btnPlay, &QPushButton::clicked, [=]() { playerRef->togglePause(); });
        connect(volSlider, &QSlider::valueChanged, [=](int value) { playerRef->setVolume(value); });

        return col;
    };

    // Build UI - Player 1
    QVBoxLayout *leftCol = createPlayerColumn(player1, "Player 1 (Left)");
    videoArea->addLayout(leftCol);

    // Vertical divider between players
    QFrame *vLine = new QFrame();
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFrameShadow(QFrame::Sunken);
    videoArea->addWidget(vLine);

    // Player 2
    QVBoxLayout *rightCol = createPlayerColumn(player2, "Player 2 (Right)");
    videoArea->addLayout(rightCol);

    mainLayout->addLayout(videoArea);

    // Global Controls
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    QHBoxLayout *globalSeek = new QHBoxLayout();
    QPushButton *gBack1m  = new QPushButton("Global << 1m");
    QPushButton *gBack10s = new QPushButton("Global < 10s");
    QPushButton *gFwd10s  = new QPushButton("Global 10s >");
    QPushButton *gFwd1m   = new QPushButton("Global 1m >>");

    globalSeek->addWidget(gBack1m);
    globalSeek->addWidget(gBack10s);
    globalSeek->addWidget(gFwd10s);
    globalSeek->addWidget(gFwd1m);
    mainLayout->addLayout(globalSeek);

    QHBoxLayout *globalControls = new QHBoxLayout();
    QPushButton *btnGlobalPause = new QPushButton("Global Pause");
    QPushButton *btnGlobalPlay  = new QPushButton("Global Play");
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
        int flag = 1;
        if (player1->mpv) mpv_set_property(player1->mpv, "pause", MPV_FORMAT_FLAG, &flag);
        if (player2->mpv) mpv_set_property(player2->mpv, "pause", MPV_FORMAT_FLAG, &flag);
    });

    connect(btnGlobalPlay, &QPushButton::clicked, this, [=]() {
        int flag = 0;
        if (player1->mpv) mpv_set_property(player1->mpv, "pause", MPV_FORMAT_FLAG, &flag);
        if (player2->mpv) mpv_set_property(player2->mpv, "pause", MPV_FORMAT_FLAG, &flag);
    });
}

MainWindow::~MainWindow()
{
    // Shutdown is idempotent (checks if mpv is null), but we should
    // only call it here if closeEvent wasn't already called
    if (player1) player1->shutdown();
    if (player2) player2->shutdown();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // First, close any loaded videos (stop playback, unload files)
    // This releases video resources before we destroy MPV
    if (player1) player1->closeVideo();
    if (player2) player2->closeVideo();

    // Process events to let MPV handle the stop commands
    QApplication::processEvents();

    // Now shut down the MPV instances
    if (player1) player1->shutdown();
    if (player2) player2->shutdown();

    event->accept();
}
