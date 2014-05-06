#include <QApplication>
#include <QGLFormat>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QGLFormat glFormat;
    glFormat.setVersion(3, 3);
    glFormat.setProfile(QGLFormat::CoreProfile);

    // Set the default GL format to OpenGL 3.2 Core
    QGLFormat::setDefaultFormat(glFormat);

    MainWindow w;
    w.show();

    return a.exec();
}
