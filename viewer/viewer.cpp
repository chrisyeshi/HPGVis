#include "viewer.h"
#include <QMouseEvent>
#include <iostream>
#include <cassert>
#include "boost/shared_ptr.hpp"

Viewer::Viewer(QWidget *parent) :
    QGLWidget(parent),
    vboQuad(QOpenGLBuffer::VertexBuffer),
    texTf(QOpenGLTexture::Target1D),
    texAlpha(QOpenGLTexture::Target1D),
    texRaf0(NULL), texRaf1(NULL), texRaf2(NULL), texRaf3(NULL),
    texDep0(NULL), texDep1(NULL), texDep2(NULL), texDep3(NULL)
{
    setFocusPolicy(Qt::StrongFocus);
}

Viewer::~Viewer()
{
    if (texRaf0) delete texRaf0;
    if (texRaf1) delete texRaf1;
    if (texRaf2) delete texRaf2;
    if (texRaf3) delete texRaf3;
    if (texDep0) delete texDep0;
    if (texDep1) delete texDep1;
    if (texDep2) delete texDep2;
    if (texDep3) delete texDep3;
}

//
//
// The interface
//
//

void Viewer::renderRAF(const hpgv::ImageRAF &image)
{
    makeCurrent();

    imageRaf = image;
    int w = imageRaf.getWidth();
    int h = imageRaf.getHeight();
    boost::shared_ptr<float[]> temp(new float [w * h * 4]);
    // texRaf0
    for (int i = 0; i < w * h; ++i)
    {
        temp[4 * i + 0] = imageRaf.getRafs()[i + w * h * 0];
        temp[4 * i + 1] = imageRaf.getRafs()[i + w * h * 1];
        temp[4 * i + 2] = imageRaf.getRafs()[i + w * h * 2];
        temp[4 * i + 3] = imageRaf.getRafs()[i + w * h * 3];
    }
    updateRAF(texRaf0, temp.get(), w, h);
    // texRaf1
    for (int i = 0; i < w * h; ++i)
    {
        temp[4 * i + 0] = imageRaf.getRafs()[i + w * h * 4];
        temp[4 * i + 1] = imageRaf.getRafs()[i + w * h * 5];
        temp[4 * i + 2] = imageRaf.getRafs()[i + w * h * 6];
        temp[4 * i + 3] = imageRaf.getRafs()[i + w * h * 7];
    }
    updateRAF(texRaf1, temp.get(), w, h);
    // texRaf2
    for (int i = 0; i < w * h; ++i)
    {
        temp[4 * i + 0] = imageRaf.getRafs()[i + w * h * 8];
        temp[4 * i + 1] = imageRaf.getRafs()[i + w * h * 9];
        temp[4 * i + 2] = imageRaf.getRafs()[i + w * h * 10];
        temp[4 * i + 3] = imageRaf.getRafs()[i + w * h * 11];
    }
    updateRAF(texRaf2, temp.get(), w, h);
    // texRaf3
    for (int i = 0; i < w * h; ++i)
    {
        temp[4 * i + 0] = imageRaf.getRafs()[i + w * h * 12];
        temp[4 * i + 1] = imageRaf.getRafs()[i + w * h * 13];
        temp[4 * i + 2] = imageRaf.getRafs()[i + w * h * 14];
        temp[4 * i + 3] = imageRaf.getRafs()[i + w * h * 15];
    }
    updateRAF(texRaf3, temp.get(), w, h);
    // texAlpha
    assert(nBins == imageRaf.getNBins());
    std::vector<float> alphas = imageRaf.getAlphas();
    texAlpha.setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, alphas.data());
    // texDep0
    for (int i = 0; i < w * h; ++i)
    {
        temp[4 * i + 0] = imageRaf.getDepths()[i + w* h * 0];
        temp[4 * i + 1] = imageRaf.getDepths()[i + w* h * 1];
        temp[4 * i + 2] = imageRaf.getDepths()[i + w* h * 2];
        temp[4 * i + 3] = imageRaf.getDepths()[i + w* h * 3];
    }
    updateRAF(texDep0, temp.get(), w, h);
    // texDep1
    for (int i = 0; i < w * h; ++i)
    {
        temp[4 * i + 0] = imageRaf.getDepths()[i + w* h * 4];
        temp[4 * i + 1] = imageRaf.getDepths()[i + w* h * 5];
        temp[4 * i + 2] = imageRaf.getDepths()[i + w* h * 6];
        temp[4 * i + 3] = imageRaf.getDepths()[i + w* h * 7];
    }
    updateRAF(texDep1, temp.get(), w, h);
    // texDep2
    for (int i = 0; i < w * h; ++i)
    {
        temp[4 * i + 0] = imageRaf.getDepths()[i + w* h * 8];
        temp[4 * i + 1] = imageRaf.getDepths()[i + w* h * 9];
        temp[4 * i + 2] = imageRaf.getDepths()[i + w* h * 10];
        temp[4 * i + 3] = imageRaf.getDepths()[i + w* h * 11];
    }
    updateRAF(texDep2, temp.get(), w, h);
    // texDep3
    for (int i = 0; i < w * h; ++i)
    {
        temp[4 * i + 0] = imageRaf.getDepths()[i + w* h * 12];
        temp[4 * i + 1] = imageRaf.getDepths()[i + w* h * 13];
        temp[4 * i + 2] = imageRaf.getDepths()[i + w* h * 14];
        temp[4 * i + 3] = imageRaf.getDepths()[i + w* h * 15];
    }
    updateRAF(texDep3, temp.get(), w, h);

    updateGL();
}

//
//
// Public slots
//
//

