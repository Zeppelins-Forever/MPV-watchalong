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

MpvWidget::MpvWidget(QWidget *parent) : QWidget(parent), mpv(nullptr), statusLabel(nullptr), timeLabel(nullptr), subtitleCombo(nullptr), audioCombo(nullptr) {
    // 1. Setup Widget
    setStyleSheet("background-color: black;");

    // 2. Create MPV
    mpv = mpv_create();
    if (!mpv) {
        qDebug() << "Failed to create MPV instance!";
        return;
    }

    // 3. Configure MPV (Separate Window)
    // We intentionally do NOT set "wid" so MPV opens in its own window,
    // allowing users to position and resize video windows independently.
    
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
    
    // 5. Refresh subtitle and audio tracks after a short delay to let MPV parse the file
    QTimer::singleShot(500, this, &MpvWidget::refreshSubtitleTracks);
    QTimer::singleShot(500, this, &MpvWidget::refreshAudioTracks);
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
    
    // 4. Reset subtitle combo
    if (subtitleCombo) {
        subtitleCombo->blockSignals(true);
        subtitleCombo->clear();
        subtitleCombo->addItem("Off", 0);
        subtitleCombo->blockSignals(false);
    }
    
    // 5. Reset audio combo
    if (audioCombo) {
        audioCombo->blockSignals(true);
        audioCombo->clear();
        audioCombo->blockSignals(false);
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

void MpvWidget::refreshSubtitleTracks() {
    if (!mpv || !subtitleCombo) return;
    
    // Block signals while we update the combo box
    subtitleCombo->blockSignals(true);
    subtitleCombo->clear();
    subtitleCombo->addItem("Off", 0);  // sid=0 means no subtitles
    
    // Get the track list from MPV
    mpv_node trackList;
    if (mpv_get_property(mpv, "track-list", MPV_FORMAT_NODE, &trackList) >= 0) {
        if (trackList.format == MPV_FORMAT_NODE_ARRAY) {
            for (int i = 0; i < trackList.u.list->num; i++) {
                mpv_node *track = &trackList.u.list->values[i];
                if (track->format != MPV_FORMAT_NODE_MAP) continue;
                
                QString type;
                int64_t id = 0;
                QString title;
                QString lang;
                bool isExternal = false;
                
                // Parse track properties
                for (int j = 0; j < track->u.list->num; j++) {
                    const char *key = track->u.list->keys[j];
                    mpv_node *val = &track->u.list->values[j];
                    
                    if (strcmp(key, "type") == 0 && val->format == MPV_FORMAT_STRING) {
                        type = QString::fromUtf8(val->u.string);
                    } else if (strcmp(key, "id") == 0 && val->format == MPV_FORMAT_INT64) {
                        id = val->u.int64;
                    } else if (strcmp(key, "title") == 0 && val->format == MPV_FORMAT_STRING) {
                        title = QString::fromUtf8(val->u.string);
                    } else if (strcmp(key, "lang") == 0 && val->format == MPV_FORMAT_STRING) {
                        lang = QString::fromUtf8(val->u.string);
                    } else if (strcmp(key, "external") == 0 && val->format == MPV_FORMAT_FLAG) {
                        isExternal = val->u.flag;
                    }
                }
                
                // Only add subtitle tracks
                if (type == "sub") {
                    QString label = QString("#%1").arg(id);
                    if (!lang.isEmpty()) label += " [" + lang + "]";
                    if (!title.isEmpty()) label += " " + title;
                    if (isExternal) label += " (external)";
                    
                    subtitleCombo->addItem(label, static_cast<int>(id));
                }
            }
        }
        mpv_free_node_contents(&trackList);
    }
    
    // Select current track
    int64_t currentSid = 0;
    mpv_get_property(mpv, "sid", MPV_FORMAT_INT64, &currentSid);
    for (int i = 0; i < subtitleCombo->count(); i++) {
        if (subtitleCombo->itemData(i).toInt() == currentSid) {
            subtitleCombo->setCurrentIndex(i);
            break;
        }
    }
    
    subtitleCombo->blockSignals(false);
}

void MpvWidget::setSubtitleTrack(int index) {
    if (!mpv || !subtitleCombo) return;
    
    int sid = subtitleCombo->itemData(index).toInt();
    int64_t sidValue = sid;
    mpv_set_property(mpv, "sid", MPV_FORMAT_INT64, &sidValue);
}

void MpvWidget::loadExternalSubtitles(QString path) {
    if (!mpv) return;
    
    QByteArray pathBytes = path.toUtf8();
    const char *cmd[] = {"sub-add", pathBytes.data(), "auto", NULL};
    mpv_command(mpv, cmd);
    
    // Refresh the track list to show the new subtitle
    refreshSubtitleTracks();
    
    // Select the newly added track (it should be the last one)
    if (subtitleCombo && subtitleCombo->count() > 0) {
        subtitleCombo->setCurrentIndex(subtitleCombo->count() - 1);
    }
}

void MpvWidget::refreshAudioTracks() {
    if (!mpv || !audioCombo) return;
    
    // Block signals while we update the combo box
    audioCombo->blockSignals(true);
    audioCombo->clear();
    
    // Get the track list from MPV
    mpv_node trackList;
    if (mpv_get_property(mpv, "track-list", MPV_FORMAT_NODE, &trackList) >= 0) {
        if (trackList.format == MPV_FORMAT_NODE_ARRAY) {
            for (int i = 0; i < trackList.u.list->num; i++) {
                mpv_node *track = &trackList.u.list->values[i];
                if (track->format != MPV_FORMAT_NODE_MAP) continue;
                
                QString type;
                int64_t id = 0;
                QString title;
                QString lang;
                int64_t channels = 0;
                
                // Parse track properties
                for (int j = 0; j < track->u.list->num; j++) {
                    const char *key = track->u.list->keys[j];
                    mpv_node *val = &track->u.list->values[j];
                    
                    if (strcmp(key, "type") == 0 && val->format == MPV_FORMAT_STRING) {
                        type = QString::fromUtf8(val->u.string);
                    } else if (strcmp(key, "id") == 0 && val->format == MPV_FORMAT_INT64) {
                        id = val->u.int64;
                    } else if (strcmp(key, "title") == 0 && val->format == MPV_FORMAT_STRING) {
                        title = QString::fromUtf8(val->u.string);
                    } else if (strcmp(key, "lang") == 0 && val->format == MPV_FORMAT_STRING) {
                        lang = QString::fromUtf8(val->u.string);
                    } else if (strcmp(key, "demux-channel-count") == 0 && val->format == MPV_FORMAT_INT64) {
                        channels = val->u.int64;
                    }
                }
                
                // Only add audio tracks
                if (type == "audio") {
                    QString label = QString("#%1").arg(id);
                    if (!lang.isEmpty()) label += " [" + lang + "]";
                    if (!title.isEmpty()) label += " " + title;
                    if (channels > 0) {
                        if (channels == 1) label += " (Mono)";
                        else if (channels == 2) label += " (Stereo)";
                        else if (channels == 6) label += " (5.1)";
                        else if (channels == 8) label += " (7.1)";
                        else label += QString(" (%1ch)").arg(channels);
                    }
                    
                    audioCombo->addItem(label, static_cast<int>(id));
                }
            }
        }
        mpv_free_node_contents(&trackList);
    }
    
    // Select current track
    int64_t currentAid = 0;
    mpv_get_property(mpv, "aid", MPV_FORMAT_INT64, &currentAid);
    for (int i = 0; i < audioCombo->count(); i++) {
        if (audioCombo->itemData(i).toInt() == currentAid) {
            audioCombo->setCurrentIndex(i);
            break;
        }
    }
    
    audioCombo->blockSignals(false);
}

void MpvWidget::setAudioTrack(int index) {
    if (!mpv || !audioCombo) return;
    
    int aid = audioCombo->itemData(index).toInt();
    int64_t aidValue = aid;
    mpv_set_property(mpv, "aid", MPV_FORMAT_INT64, &aidValue);
}

// ---------------------------------------------------------
// IMPLEMENTATION: MainWindow
// ---------------------------------------------------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Set default window size
    resize(900, 450);

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

        // MpvWidget is not displayed - video plays in its own window
        // We just need it to exist to manage the MPV instance
        playerRef = new MpvWidget();
        playerRef->setVisible(false);

        // Info Row
        QHBoxLayout *infoRow = new QHBoxLayout();
        QLabel *fileLabel = new QLabel("No file loaded");
        fileLabel->setStyleSheet("color: #333; font-weight: bold;");
        fileLabel->setWordWrap(true);
        fileLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

        QLabel *timeLabel = new QLabel("--:--:-- / --:--:--");
        timeLabel->setStyleSheet("color: #0055aa; font-family: monospace;");
        timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        timeLabel->setFixedWidth(130);  // Fixed width prevents layout shift when time changes

        infoRow->addWidget(fileLabel, 1);  // Give fileLabel stretch factor
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
        
        // Subtitle Controls
        QHBoxLayout *subRow = new QHBoxLayout();
        QLabel *subLabel = new QLabel("Subs:");
        QComboBox *subCombo = new QComboBox();
        subCombo->addItem("Off", 0);
        subCombo->setMinimumWidth(120);
        QPushButton *btnLoadSub = new QPushButton("Load Sub...");
        
        subRow->addWidget(subLabel);
        subRow->addWidget(subCombo, 1);
        subRow->addWidget(btnLoadSub);
        col->addLayout(subRow);
        
        playerRef->subtitleCombo = subCombo;
        
        // Audio Controls
        QHBoxLayout *audioRow = new QHBoxLayout();
        QLabel *audioLabel = new QLabel("Audio:");
        QComboBox *audioCombo = new QComboBox();
        audioCombo->setMinimumWidth(120);
        
        audioRow->addWidget(audioLabel);
        audioRow->addWidget(audioCombo, 1);
        col->addLayout(audioRow);
        
        playerRef->audioCombo = audioCombo;

        // Wiring
        connect(btnBack1m,  &QPushButton::clicked, [=]() { playerRef->seek(-60.0); });
        connect(btnBack10s, &QPushButton::clicked, [=]() { playerRef->seek(-10.0); });
        connect(btnFwd10s,  &QPushButton::clicked, [=]() { playerRef->seek(10.0); });
        connect(btnFwd1m,   &QPushButton::clicked, [=]() { playerRef->seek(60.0); });

        connect(btnLoad, &QPushButton::clicked, this, [=]() {
            QString fileName = QFileDialog::getOpenFileName(this, "Select Video", "", "Videos (*.mp4 *.mkv *.avi *.mov)");
            if (!fileName.isEmpty()) {
                // Process events to let macOS finalize any permission grants
                QApplication::processEvents();
                
                // Small delay to ensure permission is fully granted
                QTimer::singleShot(100, [=]() {
                    playerRef->loadVideo(fileName);
                });
            }
        });

        connect(btnClose, &QPushButton::clicked, [=]() { playerRef->closeVideo(); });
        connect(btnPlay, &QPushButton::clicked, [=]() { playerRef->togglePause(); });
        connect(volSlider, &QSlider::valueChanged, [=](int value) { playerRef->setVolume(value); });
        
        // Subtitle wiring
        connect(subCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                [=](int index) { playerRef->setSubtitleTrack(index); });
        
        connect(btnLoadSub, &QPushButton::clicked, this, [=]() {
            QString subFile = QFileDialog::getOpenFileName(this, "Select Subtitle File", "", 
                "Subtitles (*.srt *.ass *.ssa *.sub *.vtt);;All Files (*)");
            if (!subFile.isEmpty()) {
                // Process events to let macOS finalize any permission grants
                QApplication::processEvents();
                
                QTimer::singleShot(100, [=]() {
                    playerRef->loadExternalSubtitles(subFile);
                });
            }
        });
        
        // Audio wiring
        connect(audioCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                [=](int index) { playerRef->setAudioTrack(index); });

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
