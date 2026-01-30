QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

# --- OS-SPECIFIC LINKING ---

# 1. Linux (Ubuntu/Debian/Fedora)
unix:!macx {
    # On Linux, installing libmpv-dev puts files in standard paths (/usr/include and /usr/lib).
    # We don't need to specify paths manually, just link the library.
    LIBS += -lmpv
}

# 2. macOS (Keep this if you ever go back to Mac)
macx {
    exists(/opt/homebrew/lib/libmpv.dylib) {
        INCLUDEPATH += /opt/homebrew/include
        LIBS += -L/opt/homebrew/lib -lmpv
    } else:exists(/usr/local/lib/libmpv.dylib) {
        INCLUDEPATH += /usr/local/include
        LIBS += -L/usr/local/lib -lmpv
    }
}

# 3. Windows (For future reference)
win32 {
    # You would point this to where you downloaded the Windows dev builds
    # INCLUDEPATH += C:/mpv-dev/include
    # LIBS += -LC:/mpv-dev/lib -lmpv
}
