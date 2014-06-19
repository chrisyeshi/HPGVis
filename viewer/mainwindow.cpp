#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <iostream>
#include <sys/time.h>
#include "imageraf.h"
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->tf, SIGNAL(tfChanged(mslib::TF&)), ui->viewer, SLOT(tfChanged(mslib::TF&)));
    connect(ui->open, SIGNAL(clicked()), this, SLOT(open()));
    connect(ui->movie, SIGNAL(clicked()), this, SLOT(movie()));
    connect(ui->timeSlider, SIGNAL(valueChanged(int)), this, SLOT(timeChanged(int)));
    connect(ui->light, SIGNAL(lightDirChanged(QVector3D)), ui->viewer, SLOT(lightDirChanged(QVector3D)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

//
//
// Public Slots
//
//

void MainWindow::open()
{
    QString qfilename = QFileDialog::getOpenFileName(this,
            tr("Open RAF file"), QString(), tr("Explorable Images (*.raf)"));

    QFileInfo qfi(qfilename);

    parentDir = qfi.absoluteDir();
    files = parentDir.entryList(QStringList(tr("*.raf")));

    if(files.empty())
    {
        cout << "No files Available :(" << endl;
        return;
    }

    ui->timeSlider->setRange(0, files.size() - 1);

    hpgv::ImageRAF image;
    if (image.open(qfilename.toUtf8().constData()))
    {
        ui->viewer->renderRAF(image);
    }

}

void MainWindow::movie()
{
    for (int iFile = 0; iFile < files.size(); ++iFile)
    {
        hpgv::ImageRAF image;
        bool isOpen = image.open(parentDir.absoluteFilePath(files[iFile]).toUtf8().constData());
        if (isOpen)
        {
            ui->viewer->renderRAF(image);
            ui->viewer->updateGL();
            ui->viewer->screenCapture();
        }
    }
}

void MainWindow::timeChanged(int val)
{
//    int val = ui->timeSlider->value();
    if(val < 0 || val >= files.size())
        return;

    hpgv::ImageRAF image;
    timeval start; gettimeofday(&start, NULL);
    bool isOpen = image.open(parentDir.absoluteFilePath(files[val]).toUtf8().constData());
    timeval end; gettimeofday(&end, NULL);
    double time_file = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    std::cout << "Time: File:    " << time_file << " ms" << std::endl;
    if (isOpen)
    {
        ui->viewer->renderRAF(image);
    }
}

//
//
// Protected Methods
//
//
