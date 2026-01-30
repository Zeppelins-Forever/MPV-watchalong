QT       += core gui widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Platform-specific MPV configuration
macx {
    INCLUDEPATH += $$PWD/include
    LIBS += -L/opt/homebrew/lib -lmpv
    CONFIG += release
}

win32 {
    # Adjust this path to where you extracted mpv-dev
    MPV_PATH = C:\mpv-dev
    INCLUDEPATH += $$MPV_PATH/include
    LIBS += -L$$MPV_PATH -lmpv.dll
    CONFIG += release
}

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += mpv
    CONFIG += release
}
