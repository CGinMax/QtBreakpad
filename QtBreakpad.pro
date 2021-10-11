QT -= gui
QT += concurrent

TARGET = LibBreakpad
TEMPLATE = lib

DEFINES += LIBBREAKPAD_LIBRARY


DEFINES += QT_DEPRECATED_WARNINGS


SOURCES += \
        $$PWD/libbreakpad.cpp \
        $$PWD/platformhelper.cpp \
        $$PWD/infomanager.cpp

HEADERS += \
        $$PWD/libbreakpad.h \
        $$PWD/libbreakpad_global.h \
        $$PWD/platformhelper.h \
        $$PWD/infomanager.h \

CONFIG(debug, debug | release){
    DESTDIR = $$PWD/../bin/debug
}
else {
    DESTDIR = $$PWD/../bin/release
}

INCLUDEPATH += $$PWD/breakpad/include/

unix:!mac {
    SOURCES += \
        $$PWD/platformhelper_linux.cpp

    LIBS += -L$$PWD/breakpad/lib/linux-x86-64 -lbreakpad
    LIBS += -L$$PWD/breakpad/lib/linux-x86-64 -lbreakpad_client
} else:win32:!win32-g++ {

    SOURCES += \
        $$PWD/platformhelper_win_msvc.cpp
    LIBS += -L$$PWD/breakpad/lib/windows-msvc2017 -lcommon
    LIBS += -L$$PWD/breakpad/lib/windows-msvc2017 -lexception_handler
    LIBS += -L$$PWD/breakpad/lib/windows-msvc2017 -lcrash_generation_client
}
