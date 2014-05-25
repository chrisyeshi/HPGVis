#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include "imageraf.h"

#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->tf, SIGNAL(tfChanged(mslib::TF&)), ui->viewer, SLOT(tfChanged(mslib::TF&)));
    connect(ui->open, SIGNAL(clicked()), this, SLOT(open()));
    connect(ui->timeSlider, SIGNAL(sliderReleased()), this, SLOT(timeChanged()));
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
//    std::string filename = qfilename.toUtf8().constData();

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
    if (image.open(parentDir.absoluteFilePath(files.front()).toUtf8().constData())) {
        ui->viewer->renderRAF(image);
        ui->viewer->getFeatureMap(image);
    }

}

void MainWindow::timeChanged()
{
    int val = ui->timeSlider->value();
    if(val < 0 || val >= files.size())
        return;

    hpgv::ImageRAF image;
    if (image.open(parentDir.absoluteFilePath(files[val]).toUtf8().constData())) {
        ui->viewer->renderRAF(image);
        ui->viewer->getFeatureMap(image);
    }
}

//
//
// Protected Methods
//
//
