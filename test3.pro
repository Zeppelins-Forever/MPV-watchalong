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
    # Make sure you have mpv installed on system via Brew
    #INCLUDEPATH += $$PWD/include
    #LIBS += -L/opt/homebrew/lib -lmpv
    #CONFIG += release
    exists(/opt/homebrew/lib/libmpv.dylib) {
            INCLUDEPATH += /opt/homebrew/include
            LIBS += -L/opt/homebrew/lib -lmpv
    } else:exists(/usr/local/lib/libmpv.dylib) {
            INCLUDEPATH += /usr/local/include
            LIBS += -L/usr/local/lib -lmpv
    }
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
