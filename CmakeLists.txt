# ==============================================================================
# CMakeLists.txt - CMake Build Configuration for MPV-watchalong
# ==============================================================================
#
# CMake is a "meta build system" - it doesn't compile code directly. Instead,
# it generates build files for other systems (Makefiles, Ninja, Visual Studio
# projects, Xcode projects, etc.). This makes it highly portable.
#
# WHY CMAKE OVER QMAKE?
# ---------------------
# 1. Industry Standard: CMake is the de facto standard for C++ projects
# 2. Better IDE Support: Most IDEs (CLion, VS Code, Visual Studio) prefer CMake
# 3. More Powerful: Better dependency management, testing, packaging support
# 4. Qt's Future: Qt 6 was built with CMake; qmake is in maintenance mode
# 5. Cross-Project Compatibility: Easier to integrate with other CMake libraries
#
# HOW TO BUILD WITH CMAKE:
# ------------------------
# Method 1: Command Line (Traditional)
#   mkdir build          # Create a build directory (keeps source clean)
#   cd build
#   cmake ..             # Generate build files (.. points to source directory)
#   cmake --build .      # Compile the project
#
# Method 2: Command Line (Modern CMake 3.13+)
#   cmake -B build       # Generate build files in "build" directory
#   cmake --build build  # Compile
#
# Method 3: Qt Creator
#   File -> Open File or Project -> Select CMakeLists.txt
#   Qt Creator handles everything automatically!
#
# Method 4: VS Code
#   Install "CMake Tools" extension, open folder, it auto-detects CMakeLists.txt
#
# ==============================================================================

# ------------------------------------------------------------------------------
# CMake Minimum Version
# ------------------------------------------------------------------------------
# cmake_minimum_required() MUST be the first command in CMakeLists.txt.
# It specifies the minimum CMake version required to build this project.
#
# Version 3.16 is chosen because:
#   - It's the minimum for Qt 6 support
#   - It has good Qt5 support
#   - It includes many modern features (Unity builds, PCH, etc.)
#   - It's available in most Linux distros (Ubuntu 20.04+)
#
# The FATAL_ERROR option makes CMake stop with an error if the version is too old.
# (In modern CMake this is the default, but we include it for clarity.)
# ------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# ------------------------------------------------------------------------------
# Project Declaration
# ------------------------------------------------------------------------------
# project() defines the project name and metadata. This should come right after
# cmake_minimum_required().
#
# Parameters:
#   - MPV-watchalong: The project name (used in IDE titles, package names, etc.)
#   - VERSION 1.0.0: Semantic version number (MAJOR.MINOR.PATCH)
#   - DESCRIPTION: Human-readable description
#   - LANGUAGES CXX: We're using C++ (CXX). Could also include C, OBJC, etc.
#
# After this command, CMake sets variables like:
#   - PROJECT_NAME = "MPV-watchalong"
#   - PROJECT_VERSION = "1.1.3"
#   - PROJECT_VERSION_MAJOR = 1
#   - PROJECT_VERSION_MINOR = 0
#   - PROJECT_VERSION_PATCH = 0
# ------------------------------------------------------------------------------
project(MPV-watchalong
    VERSION 1.0.0
    DESCRIPTION "Synchronized dual video player using MPV"
    LANGUAGES CXX
)

