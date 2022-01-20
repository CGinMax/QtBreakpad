QT += gui widgets concurrent

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
#QMAKE_EXTRA_COMPILERS += copy_files
#copy_files.name = COPY
#copy_files.input = $$PWD/breakpad-linux-x86-64/bin/dump_syms $$PWD/breakpad/linux-x86-64/bin/minidump_stackwalk
#copy_files.output =$$DESTDIR
#copy_files.CONFIG = no_link

#copy_files.output_function = fileCopyDestination
defineReplace(fileCopyDestination) {
    return($$shadowed($$1))
}

unix:!mac {
!exists($$PWD/breakpad/linux-x86-64/lib) {
#    error("请运行$$absolute_path("build-linux.sh", "$$PWD/../buildpad-src")脚本编译breakpad")
#copy_files.commands =
}
#$${PWD}/breakpad/linux-x86-64/bin/minidump_stackwalk
QMAKE_POST_LINK += $$quote(cp -r $${PWD}/breakpad/linux-x86-64/bin/dump_syms  $${DESTDIR}/breakpad)

INCLUDEPATH += $$PWD/breakpad/linux-x86-64/include/breakpad
    SOURCES += \
        $$PWD/platformhelper_linux.cpp

    LIBS += -L$$PWD/breakpad/linux-x86-64/lib/ -lbreakpad -lbreakpad_client
} else:win32:!win32-g++ {
INCLUDEPATH += $$PWD/breakpad/windows-msvc2017/include/breakpad
    SOURCES += \
        $$PWD/platformhelper_win_msvc.cpp
    LIBS += -L$$PWD/breakpad/windows-msvc2017/lib -lcommon -lexception_handler -lcrash_generation_client
}
