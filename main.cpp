#include "mainwindow.h"

#include <QApplication>
#include <locale.h> // <--- 1. Add this include

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // <--- 2. Add this line.
    // Qt sets the locale to the system default, which breaks mpv.
    // We must reset the numeric handling to "C" (standard) for mpv to load.
    setlocale(LC_NUMERIC, "C");

    MainWindow w;
    w.show();
    return a.exec();
}
