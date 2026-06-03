/****************************************************/
/* 文件: main_gui.cpp                                 */
/* TINY 编译器 Qt GUI 入口程序                       */
/****************************************************/

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("TINY 编译器 GUI");
    app.setApplicationVersion("1.0");

    MainWindow w;
    w.resize(1200, 800);
    w.show();

    return app.exec();
}
