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
class KeyFrame;

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
    void makeKeyFrame();

protected:
    void setKeyFrame(const KeyFrame& key);

private slots:
    void on_tfHeader_toggled(bool checked);

    void on_lightHeader_toggled(bool checked);

    void on_infoHeader_toggled(bool checked);

    void on_animationHeader_toggled(bool checked);

    void on_advancedHeader_toggled(bool checked);

    void on_stepFunc_clicked();

private:
    Ui::MainWindow *ui;
    QDir parentDir;
    QStringList files;
    ImageCache imageCache;
};

#endif // MAINWINDOW_H