# ------------------------------------------------------------------------------
# C++ Standard Configuration
# ------------------------------------------------------------------------------
# These commands configure the C++ standard for the entire project.
#
# CMAKE_CXX_STANDARD: Which C++ version to use (11, 14, 17, 20, 23)
#   - We use 17 for modern features (structured bindings, if-init, etc.)
#
# CMAKE_CXX_STANDARD_REQUIRED: If ON, CMake will error if the compiler doesn't
#   support the requested standard. If OFF, it falls back to an older standard.
#
# CMAKE_CXX_EXTENSIONS: Whether to use compiler-specific extensions
#   - ON: Use extensions like GNU++ (g++ -std=gnu++17)
#   - OFF: Use strict standard (g++ -std=c++17)
#   - OFF is more portable across compilers
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------------------
# Qt-Specific CMake Configuration
# ------------------------------------------------------------------------------
# These settings enable Qt's CMake integration features.
#
# CMAKE_AUTOMOC: Automatically run MOC (Meta-Object Compiler) on headers
#   - MOC processes Q_OBJECT macros to generate signal/slot code
#   - Without this, you'd need to manually specify which files need MOC
#
# CMAKE_AUTORCC: Automatically run RCC (Resource Compiler) on .qrc files
#   - RCC compiles Qt resource files (images, etc.) into the binary
#   - We don't use .qrc files, but it's good to enable anyway
#
# CMAKE_AUTOUIC: Automatically run UIC (User Interface Compiler) on .ui files
#   - UIC converts Qt Designer .ui files to C++ header files
#   - Creates ui_mainwindow.h from mainwindow.ui
# ------------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ------------------------------------------------------------------------------
# Find Qt Package
# ------------------------------------------------------------------------------
# find_package() searches for installed libraries and makes them available.
#
# Qt6/Qt5 Detection Strategy:
#   We first try to find Qt6. If not found, we fall back to Qt5.
#   This makes the project compatible with both versions.
#
# Parameters:
#   - Qt6 or Qt5: Package name
#   - QUIET: Don't print messages (we handle messaging ourselves)
#   - COMPONENTS: Which Qt modules we need
#       - Widgets: GUI widgets (buttons, labels, etc.) - includes Core and Gui
#
# After find_package succeeds, it defines:
#   - Qt6_FOUND or Qt5_FOUND: TRUE if found
#   - Qt::Widgets: Target to link against (modern CMake style)
#   - Qt6::Widgets or Qt5::Widgets: Version-specific target names
#
# The QT_VERSION_MAJOR variable is set by Qt's CMake files and tells us
# which major version (5 or 6) was found.
# ------------------------------------------------------------------------------
find_package(Qt6 QUIET COMPONENTS Widgets)

if(Qt6_FOUND)
    message(STATUS "Found Qt6: ${Qt6_VERSION}")
    # Qt6 uses Qt:: namespace for all targets
    set(QT_LIBRARIES Qt6::Widgets)
else()
    # Qt6 not found, try Qt5
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    message(STATUS "Found Qt5: ${Qt5_VERSION}")
    # Qt5 also uses Qt5:: namespace, but we'll alias it
    set(QT_LIBRARIES Qt5::Widgets)
endif()

# ------------------------------------------------------------------------------
# Find MPV Library
# ------------------------------------------------------------------------------
# MPV doesn't provide CMake config files, so we use pkg-config to find it.
# pkg-config is a standard Unix tool that provides compiler/linker flags.
#
# PkgConfig is a CMake module that wraps the pkg-config tool.
# QUIET means don't complain if pkg-config isn't found (we handle it below).
# ------------------------------------------------------------------------------
find_package(PkgConfig QUIET)

# Try to find MPV using pkg-config (works on Linux and macOS with Homebrew)
if(PkgConfig_FOUND)
    # pkg_check_modules searches for a package and creates variables:
    #   - MPV_FOUND: TRUE if found
    #   - MPV_INCLUDE_DIRS: Directories containing headers
    #   - MPV_LIBRARIES: Libraries to link
    #   - MPV_LIBRARY_DIRS: Directories containing libraries
    pkg_check_modules(MPV QUIET mpv)
endif()

