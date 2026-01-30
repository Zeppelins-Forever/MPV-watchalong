QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += $$PWD/include

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# -------------------------------------------------------------------
# LINKING MPV LIBRARY (macOS Fix)
# -------------------------------------------------------------------

# 1. Check for Apple Silicon (M1/M2/M3) Homebrew path
exists(/opt/homebrew/lib/libmpv.dylib) {
    message("Found MPV on Apple Silicon path")
    INCLUDEPATH += /opt/homebrew/include
    LIBS += -L/opt/homebrew/lib -lmpv
}
# 2. Check for Intel Mac Homebrew path
else:exists(/usr/local/lib/libmpv.dylib) {
    message("Found MPV on Intel path")
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -lmpv
}
# 3. Fallback/Error if not found
else {
    error("Could not find libmpv.dylib! Please run 'brew install mpv' in terminal.")
}
