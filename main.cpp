// ============================================================================
// main.cpp - Application Entry Point
// ============================================================================
// This is the entry point for the MPV-watchalong application.
// In C++, the main() function is where program execution begins.
// For Qt applications, main() sets up the Qt framework and starts the event loop.
// ============================================================================

#include "mainwindow.h"      // Our custom MainWindow class (the app's main UI)

#include <QApplication>      // Qt's application class - manages app-wide resources
// and settings. Required for any Qt GUI application.

#include <locale.h>          // C standard library for locale (language/region) settings.
// We need this to fix a compatibility issue with MPV.

// If we are on Linux/Mac, import signals library (since it doesn't exist on Windows.
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    #include <signal.h>
#endif
// ----------------------------------------------------------------------------
// main() - Program entry point
// ----------------------------------------------------------------------------
// Parameters:
//   argc - "argument count" - number of command-line arguments passed to the program
//   argv - "argument vector" - array of C-strings containing the actual arguments
//
// These are standard C/C++ parameters that Qt uses internally for things like
// parsing command-line options (e.g., -style, -geometry, etc.)
// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Ignore SIGPIPE.
    // On Linux, closing a window or stopping audio while a stream is active
    // can trigger this signal. The default action is to crash the app.
    // We must ignore it to let the app continue running.
    #if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
        signal(SIGPIPE, SIG_IGN);
    #endif
    

    // Create the QApplication instance.
    // This MUST be created before any Qt widgets. It:
    //   - Initializes the Qt framework
    //   - Manages application-wide settings (app name, organization, etc.)
    //   - Handles the event loop (processing user input, timers, etc.)
    //   - Manages system resources like fonts, clipboard, etc.
    QApplication a(argc, argv);

    // CRITICAL FIX FOR MPV COMPATIBILITY:
    // Qt automatically sets the application's "locale" (regional settings) to
    // match the user's system. This affects how numbers are formatted - for
    // example, in German locales, "1.5" would be written as "1,5".
    //
    // MPV expects numbers in the standard "C" locale format (using periods for
    // decimals). If we don't reset this, MPV can fail to parse configuration
    // values and may not load videos correctly.
    //
    // LC_NUMERIC specifically controls number formatting, leaving other locale
    // settings (like date/time, currency) unchanged.
    setlocale(LC_NUMERIC, "C");

    // Create our main window instance.
    // This constructs the entire UI and sets up all the MPV players.
    // At this point, the window exists in memory but is not yet visible.
    MainWindow w;

    // Make the window visible on screen.
    // Windows are hidden by default when created, so we must explicitly show them.
    w.show();

    // Start Qt's event loop and wait for it to finish.
    //
    // The event loop is the heart of any GUI application. It:
    //   - Waits for events (mouse clicks, key presses, timer ticks, etc.)
    //   - Dispatches events to the appropriate widgets for handling
    //   - Keeps the application responsive
    //
    // a.exec() blocks here until the application is closed (e.g., user closes
    // the main window). It then returns an exit code:
    //   - 0 typically means success/normal exit
    //   - Non-zero indicates an error
    //
    // We return this exit code to the operating system.
    return a.exec();
}