# If pkg-config didn't find MPV, try manual detection
if(NOT MPV_FOUND)
    message(STATUS "pkg-config didn't find MPV, trying manual detection...")

    # ------------------------------------------------------------------------
    # Platform-Specific MPV Detection
    # ------------------------------------------------------------------------
    # CMAKE_SYSTEM_NAME contains the OS name: "Darwin" (macOS), "Windows", "Linux"
    # We use this to set platform-specific search paths.
    # ------------------------------------------------------------------------

    if(APPLE)
        # macOS: Check Homebrew locations
        # Apple Silicon Macs use /opt/homebrew, Intel Macs use /usr/local
        set(MPV_SEARCH_PATHS
            /opt/homebrew          # Apple Silicon Homebrew
            /usr/local             # Intel Homebrew
            ${CMAKE_SOURCE_DIR}    # Project directory (for bundled MPV)
        )
    elseif(WIN32)
        # Windows: Check common locations and environment variable
        # Users should set MPV_DIR environment variable or put mpv-dev in project
        set(MPV_SEARCH_PATHS
            $ENV{MPV_DIR}                    # Environment variable
            ${CMAKE_SOURCE_DIR}/mpv-dev      # Subdirectory of project
            "C:/mpv-dev"                     # Common location
        )
    else()
        # Linux: Check standard locations
        set(MPV_SEARCH_PATHS
            /usr
            /usr/local
        )
    endif()

    # ------------------------------------------------------------------------
    # Search for MPV Header
    # ------------------------------------------------------------------------
    # find_path searches for a file in a list of directories.
    # If found, MPV_INCLUDE_DIR is set to the directory containing it.
    # HINTS provides suggested locations to search first.
    # PATH_SUFFIXES are subdirectories to check within each search path.
    # ------------------------------------------------------------------------
    find_path(MPV_INCLUDE_DIR
        NAMES mpv/client.h           # The file we're looking for
        HINTS ${MPV_SEARCH_PATHS}    # Where to look
        PATH_SUFFIXES include        # Check "include" subdirectory
    )

    # ------------------------------------------------------------------------
    # Search for MPV Library
    # ------------------------------------------------------------------------
    # find_library searches for a library file.
    # CMake automatically handles platform differences:
    #   - Linux: looks for libmpv.so
    #   - macOS: looks for libmpv.dylib
    #   - Windows: looks for mpv.lib, mpv.dll.a, etc.
    # ------------------------------------------------------------------------
    find_library(MPV_LIBRARY
        NAMES mpv mpv.dll            # Library name (without lib prefix or extension)
        HINTS ${MPV_SEARCH_PATHS}    # Where to look
        PATH_SUFFIXES lib lib64      # Check lib and lib64 subdirectories
    )

    # Check if we found both header and library
    if(MPV_INCLUDE_DIR AND MPV_LIBRARY)
        set(MPV_FOUND TRUE)
        set(MPV_INCLUDE_DIRS ${MPV_INCLUDE_DIR})
        set(MPV_LIBRARIES ${MPV_LIBRARY})
        message(STATUS "Found MPV: ${MPV_LIBRARY}")
    else()
        # Provide helpful error message
        message(FATAL_ERROR
            "MPV library not found!\n"
            "Please install MPV development files:\n"
            "  - macOS: brew install mpv\n"
            "  - Ubuntu/Debian: sudo apt install libmpv-dev\n"
            "  - Windows: Download from https://sourceforge.net/projects/mpv-player-windows/files/libmpv/\n"
            "             and set MPV_DIR environment variable or place in ${CMAKE_SOURCE_DIR}/mpv-dev\n"
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# Define Source Files
# ------------------------------------------------------------------------------
# We list all source files in a variable for clarity and reuse.
# Using a variable makes it easier to manage as the project grows.
#
# In larger projects, you might use file(GLOB ...) to auto-detect files,
# but explicit listing is preferred because:
#   1. CMake re-runs automatically when CMakeLists.txt changes
#   2. With GLOB, adding a file requires manual CMake re-run
#   3. Explicit lists make the build more predictable
# ------------------------------------------------------------------------------
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# ==============================================================================
# APPLICATION ICON CONFIGURATION (BEFORE add_executable)
# ==============================================================================
# Platform-specific icon handling. Unlike qmake, CMake requires more explicit
# configuration for application icons.
#
# IMPORTANT: For macOS, the icon file MUST be added to PROJECT_SOURCES before
# creating the executable. This ensures the icon is properly included in the
# app bundle. Setting source file properties and adding via target_sources()
# after qt_add_executable() may not work reliably, especially with Qt 6.
#
# NOTE ON LINUX: Linux does not support embedded application icons in ELF
# executables. Desktop environments use external icon files referenced by
# .desktop files, following the freedesktop.org specification. Linux icon
# installation is handled in the installation section below.
# ==============================================================================

# ------------------------------------------------------------------------------
# macOS Icon - Add to Sources BEFORE Creating Executable
# ------------------------------------------------------------------------------
if(APPLE)
    set(MACOS_ICON_FILE "${CMAKE_SOURCE_DIR}/icons/app-icon.icns")
    if(EXISTS ${MACOS_ICON_FILE})
        # Add icon to project sources - this is critical for proper bundle inclusion
        list(APPEND PROJECT_SOURCES ${MACOS_ICON_FILE})

        # Tell CMake this is a resource file that goes in the bundle's Resources folder
        set_source_files_properties(${MACOS_ICON_FILE} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )

        message(STATUS "macOS icon found: ${MACOS_ICON_FILE}")
    else()
        message(STATUS "macOS icon not found at ${MACOS_ICON_FILE}")
    endif()
endif()

# ------------------------------------------------------------------------------
# Windows Icon - Generate Resource File BEFORE Creating Executable
# ------------------------------------------------------------------------------
if(WIN32)
    set(WINDOWS_ICON_FILE "${CMAKE_SOURCE_DIR}/icons/app-icon.ico")
    if(EXISTS ${WINDOWS_ICON_FILE})
        # Windows requires a .rc (resource script) file to embed the icon
        # We generate one automatically
        set(RC_FILE "${CMAKE_BINARY_DIR}/app-icon.rc")

        # Create the .rc file content
        # IDI_ICON1 is a standard identifier for the application icon
        file(WRITE ${RC_FILE} "IDI_ICON1 ICON \"${WINDOWS_ICON_FILE}\"\n")

        # Add the .rc file to the project sources
        list(APPEND PROJECT_SOURCES ${RC_FILE})

        message(STATUS "Windows icon configured: ${WINDOWS_ICON_FILE}")
    else()
        message(STATUS "Windows icon not found at ${WINDOWS_ICON_FILE}")
    endif()
endif()

# ------------------------------------------------------------------------------
# Create the Executable Target
# ------------------------------------------------------------------------------
# add_executable() creates a build target for an executable program.
#
# For Qt 6, we use qt_add_executable() which adds some Qt-specific handling
# (like macOS bundle setup). For Qt 5, we use regular add_executable().
#
# The WIN32 option (Windows) / MACOSX_BUNDLE option (macOS) create a GUI
# application instead of a console application:
#   - Windows: No console window appears when running the app
#   - macOS: Creates a proper .app bundle
#
# NOTE: PROJECT_SOURCES now includes platform-specific icon resources
# (macOS .icns file, Windows .rc file) that were added above.
# ------------------------------------------------------------------------------
if(Qt6_FOUND)
    # Qt 6 provides qt_add_executable for better integration
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION        # We'll call qt_finalize_executable later
        MACOSX_BUNDLE
        WIN32
        ${PROJECT_SOURCES}
    )
