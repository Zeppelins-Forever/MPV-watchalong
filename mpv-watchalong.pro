# ==============================================================================
# MPV-watchalong.pro - Qt Project File
# ==============================================================================
# This file tells Qt's build system (qmake) how to compile the project.
# It specifies which source files to compile, what libraries to link,
# and various build settings.
#
# To build the project:
#   1. Run: qmake MPV-watchalong.pro  (generates Makefile)
#   2. Run: make                       (compiles the project)
# Or just open this file in Qt Creator and click Build!
# ==============================================================================

# ------------------------------------------------------------------------------
# Qt Modules
# ------------------------------------------------------------------------------
# QT variable specifies which Qt modules to include.
# Each module provides different functionality:
#   - core: Core non-GUI classes (QString, QFile, etc.)
#   - gui: Base GUI functionality (colors, fonts, images)
#   - widgets: UI widgets (buttons, labels, layouts, etc.)
# ------------------------------------------------------------------------------
QT       += core gui widgets

# ------------------------------------------------------------------------------
# Build Configuration
# ------------------------------------------------------------------------------
# CONFIG specifies various build options.
# c++17 enables C++17 standard features (modern C++ with lambdas, auto, etc.)
# ------------------------------------------------------------------------------
CONFIG += c++17

# ------------------------------------------------------------------------------
# Target Name
# ------------------------------------------------------------------------------
# TARGET specifies the name of the output executable/application.
# This produces:
#   - Windows: MPV-watchalong.exe
#   - Linux: MPV-watchalong
#   - macOS: MPV-watchalong.app (application bundle)
# ------------------------------------------------------------------------------
TARGET = MPV-watchalong

# ------------------------------------------------------------------------------
# Source Files
# ------------------------------------------------------------------------------
# SOURCES lists all .cpp files to compile.
# These are the implementation files containing actual code.
# ------------------------------------------------------------------------------
SOURCES += \
    main.cpp \
    mainwindow.cpp

# ------------------------------------------------------------------------------
# Header Files
# ------------------------------------------------------------------------------
# HEADERS lists all .h files.
# Qt's MOC (Meta-Object Compiler) processes headers with Q_OBJECT macro.
# ------------------------------------------------------------------------------
HEADERS += \
    mainwindow.h

# ------------------------------------------------------------------------------
# UI Form Files
# ------------------------------------------------------------------------------
# FORMS lists Qt Designer .ui files.
# Qt's UIC (User Interface Compiler) converts these to C++ code.
# ------------------------------------------------------------------------------
FORMS += \
    mainwindow.ui

# ==============================================================================
# APPLICATION ICON CONFIGURATION
# ==============================================================================
# Each platform handles application icons differently.
# See the README section below for where to place icon files.
# ==============================================================================

# ------------------------------------------------------------------------------
# macOS Icon
# ------------------------------------------------------------------------------
# macOS uses .icns files (Apple Icon Image format).
# ICON specifies the path to the icon file.
# Qt will automatically include it in the application bundle.
# ------------------------------------------------------------------------------
macx {
    ICON = icons/app-icon.icns
}

# ------------------------------------------------------------------------------
# Windows Icon
# ------------------------------------------------------------------------------
# Windows uses .ico files embedded via a resource file (.rc).
# RC_ICONS is a convenient shorthand - Qt generates the .rc file automatically.
# The icon will be embedded in the .exe file.
# ------------------------------------------------------------------------------
win32 {
    RC_ICONS = icons/app-icon.ico
}

# ------------------------------------------------------------------------------
# Linux Icon
# ------------------------------------------------------------------------------
# Linux doesn't embed icons in executables. Instead, icons are installed
# separately and referenced in .desktop files. For development, the icon
# won't show on the executable itself, but you can include it in your
# installation/packaging scripts.
#
# For proper Linux deployment, you'd create a .desktop file that references
# the icon. This is typically done during packaging (deb, rpm, AppImage, etc.)
# ------------------------------------------------------------------------------

# ==============================================================================
# PLATFORM-SPECIFIC MPV CONFIGURATION
# ==============================================================================
# MPV library location varies by platform. We use conditional blocks to
# specify the correct paths for each operating system.
#
# qmake conditionals:
#   macx      - macOS
#   win32     - Windows (both 32-bit and 64-bit)
#   unix:!macx - Unix-like systems that are NOT macOS (Linux, BSD, etc.)
# ==============================================================================

