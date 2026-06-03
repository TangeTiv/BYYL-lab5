# Tiny Compiler GUI - Qt 项目文件
# 使用 Qt 6.9.1 (mingw_64)

QT       += core gui widgets
CONFIG   += c++14
TARGET   = TinyCompilerGUI
TEMPLATE = app

# 包含编译器源代码目录
INCLUDEPATH += ..

SOURCES += \
    main_gui.cpp \
    mainwindow.cpp \
    ../compiler_api.cpp \
    ../globals.cpp \
    ../scanner.cpp \
    ../parser.cpp \
    ../analyzer.cpp \
    ../symtab.cpp \
    ../cgen.cpp \
    ../code.cpp \
    ../util.cpp

HEADERS += \
    mainwindow.h \
    ../compiler_api.h \
    ../globals.h \
    ../scanner.h \
    ../parser.h \
    ../analyzer.h \
    ../symtab.h \
    ../cgen.h \
    ../code.h \
    ../util.h

# Windows 下隐藏控制台窗口
win32:CONFIG(release, debug|release): DEFINES += QT_NO_DEBUG_OUTPUT