else()
    # Qt 5 uses standard CMake command
    add_executable(${PROJECT_NAME}
        WIN32                      # Windows: GUI app (no console)
        MACOSX_BUNDLE             # macOS: Create .app bundle
        ${PROJECT_SOURCES}
    )
endif()

# ------------------------------------------------------------------------------
# macOS Bundle Properties (AFTER Creating Executable)
# ------------------------------------------------------------------------------
# Set bundle metadata properties. This must come after add_executable but
# the icon file itself must have been included in the sources above.
# ------------------------------------------------------------------------------
if(APPLE AND EXISTS "${CMAKE_SOURCE_DIR}/icons/app-icon.icns")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        # MACOSX_BUNDLE_ICON_FILE should be just the filename, not the full path
        # CMake will look for this file in the Resources folder of the bundle
        MACOSX_BUNDLE_ICON_FILE "app-icon.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.mpv-watchalong"
    )
    message(STATUS "macOS bundle properties configured")
endif()

# ------------------------------------------------------------------------------
# Include Directories
# ------------------------------------------------------------------------------
# target_include_directories() specifies where to find header files.
#
# PRIVATE means these directories are only for building this target,
# not for other targets that might link against it.
#
# We add:
#   1. MPV include directories (where mpv/client.h is located)
#   2. Project source directory (for our own headers)
#   3. A local "include" folder (for any bundled headers)
# ------------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${MPV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# ------------------------------------------------------------------------------
# Link Libraries
# ------------------------------------------------------------------------------
# target_link_libraries() specifies which libraries to link against.
#
# Modern CMake uses "target-based" linking. Instead of just library names,
# we link against "targets" (like Qt6::Widgets) which automatically bring
# along their include directories and compiler flags. This is called
# "transitive dependencies" - if Qt::Widgets needs Qt::Core, it's automatic.
#
# PRIVATE means we use these libraries internally but don't expose them
# to targets that link against us (not relevant for executables, but
# good practice).
# ------------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_LIBRARIES}        # Qt::Widgets (includes Core and Gui)
    ${MPV_LIBRARIES}       # libmpv
)

