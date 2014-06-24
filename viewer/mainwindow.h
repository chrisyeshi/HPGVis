#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QStringList>
#include <QTimer>
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

protected:

private:
    Ui::MainWindow *ui;
    QDir parentDir;
    QStringList files;
    ImageCache imageCache;
};

#endif // MAINWINDOW_H