# ------------------------------------------------------------------------------
# macOS Configuration
# ------------------------------------------------------------------------------
# On macOS, MPV is typically installed via Homebrew.
# Homebrew on Apple Silicon (M1/M2) installs to /opt/homebrew
# Homebrew on Intel Macs installs to /usr/local
#
# We also include a local "include" folder for the mpv headers.
# Adjust paths if your setup differs.
# ------------------------------------------------------------------------------
macx {
    # Add local include folder and Homebrew include path
    INCLUDEPATH += $$PWD/include
    INCLUDEPATH += /opt/homebrew/include

    # Link against libmpv from Homebrew
    LIBS += -L/opt/homebrew/lib -lmpv
}

# ------------------------------------------------------------------------------
# Windows Configuration
# ------------------------------------------------------------------------------
# On Windows, you need to download the MPV development files separately.
# Download from: https://sourceforge.net/projects/mpv-player-windows/files/libmpv/
#
# Extract the archive and set MPV_PATH below to the extraction location.
# The archive contains:
#   - include/mpv/client.h (and other headers)
#   - libmpv.dll.a (import library for linking)
#   - mpv-2.dll (runtime DLL - must be distributed with your app)
# ------------------------------------------------------------------------------
win32 {
    # IMPORTANT: Change this path to where you extracted mpv-dev!
    MPV_PATH = C:/mpv-dev

    # Add MPV headers to include path
    INCLUDEPATH += $$MPV_PATH/include

    # Link against MPV import library
    # Note: The actual DLL (mpv-2.dll) must be in the same folder as the .exe
    # or in the system PATH when running the application.
    LIBS += -L$$MPV_PATH -lmpv.dll
}

# ------------------------------------------------------------------------------
# Linux Configuration
# ------------------------------------------------------------------------------
# On Linux, we use pkg-config to find MPV automatically.
# pkg-config is a standard tool that provides compiler/linker flags for libraries.
#
# First, install the MPV development package:
#   Ubuntu/Debian: sudo apt install libmpv-dev
#   Fedora: sudo dnf install mpv-libs-devel
#   Arch: sudo pacman -S mpv
#
# pkg-config will automatically add the correct -I (include) and -l (link) flags.
# ------------------------------------------------------------------------------
unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += mpv
}

# ==============================================================================
# ICON FILE SETUP INSTRUCTIONS
# ==============================================================================
#
# Create an "icons" folder in your project directory and add these files:
#
# 1. icons/app-icon.icns (macOS)
#    - Apple Icon Image format
#    - Can be created from a PNG using:
#      iconutil -c icns icon.iconset/
#    - Or use an online converter
#    - Should contain multiple resolutions (16x16 to 1024x1024)
#
# 2. icons/app-icon.ico (Windows)
#    - Windows Icon format
#    - Can be created from a PNG using GIMP, ImageMagick, or online converters
#    - Should contain multiple resolutions (16x16, 32x32, 48x48, 256x256)
#
# 3. icons/app-icon.png (Linux - for .desktop files)
#    - Standard PNG image
#    - Recommended sizes: 48x48, 128x128, or 256x256 pixels
#    - Used when creating .desktop files for app launchers
#
# Quick icon creation from a single PNG (using ImageMagick):
#
#   # Create Windows .ico:
#   convert icon-256.png -define icon:auto-resize=256,128,64,48,32,16 app-icon.ico
#
#   # For macOS .icns, you need an iconset folder with specific sizes:
#   mkdir icon.iconset
#   sips -z 16 16     icon-1024.png --out icon.iconset/icon_16x16.png
#   sips -z 32 32     icon-1024.png --out icon.iconset/icon_16x16@2x.png
#   sips -z 32 32     icon-1024.png --out icon.iconset/icon_32x32.png
#   sips -z 64 64     icon-1024.png --out icon.iconset/icon_32x32@2x.png
#   sips -z 128 128   icon-1024.png --out icon.iconset/icon_128x128.png
#   sips -z 256 256   icon-1024.png --out icon.iconset/icon_128x128@2x.png
#   sips -z 256 256   icon-1024.png --out icon.iconset/icon_256x256.png
#   sips -z 512 512   icon-1024.png --out icon.iconset/icon_256x256@2x.png
#   sips -z 512 512   icon-1024.png --out icon.iconset/icon_512x512.png
#   sips -z 1024 1024 icon-1024.png --out icon.iconset/icon_512x512@2x.png
#   iconutil -c icns icon.iconset
#
# ==============================================================================
