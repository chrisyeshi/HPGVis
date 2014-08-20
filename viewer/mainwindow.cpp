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
#include "json.h"
#include "keyframe.h"
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QShortcut* shortcutClose = new QShortcut(tr("Ctrl+w"), this);
    connect(shortcutClose, SIGNAL(activated()), this, SLOT(close()));
    connect(ui->tf, SIGNAL(tfChanged(const mslib::TF&)), ui->viewer, SLOT(tfChanged(const mslib::TF&)));
    connect(ui->open, SIGNAL(clicked()), this, SLOT(open()));
    connect(ui->exit, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->timeSlider->getSlider(), SIGNAL(valueChanged(int)), this, SLOT(timeChanged(int)));
    connect(ui->light, SIGNAL(lightDirChanged(QVector3D)), ui->viewer, SLOT(lightDirChanged(QVector3D)));
    connect(ui->light, SIGNAL(lightDirChanged(QVector3D)), this, SLOT(updateInfo()));
    connect(ui->viewer, SIGNAL(viewChanged()), this, SLOT(updateInfo()));
    QMenu* more = new QMenu(this);
    more->addAction(ui->actionMovie);
    more->addAction(ui->actionKeyFrame);
    ui->more->setMenu(more);
    connect(ui->actionMovie, SIGNAL(triggered()), this, SLOT(movie()));
    connect(ui->actionKeyFrame, SIGNAL(triggered()), this, SLOT(makeKeyFrame()));
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
//    for (int iFile = 0; iFile < imageCache.getImageCount(); ++iFile)
//    {
//        ui->viewer->renderRAF(imageCache.getImage(iFile));
//        ui->viewer->updateGL();
//        ui->viewer->screenCapture();
//    }
    QString filename = QFileDialog::getOpenFileName(this, tr("Script File"), ".", tr("JSON Files (*.json)"));
    std::ifstream fin(filename.toUtf8().constData());
    Json::Value root;
    Json::Reader reader;
    bool parsed = reader.parse(fin, root);
    if (!parsed) return;
    // read key frames
    std::vector<KeyFrame> keys(root.size());
    for (unsigned int iFrame = 0; iFrame < root.size(); ++iFrame)
    {
        keys[iFrame] = KeyFrame(root[iFrame]);
    }
    // loop
    int frame = 1;
    for (unsigned int iFrame = 0; iFrame < keys.size() - 1; ++iFrame)
    {
        KeyFrame a = keys[iFrame];
        KeyFrame b = keys[iFrame + 1];
        int fa = a.frame;
        int fb = b.frame;
        for (int i = fa; i < fb; ++i)
        {
            float r = float(i - fa) / float(fb - fa);
            KeyFrame c = KeyFrame::interpolate(a, b, r);
            setKeyFrame(c);
            ui->viewer->screenCapture(frame++);
        }
    }
    setKeyFrame(keys[keys.size() - 1]);
    ui->viewer->screenCapture(frame++);
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
    // view
    float zoom = ui->viewer->getZoom();
    QPointF focal = ui->viewer->getFocal();
    ss << "Zoom: " << zoom << std::endl;
    ss << "Focal: [" << focal.x() << ", " << focal.y() << "]" << std::endl;
    // window size
    ss << "Window: [" << ui->viewer->width() << ", " << ui->viewer->height() << "]" << std::endl;

    ui->info->setPlainText(ss.str().c_str());
}

void MainWindow::makeKeyFrame()
{
    KeyFrame key;
    key.frame = 0;
    key.timestep = imageCache.getCurrIdx();
    key.zoom = ui->viewer->getZoom();
    key.focal = ui->viewer->getFocal();
    key.light = ui->light->getLightDir();
    key.tf = ui->tf->getTF();
    // print
    Json::Value json = key.toJSON();
    Json::StyledWriter writer;
    std::string out = writer.write(json);
    std::cout << out << std::endl;
}

void MainWindow::setKeyFrame(const KeyFrame &key)
{
    ui->viewer->renderRAF(imageCache.getImage(key.timestep));
    ui->viewer->setView(key.zoom, key.focal);
    ui->viewer->lightDirChanged(key.light);
    ui->viewer->tfChanged(key.tf);
}

//
//
// Protected Methods
//
//
