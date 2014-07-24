#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QStringList>
#include <QTimer>
#include <map>
#include <string>
#include "imagecache.h"

class HPGVRender;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void open();
    void movie();
    void timeChanged(int);
    void updateInfo();

protected:

private:
    Ui::MainWindow *ui;
    QDir parentDir;
    QStringList files;
    ImageCache imageCache;
};

#endif // MAINWINDOW_H