# On Linux, we might need to add the library directory to the runtime path
if(MPV_LIBRARY_DIRS)
    target_link_directories(${PROJECT_NAME} PRIVATE ${MPV_LIBRARY_DIRS})
endif()

# ------------------------------------------------------------------------------
# Qt 6 Finalization
# ------------------------------------------------------------------------------
# Qt 6 requires calling qt_finalize_executable() to complete setup.
# This handles various platform-specific details.
# ------------------------------------------------------------------------------
if(Qt6_FOUND)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# ==============================================================================
# INSTALLATION RULES (Optional)
# ==============================================================================
# These rules define how to install the built application.
# Run "cmake --install build" (or "make install") to install.
#
# GNUInstallDirs provides standard installation directory variables:
#   - CMAKE_INSTALL_BINDIR: Where to put executables (usually "bin")
#   - CMAKE_INSTALL_LIBDIR: Where to put libraries (usually "lib")
#   - etc.
# ==============================================================================

include(GNUInstallDirs)

# Install the executable
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .                              # macOS: .app bundle
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}       # Windows/Linux: executable
)

# ------------------------------------------------------------------------------
# Linux Desktop Integration
# ------------------------------------------------------------------------------
# Linux does NOT support embedded application icons in ELF executables.
# Instead, desktop environments follow the freedesktop.org specification:
#   - Icons are installed to /usr/share/icons/hicolor/<size>/apps/
#   - .desktop files in /usr/share/applications/ reference these icons
#
# For a complete Linux installation, you would also need a .desktop file.
# Example mpv-watchalong.desktop:
#   [Desktop Entry]
#   Type=Application
#   Name=MPV Watchalong
#   Exec=mpv-watchalong
#   Icon=mpv-watchalong
#   Categories=AudioVideo;Video;
# ------------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    # Install icon for Linux desktop
    install(FILES icons/app-icon.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps
        RENAME mpv-watchalong.png
        OPTIONAL  # Don't fail if file doesn't exist
    )

    # Optionally install a .desktop file if one exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/mpv-watchalong.desktop")
        install(FILES mpv-watchalong.desktop
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        )
    endif()
endif()

# ==============================================================================
# BUILD INFORMATION
# ==============================================================================
# Print useful information at the end of configuration.
# This helps with debugging build issues.
# ==============================================================================

message(STATUS "")
message(STATUS "=== MPV-watchalong Build Configuration ===")
message(STATUS "CMake version:     ${CMAKE_VERSION}")
message(STATUS "C++ compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ standard:      C++${CMAKE_CXX_STANDARD}")
message(STATUS "Qt version:        ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}")
message(STATUS "MPV include:       ${MPV_INCLUDE_DIRS}")
message(STATUS "MPV library:       ${MPV_LIBRARIES}")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==========================================")
message(STATUS "")
