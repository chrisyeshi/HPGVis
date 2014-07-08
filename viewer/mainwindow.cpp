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
    ui->timeSlider->setTracking(false);
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
    if (qfilename.isNull())
        return;
    int iImage = imageCache.open(qfilename);
    ui->timeSlider->setRange(0, imageCache.getImageCount() - 1);
    ui->timeSlider->setValue(iImage);
    if (!ui->timeSlider->hasTracking())
        ui->viewer->renderRAF(imageCache.getImage(iImage));
}

void MainWindow::movie()
{
    for (int iFile = 0; iFile < imageCache.getImageCount(); ++iFile)
    {
        ui->viewer->renderRAF(imageCache.getImage(iFile));
        ui->viewer->updateGL();
        ui->viewer->screenCapture();
    }
}

void MainWindow::timeChanged(int val)
{
    if(val < 0 || val >= imageCache.getImageCount())
        return;
    ui->viewer->renderRAF(imageCache.getImage(val));
}

//
//
// Protected Methods
//
//
