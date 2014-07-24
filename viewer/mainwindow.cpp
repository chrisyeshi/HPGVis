#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QShortcut>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/time.h>
#include "imageraf.h"
#include "markslider.h"
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QShortcut* shortcutClose = new QShortcut(tr("Ctrl+w"), this);
    connect(shortcutClose, SIGNAL(activated()), this, SLOT(close()));
    connect(ui->tf, SIGNAL(tfChanged(mslib::TF&)), ui->viewer, SLOT(tfChanged(mslib::TF&)));
    connect(ui->open, SIGNAL(clicked()), this, SLOT(open()));
    connect(ui->exit, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->timeSlider->getSlider(), SIGNAL(valueChanged(int)), this, SLOT(timeChanged(int)));
    connect(ui->light, SIGNAL(lightDirChanged(QVector3D)), ui->viewer, SLOT(lightDirChanged(QVector3D)));
    connect(ui->light, SIGNAL(lightDirChanged(QVector3D)), this, SLOT(updateInfo()));
    updateInfo();
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
    if (!ui->timeSlider->getSlider()->hasTracking())
        ui->viewer->renderRAF(imageCache.getImage(iImage));
    updateInfo();
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
    updateInfo();
}

void MainWindow::updateInfo()
{
    std::stringstream ss;
    ss << std::setprecision(3);
    // file
    std::string fn = imageCache.getCurrFilename().toUtf8().constData();
    ss << "Filename: " << fn << std::endl;
    int fs = imageCache.getCurrFilesize() / (1024 * 1024);
    ss << "Filesize: " << fs << "MB" << std::endl;
    // image
    int w = imageCache.getCurrImage().getWidth();
    int h = imageCache.getCurrImage().getHeight();
    int b = imageCache.getCurrImage().getNBins();
    ss << "Image Resolution: [" << w << ", " << h << "]" << std::endl;
    ss << "No. Bins: " << b << std::endl;
    // light
    QVector3D ld = ui->light->getLightDir();
    ss << "Light Direction: " << "[" << ld.x() << ", " << ld.y() << ", " << ld.z() << "]" << std::endl;
    ui->info->setPlainText(ss.str().c_str());
}

//
//
// Protected Methods
//
//
