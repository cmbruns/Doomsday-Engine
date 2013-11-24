# The Doomsday Engine Project
# Copyright (c) 2011-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
# Copyright (c) 2011-2013 Daniel Swanson <danij@dengine.net>
#
# Do not modify this file. Custom CONFIG options can be specified on the 
# qmake command line or in config_user.pri.
#
# User-definable variables:
#   PREFIX          Install prefix for Unix
#   SCRIPT_PYTHON   Path of the Python interpreter binary to be used in
#                   generated scripts (python on path used for building)
#
# CONFIG options for Doomsday:
# - deng_aptstable              Include the stable apt repository .list
# - deng_aptunstable            Include the unstable apt repository .list
# - deng_fluidsynth             Build the FluidSynth sound driver
# - deng_fmod                   Build the FMOD Ex sound driver
# - deng_nativesdk              (Mac) Use current OS's SDK for non-distrib use
# - deng_noclient               Disable building of the client (not on Mac)
# - deng_nodirectsound          (Windows) Disable the DirectSound sound driver
# - deng_nodisplaymode          Disable native display mode changes
# - deng_nofixedasm             Disable assembler fixed-point math
# - deng_noopenal               Disable building of the OpenAL sound driver
# - deng_nosdlmixer             Disable SDL_mixer; use dummy driver as default
# - deng_nosnowberry            (Unix) Exclude Snowberry from installation
# - deng_notools                Do not build and deploy the tools
# - deng_openal                 Build the OpenAL sound driver
# - deng_qtautoselect           (Mac) Select OS X SDK based on Qt version
# - deng_nopackres              Do not package the Doomsday resources
# - deng_rangecheck             Parameter range checking/value assertions
# - deng_snowberry              (Unix) Include Snowberry in installation
# - deng_tests                  Build and deploy the test suite
# - deng_writertypecheck        Enable type checking in Writer/Reader
#
# Read-only options (set automatically):
# - deng_debug                  Debug build.

QT -= core gui
CONFIG *= thread

# Directories ----------------------------------------------------------------

DENG_API_DIR          = $$PWD/api
DENG_INCLUDE_DIR      = $$PWD/client/include
DENG_UNIX_INCLUDE_DIR = $$DENG_INCLUDE_DIR/unix
DENG_MAC_INCLUDE_DIR  = $$DENG_INCLUDE_DIR/macx
DENG_WIN_INCLUDE_DIR  = $$DENG_INCLUDE_DIR/windows
DENG_MODULES_DIR      = $$PWD/libdeng2/modules

# Versions -------------------------------------------------------------------

# Parse versions from the header files.
!exists(versions.pri): system(python configure.py)

include(versions.pri)

# Macros ---------------------------------------------------------------------

defineTest(echo) {
    deng_verbosebuildconfig {
        !win32 {
            message($$1)
        } else {
            # We don't want to get the printed messages after everything else,
            # so print to stdout.
            system(echo $$1)
        }
    }
}

defineTest(useLibDir) {
    btype = ""
    win32 {
        deng_debug: btype = "/Debug"
              else: btype = "/Release"
    }
    exists($${1}$${btype}) {
        LIBS += -L$${1}$${btype}
        export(LIBS)
        return(true)
    }
    return(false)
}

defineTest(doPostLink) {
    isEmpty(QMAKE_POST_LINK) {
        QMAKE_POST_LINK = $$1
    } else {
        QMAKE_POST_LINK = $$QMAKE_POST_LINK && $$1
    }
    export(QMAKE_POST_LINK)
}

macx {
    defineTest(removeQtLibPrefix) {
        doPostLink("install_name_tool -change $$[QT_INSTALL_LIBS]/$$2 $$2 $$1")
    }
    defineTest(fixInstallName) {
        # 1: binary file
        # 2: library name
        # 3: path to Frameworks/
        removeQtLibPrefix($$1, $$2)
        doPostLink("install_name_tool -change $$2 @executable_path/$$3/Frameworks/$$2 $$1")
    }
    defineTest(fixPluginInstallId) {
        # 1: target name
        # 2: version
        doPostLink("install_name_tool -id @executable_path/../DengPlugins/$${1}.bundle/Versions/$$2/$$1 $${1}.bundle/Versions/$$2/$$1")
    }
}

# Build Options --------------------------------------------------------------

# Configure for Debug/Release build.
CONFIG(debug, debug|release) {
    !win32: echo(Debug build.)
    DEFINES += _DEBUG
    CONFIG += deng_debug deng_rangecheck
} else {
    !win32: echo(Release build.)
    DEFINES += NDEBUG
}

# Debugging options.
deng_fakememoryzone: DEFINES += LIBDENG_FAKE_MEMORY_ZONE

# Check for Qt 5.
greaterThan(QT_MAJOR_VERSION, 4) {
    CONFIG += deng_qt5 c++11
}

# Check for a 64-bit compiler.
contains(QMAKE_HOST.arch, x86_64) {
    echo(64-bit architecture detected.)
    DEFINES += HOST_IS_64BIT
    win32: CONFIG += win64
}

isStableRelease(): DEFINES += DENG_STABLE

# Options defined by the user (may not exist).
exists(config_user.pri): include(config_user.pri)

    win32: include(config_win32.pri)
else:macx: include(config_macx.pri)
     else: include(config_unix.pri)

# Apply deng_* Configuration -------------------------------------------------

unix:deng_ccache {
    # ccache can be used to speed up recompilation.
    *-clang* {
        QMAKE_CC  = ccache $$QMAKE_CC -Qunused-arguments
        QMAKE_CXX = ccache $$QMAKE_CXX -Qunused-arguments
        QMAKE_CXXFLAGS_WARN_ON += -Wno-self-assign
    }
    *-gcc*|*-g++* {
        QMAKE_CC  = ccache $$QMAKE_CC
        QMAKE_CXX = ccache $$QMAKE_CXX
    }
}

deng_nofixedasm {
    DEFINES += DENG_NO_FIXED_ASM
}
!deng_rangecheck {
    DEFINES += DENG_NO_RANGECHECKING
}
deng_nosdlmixer|deng_nosdl {
    DEFINES += DENG_DISABLE_SDLMIXER
}
deng_nosdl {
    DEFINES += DENG_NO_SDL
}
