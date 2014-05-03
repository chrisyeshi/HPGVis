#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include "imageraf.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->tf, SIGNAL(tfChanged(mslib::TF&)), ui->viewer, SLOT(tfChanged(mslib::TF&)));
    connect(ui->open, SIGNAL(clicked()), this, SLOT(open()));
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
    std::string filename = qfilename.toUtf8().constData();
    hpgv::ImageRAF image;
    if (image.open(filename))
        ui->viewer->renderRAF(image);
}

//
//
// Protected Methods
//
//
