#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QStringList>

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
    void timeChanged();

protected:

private:
    Ui::MainWindow *ui;
    QDir parentDir;
    QStringList files;

};

#endif // MAINWINDOW_H
