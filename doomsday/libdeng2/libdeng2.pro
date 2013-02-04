# The Doomsday Engine Project
# Copyright (c) 2011-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
# Copyright (c) 2011-2013 Daniel Swanson <danij@dengine.net>

TEMPLATE = lib
TARGET = deng2

# Build Configuration --------------------------------------------------------

include(../config.pri)
include(../dep_zlib.pri)

# Version numbers.
LIBDENG2_RELEASE_LABEL  = Dev
LIBDENG2_MAJOR_VERSION  = 2
LIBDENG2_MINOR_VERSION  = 0
LIBDENG2_PATCHLEVEL     = 0

VERSION = $${LIBDENG2_MAJOR_VERSION}.$${LIBDENG2_MINOR_VERSION}.$${LIBDENG2_PATCHLEVEL}

DEFINES += LIBDENG2_RELEASE_LABEL=\\\"$$LIBDENG2_RELEASE_LABEL\\\"
DEFINES += LIBDENG2_MAJOR_VERSION=$$LIBDENG2_MAJOR_VERSION
DEFINES += LIBDENG2_MINOR_VERSION=$$LIBDENG2_MINOR_VERSION
DEFINES += LIBDENG2_PATCHLEVEL=$$LIBDENG2_PATCHLEVEL
DEFINES += LIBDENG2_VERSION=\\\"$${LIBDENG2_MAJOR_VERSION}.$${LIBDENG2_MINOR_VERSION}.$${LIBDENG2_PATCHLEVEL}\\\"

DEFINES += __DENG2__

# Using Qt.
QT += core network gui

win32 {
    # Keep the version number out of the file name.
    TARGET_EXT = .dll

    # For Windows, an .rc file is generated for embedded version info in the binary.
    system(python "$$PWD/res/win32/generate_rc.py" "$$PWD/res/win32/deng2.rc" \
        $$LIBDENG2_MAJOR_VERSION $$LIBDENG2_MINOR_VERSION $$LIBDENG2_PATCHLEVEL \
        $$LIBDENG2_RELEASE_LABEL $$DENG_BUILD)
    RC_FILE = res/win32/deng2.rc

    OTHER_FILES += res/win32/deng2.rc.in
}

# Enable strict warnings for C++ code.
*-g++|*-clang* {
    QMAKE_CXXFLAGS_WARN_ON *= -Wall -Wextra -pedantic
}
*-g++: QMAKE_CXXFLAGS_WARN_ON *= -Wno-long-long
win32-msvc* {
    #QMAKE_CXXFLAGS_WARN_ON ~= s/-W3/-W4/
}

INCLUDEPATH += include

# Source Files ---------------------------------------------------------------

include(data.pri)
include(filesys.pri)
include(legacy.pri)
include(network.pri)
include(scriptsys.pri)
include(widgets.pri)

# Convenience headers.
HEADERS += \
    include/de/App \
    include/de/Clock \
    include/de/CommandLine \
    include/de/Config \
    include/de/DebugLogSink \
    include/de/Error \
    include/de/FileLogSink \
    include/de/Library \
    include/de/Log \
    include/de/LogBuffer \
    include/de/LogSink \
    include/de/MonospaceLogSinkFormatter \
    include/de/Rectangle \
    include/de/TextStreamLogSink \
    include/de/UnixInfo \
    include/de/Vector \
    include/de/Version

HEADERS += \
    include/de/c_wrapper.h \
    include/de/error.h \
    include/de/libdeng2.h \
    include/de/math.h \
    include/de/rectangle.h \
    include/de/vector.h \
    include/de/version.h \
    include/de/core/app.h \
    include/de/core/clock.h \
    include/de/core/commandline.h \
    include/de/core/config.h \
    include/de/core/debuglogsink.h \
    include/de/core/filelogsink.h \
    include/de/core/library.h \
    include/de/core/log.h \
    include/de/core/logbuffer.h \
    include/de/core/logsink.h \
    include/de/core/monospacelogsinkformatter.h \
    include/de/core/textstreamlogsink.h \
    include/de/core/unixinfo.h

# Private headers.
HEADERS += \
    src/core/logtextstyle.h \
    src/core/callbacktimer.h

SOURCES += \
    src/c_wrapper.cpp \
    src/error.cpp \
    src/version.cpp \
    src/core/app.cpp \
    src/core/callbacktimer.cpp \
    src/core/clock.cpp \
    src/core/commandline.cpp \
    src/core/config.cpp \
    src/core/debuglogsink.cpp \
    src/core/filelogsink.cpp \
    src/core/library.cpp \
    src/core/log.cpp \
    src/core/logbuffer.cpp \
    src/core/logsink.cpp \
    src/core/monospacelogsinkformatter.cpp \
    src/core/textstreamlogsink.cpp \
    src/core/unixinfo.cpp 

OTHER_FILES += \
    modules/Config.de \
    modules/recutil.de

# Installation ---------------------------------------------------------------

macx {
    defineTest(fixInstallName) {
        doPostLink("install_name_tool -change $$1 @executable_path/../Frameworks/$$1 libdeng2.2.dylib")
        doPostLink("install_name_tool -change $$(QTDIR)lib/$$1 @executable_path/../Frameworks/$$1 libdeng2.2.dylib")
        doPostLink("install_name_tool -change $$(QTDIR)/lib/$$1 @executable_path/../Frameworks/$$1 libdeng2.2.dylib")
    }
    doPostLink("install_name_tool -id @executable_path/../Frameworks/libdeng2.2.dylib libdeng2.2.dylib")
    fixInstallName("QtCore.framework/Versions/4/QtCore")
    fixInstallName("QtNetwork.framework/Versions/4/QtNetwork")
    fixInstallName("QtGui.framework/Versions/4/QtGui")

    # Update the library included in the main app bundle.
    doPostLink("mkdir -p ../client/Doomsday.app/Contents/Frameworks")
    doPostLink("cp -fRp libdeng2*dylib ../client/Doomsday.app/Contents/Frameworks")
}

!macx {
    INSTALLS += target
    target.path = $$DENG_LIB_DIR
}