void Viewer::tfChanged(mslib::TF& tf)
{
    assert(tf.resolution() == nBins);
    texTf.setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, tf.colorMap());

    updateGL();
}

void Viewer::snapshot(const std::string &filename)
{
    makeCurrent();
    QImage image = grabFrameBuffer();
    image.save(filename.c_str(), "PNG");
}

//
//
// Inherited from QGLWidget
//
//

void Viewer::initializeGL()
{
    initQuadVbo();
    updateProgram();
    initTF();
    qglClearColor(QColor(0, 0, 0, 0));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void Viewer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!texRaf0) return;

    progRaf.bind();
    vboQuad.bind();
    texTf.bind(0);
    texRaf0->bind(1);
    texRaf1->bind(2);
    texRaf2->bind(3);
    texRaf3->bind(4);
    texAlpha.bind(5);
    texDep0->bind(6);
    texDep1->bind(7);
    texDep2->bind(8);
    texDep3->bind(9);

    progRaf.setUniformValue("viewport", float(width()), float(height()));
    progRaf.enableAttributeArray("vertex");
    progRaf.setAttributeBuffer("vertex", GL_FLOAT, 0, nFloatsPerVertQuad);
    progRaf.enableAttributeArray("texCoord");
    progRaf.setAttributeBuffer("texCoord", GL_FLOAT, nBytesQuad(), nFloatsPerVertQuad);

    glDrawArrays(GL_QUADS, 0, nVertsQuad);

    texDep3->release();
    texDep2->release();
    texDep1->release();
    texDep0->release();
    texAlpha.release();
    texRaf3->release();
    texRaf2->release();
    texRaf1->release();
    texRaf0->release();
    texTf.release();
    vboQuad.release();
    progRaf.release();
}

void Viewer::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void Viewer::mouseMoveEvent(QMouseEvent *e)
{
    int wx = e->x();
    int wy = height() - e->y();
    int x = double(wx) / double(width()) * double(imageRaf.getWidth());
    int y = double(wy) / double(height()) * double(imageRaf.getHeight());
    if (x < 0 || x >= imageRaf.getWidth() || y < 0 || y >= imageRaf.getHeight())
        return;
    std::cout << "[" << x << "," << y << "]: value: ";
    for (int i = 0; i < imageRaf.getNBins(); ++i)
        std::cout << imageRaf.getRafs()[imageRaf.getWidth() * imageRaf.getHeight() * i + (y * imageRaf.getWidth() + x)] << ", ";
    std::cout << std::endl;
    std::cout << "[" << x << "," << y << "]: depth: ";
    for (int i = 0; i < imageRaf.getNBins(); ++i)
        std::cout << imageRaf.getDepths()[imageRaf.getWidth() * imageRaf.getHeight() * i + (y * imageRaf.getWidth() + x)] << ", ";
    std::cout << std::endl;
}

void Viewer::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
    case Qt::Key_F5:
        std::cout << "F5: update shader program." << std::endl;
        updateProgram();
        updateGL();
    }
}

//
//
// My functions
//
//

void Viewer::initQuadVbo()
{
    makeCurrent();
    static GLfloat vtx[] = { -1, -1,  1, -1,  1,  1, -1,  1,
                              0,  0,  1,  0,  1,  1,  0,  1};
    // QOpenGLBuffer
    vboQuad.create();
    vboQuad.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboQuad.bind();
    vboQuad.allocate(vtx, nBytesQuad() * 2);
    vboQuad.release();
}

void Viewer::updateProgram()
{
    progRaf.removeAllShaders();
    assert(progRaf.addShaderFromSourceFile(QOpenGLShader::Vertex, "../../viewer/shaders/raf.vert"));
    assert(progRaf.addShaderFromSourceFile(QOpenGLShader::Fragment, "../../viewer/shaders/raf.frag"));
    progRaf.link();
    progRaf.bind();
    progRaf.setUniformValue("nBins", nBins);
    progRaf.setUniformValue("mvp", mvp());
    progRaf.setUniformValue("tf", 0);
    progRaf.setUniformValue("raf0", 1);
    progRaf.setUniformValue("raf1", 2);
    progRaf.setUniformValue("raf2", 3);
    progRaf.setUniformValue("raf3", 4);
    progRaf.setUniformValue("rafa", 5);
    progRaf.setUniformValue("dep0", 6);
    progRaf.setUniformValue("dep1", 7);
    progRaf.setUniformValue("dep2", 8);
    progRaf.setUniformValue("dep3", 9);
    progRaf.release();
}

void Viewer::initTF()
{
    mslib::TF tf(nBins,nBins);
    // QOpenGLTexture
    texTf.setSize(tf.resolution());
    texTf.setFormat(QOpenGLTexture::RGBA32F);
    texTf.allocateStorage();
    texTf.setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, tf.colorMap());
    texTf.setWrapMode(QOpenGLTexture::ClampToEdge);
    texTf.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    // texAlpha
    texAlpha.setSize(tf.resolution());
    texAlpha.setFormat(QOpenGLTexture::R32F);
    texAlpha.allocateStorage();
    texAlpha.setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, tf.alphaArray());
    texAlpha.setWrapMode(QOpenGLTexture::ClampToEdge);
    texAlpha.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
}

void Viewer::updateRAF(QOpenGLTexture*& tex, float* data, int width, int height)
{
    // clean up
    if (tex)
    {
        assert(!tex->isBound());
        delete tex; tex = NULL;
    }
    // new texture
    tex = new QOpenGLTexture(QOpenGLTexture::Target2D);
    tex->setSize(width, height);
    tex->setFormat(QOpenGLTexture::RGBA32F);
    tex->allocateStorage();
    tex->setWrapMode(QOpenGLTexture::ClampToEdge);
    tex->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    tex->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, data);
}
