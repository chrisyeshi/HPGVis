#include "viewer.h"
#include <QMouseEvent>
#include <QShortcut>
#include <iostream>
#include <sstream>
#include <cassert>
#include <sys/time.h>
#include "boost/shared_ptr.hpp"

using namespace std;

Viewer::Viewer(QWidget *parent) :
    QGLWidget(parent),
    vboQuad(QOpenGLBuffer::VertexBuffer),
    texTf(QOpenGLTexture::Target1D),
    texAlpha(QOpenGLTexture::Target1D),
    texArrRaf(NULL), texArrDep(NULL),
    zoomFactor(1.f), texArrNml(NULL),
    texFeature(NULL), HighlightFeatures(false), SelectedFeature(0)
{
    std::cout << "OpenGL Version: " << this->context()->format().majorVersion() << "." << this->context()->format().minorVersion() << std::endl;
    setFocusPolicy(Qt::StrongFocus);
    QShortcut* scSnapshot = new QShortcut(QKeySequence(tr("s")), this);
    connect(scSnapshot, SIGNAL(activated()), this, SLOT(screenCapture()));
}

Viewer::~Viewer()
{
    if (texArrRaf) delete texArrRaf;
    if (texArrDep) delete texArrDep;
    if (texFeature) delete texFeature;
}

//
//
// The interface
//
//

void Viewer::renderRAF(const hpgv::ImageRAF *image)
{
    makeCurrent();
    imageRaf = image;
    std::cout << "Image: Width: " << imageRaf->getWidth() << " Height: " << imageRaf->getHeight() << std::endl;
    updateTexRAF();
    updateTexNormal();
    updateFeatureMap();
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

void Viewer::lightDirChanged(QVector3D lightDir)
{
    makeCurrent();
    progRaf.bind();
    progRaf.setUniformValue("lightDir", lightDir);
    progRaf.release();

    updateGL();
}

void Viewer::screenCapture()
{
    static int i = 1;
    std::stringstream ss;
    ss << "snapshot_" << i++ << ".png";
    this->snapshot(ss.str());
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
    initializeOpenGLFunctions();
    int maxLayer; glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &maxLayer);
    std::cout << "GL_MAX_FRAMEBUFFER_LAYERS: " << maxLayer << std::endl;
    int maxColorAttach; glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttach);
    std::cout << "GL_MAX_COLOR_ATTACHMENTS: " << maxColorAttach << std::endl;
    initQuadVbo();
    updateProgram();
    updateVAO();
    initTF();
    initTexNormalTools();
    qglClearColor(QColor(0, 0, 0, 0));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void Viewer::paintGL()
{
//    timeval start; gettimeofday(&start, NULL);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!texArrRaf) return;

    progRaf.bind();
    vao.bind();
    texTf.bind(0);
    texAlpha.bind(1);
    texArrRaf->bind(2);
    texArrNml->bind(3);
    texArrDep->bind(4);
    if (texFeature) texFeature->bind(5);

    progRaf.setUniformValue("invVP", 1.f / float(imageRaf->getWidth()), 1.f / float(imageRaf->getHeight()));
    progRaf.setUniformValue("selectedID", float(SelectedFeature)/nFeatures);
    progRaf.setUniformValue("featureHighlight", int(HighlightFeatures ? 1 : 0));

    glDrawArrays(GL_TRIANGLE_FAN, 0, nVertsQuad);

    if (texFeature) texFeature->release();
    texArrDep->release();
    texArrNml->release();
    texArrRaf->release();
    texAlpha.release();
    texTf.release();
    vao.release();
    progRaf.release();

//    timeval end; gettimeofday(&end, NULL);
//    double time_normal = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
//    std::cout << "Time: Paint:  " << time_normal << " ms" << std::endl;
}

void Viewer::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    updateShaderMVP();
}

void Viewer::EnableHighlighting(float x, float y)
{
    cout << "Feature Highlighting Enabled!" << endl;
    HighlightFeatures = true;

    int idx = int(x) + int(y) * imageRaf->getWidth();
    SelectedFeature = int(mask[idx] * nFeatures + 0.5);

    std::cout << "SelectedFeature: " << SelectedFeature << std::endl;

    update();
}

void Viewer::DisableHighlighting()
{
    cout << "Feature Highlighting Disabled!" << endl;
    HighlightFeatures = false;
    update();
}

void Viewer::mousePressEvent(QMouseEvent *e)
{
    cursorPrev = e->localPos();
}

void Viewer::mouseReleaseEvent(QMouseEvent *e)
{
    QPointF moved = e->localPos() - cursorPrev;
    if (moved.manhattanLength() < 1)
    { // It's a click
        if (HighlightFeatures && e->button() == Qt::LeftButton)
        {
            float wx = float(e->x()) / float(width());
            float wy = float(height() - e->y()) / float(height());
            QVector4D screen(2.f * (wx - 0.5f), 2.f * (wy - 0.5f), 0.f, 1.f);
            QVector4D world = mvp().inverted() * screen;
            int x = (world.x() + 1.0) / 2.f * imageRaf->getWidth();
            int y = (world.y() + 1.0) / 2.f * imageRaf->getHeight();
            if (x < 0 || x >= imageRaf->getWidth() || y < 0 || y >= imageRaf->getHeight())
                return;
            EnableHighlighting(x, y);
        }
    }
}

void Viewer::mouseMoveEvent(QMouseEvent *e)
{
    // camera movement
    if (e->buttons() & Qt::LeftButton)
    {
        QPointF dir = e->localPos() - cursorPrev;
        dir.rx() = dir.x() / float(width()) * zoomFactor * 2.f;
        dir.ry() = -dir.y() / float(height()) * zoomFactor * 2.f;
        focal.rx() -= dir.x();
        focal.ry() -= dir.y();
        cursorPrev = e->localPos();
        updateShaderMVP();
        updateGL();
    } else
    {
        int wx = e->x();
        int wy = height() - e->y();
        int x = double(wx) / double(width()) * double(imageRaf->getWidth());
        int y = double(wy) / double(height()) * double(imageRaf->getHeight());
        if (x < 0 || x >= imageRaf->getWidth() || y < 0 || y >= imageRaf->getHeight())
            return;
        std::cout << "[" << x << "," << y << "]: value: ";
        for (unsigned int i = 0; i < imageRaf->getNBins(); ++i)
            std::cout << imageRaf->getRafs()[imageRaf->getWidth() * imageRaf->getHeight() * i + (y * imageRaf->getWidth() + x)] << ", ";
        std::cout << std::endl;
        std::cout << "[" << x << "," << y << "]: depth: ";
        for (unsigned int i = 0; i < imageRaf->getNBins(); ++i)
            std::cout << imageRaf->getDepths()[imageRaf->getWidth() * imageRaf->getHeight() * i + (y * imageRaf->getWidth() + x)] << ", ";
        std::cout << std::endl;
    }
}

void Viewer::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
    case Qt::Key_F5:
        std::cout << "F5: update shader program." << std::endl;
        updateProgram();
        updateVAO();
        updateGL();
        break;
    case Qt::Key_Right:
        SelectedFeature++;
        if (SelectedFeature > nFeatures)
            SelectedFeature = 0;
        std::cout << "FID: " << SelectedFeature << std::endl;
        updateGL();
        break;
    case Qt::Key_Left:
        SelectedFeature--;
        if (SelectedFeature < 0)
            SelectedFeature = nFeatures;
        std::cout << "FID: " << SelectedFeature << std::endl;
        updateGL();
        break;
    case Qt::Key_F:
        if (HighlightFeatures)
            DisableHighlighting();
        else
            EnableHighlighting(imageRaf->getWidth() / 2, imageRaf->getHeight() / 2);
        break;
    }

}

void Viewer::wheelEvent(QWheelEvent *e)
{
    int numSteps = e->angleDelta().y() / 8 / 15;
    zoomFactor *= 1.f - float(numSteps) / 15.f;
    zoomFactor = std::min(1.f, zoomFactor);
    updateShaderMVP();
    updateGL();
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
    progRaf.addShaderFromSourceFile(QOpenGLShader::Vertex,   "../../viewer/shaders/isoraf.vert");
    progRaf.addShaderFromSourceFile(QOpenGLShader::Fragment, "../../viewer/shaders/isoraf.frag");
    progRaf.link();
    progRaf.bind();
    progRaf.setUniformValue("nBins", nBins);
    progRaf.setUniformValue("mvp", mvp());
    progRaf.setUniformValue("lightDir", QVector3D(0.0, 0.0, -1.0));
    progRaf.setUniformValue("tf", 0);
    progRaf.setUniformValue("rafa", 1);
    progRaf.setUniformValue("rafarr", 2);
    progRaf.setUniformValue("nmlarr", 3);
    progRaf.setUniformValue("deparr", 4);
    progRaf.setUniformValue("featureID", 5);
    progRaf.release();

    initTexNormalTools();
}

void Viewer::updateShaderMVP()
{
    matView.setToIdentity();
    float w = float(width()) / float(height()) * zoomFactor;
    float h = 1.f * zoomFactor;
    matView.ortho(focal.x() - w, focal.x() + w, focal.y() - h, focal.y() + h, -1.f, 1.f);
    if (!progRaf.isLinked())
        return;
    progRaf.bind();
    progRaf.setUniformValue("mvp", mvp());
    progRaf.release();
}

void Viewer::updateVAO()
{
    if (!vao.isCreated())
        vao.create();
    vao.bind();
    vboQuad.bind();
    progRaf.enableAttributeArray("vertex");
    progRaf.setAttributeBuffer("vertex", GL_FLOAT, 0, nFloatsPerVertQuad);
    progRaf.enableAttributeArray("texCoord");
    progRaf.setAttributeBuffer("texCoord", GL_FLOAT, nBytesQuad(), nFloatsPerVertQuad);
    vao.release();
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

void Viewer::updateTexRAF()
{
    // RAF
    // clean up
    if (texArrRaf)
    {
        assert(!texArrRaf->isBound());
        delete texArrRaf; texArrRaf = NULL;
    }
    // new texture array
    texArrRaf = new QOpenGLTexture(QOpenGLTexture::Target2DArray);
    texArrRaf->setSize(imageRaf->getWidth(), imageRaf->getHeight());
    texArrRaf->setLayers(imageRaf->getNBins());
    texArrRaf->setFormat(QOpenGLTexture::R32F);
    texArrRaf->allocateStorage();
    texArrRaf->setWrapMode(QOpenGLTexture::ClampToEdge);
    texArrRaf->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    for (unsigned int layer = 0; layer < imageRaf->getNBins(); ++layer)
    {
        texArrRaf->setData(0, layer, QOpenGLTexture::Red, QOpenGLTexture::Float32,
                &(imageRaf->getRafs().get()[layer * imageRaf->getWidth() * imageRaf->getHeight()]));
    }

    // Depth
    if (texArrDep)
    {
        assert(!texArrDep->isBound());
        delete texArrDep; texArrDep = NULL;
    }
    // new texture array
    texArrDep = new QOpenGLTexture(QOpenGLTexture::Target2DArray);
    texArrDep->setSize(imageRaf->getWidth(), imageRaf->getHeight());
    texArrDep->setLayers(imageRaf->getNBins());
    texArrDep->setFormat(QOpenGLTexture::R32F);
    texArrDep->allocateStorage();
    texArrDep->setWrapMode(QOpenGLTexture::ClampToEdge);
    texArrDep->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    for (unsigned int layer = 0; layer < imageRaf->getNBins(); ++layer)
    {
        texArrDep->setData(0, layer, QOpenGLTexture::Red, QOpenGLTexture::Float32,
                &(imageRaf->getDepths().get()[layer * imageRaf->getWidth() * imageRaf->getHeight()]));
    }
}

void Viewer::initTexNormalTools()
{
    // program
    progArrNml.removeAllShaders();
    progArrNml.addShaderFromSourceFile(QOpenGLShader::Vertex,   "../../viewer/shaders/normal.vert");
    progArrNml.addShaderFromSourceFile(QOpenGLShader::Fragment, "../../viewer/shaders/normal.frag");
    progArrNml.link();
    progArrNml.bind();
    progArrNml.setUniformValue("deparr", 0);
    progArrNml.release();
    // vao
    if (!vaoArrNml.isCreated())
        vaoArrNml.create();
    vaoArrNml.bind();
    vboQuad.bind();
    progArrNml.enableAttributeArray("vertex");
    progArrNml.setAttributeBuffer("vertex", GL_FLOAT, 0, nFloatsPerVertQuad);
    progArrNml.enableAttributeArray("texCoord");
    progArrNml.setAttributeBuffer("texCoord", GL_FLOAT, nBytesQuad(), nFloatsPerVertQuad);
    vaoArrNml.release();
}

void Viewer::updateTexNormal()
{
    // if the dimension is changed, delete the texture
    if (texArrNml && (texArrNml->width() != imageRaf->getWidth() || texArrNml->height() != imageRaf->getHeight() || texArrNml->layers() != imageRaf->getNBins()))
    {
        delete texArrNml;
        texArrNml = NULL;
        glDeleteFramebuffers(fboArrNml.size(), fboArrNml.data());
        fboArrNml.clear();
    }
    // initialize the texture
    if (!texArrNml)
        initTexNormal();

//    timeval start; gettimeofday(&start, NULL);

    // calculate normal in GPU -- Yeah!~ I will keep the equivalent CPU code in comment below. We shall compare the performance difference.
    progArrNml.bind();
    progArrNml.setUniformValue("nBins", nBins);
    progArrNml.setUniformValue("vp", imageRaf->getWidth(), imageRaf->getHeight());
    progArrNml.setUniformValue("invVP", 1.f / float(imageRaf->getWidth()), 1.f / float(imageRaf->getHeight()));
    vaoArrNml.bind();
    texArrDep->bind(0);

    glFinish();
//    timeval init; gettimeofday(&init, NULL);
//    double time_init = (init.tv_sec - start.tv_sec) * 1000.0 + (init.tv_usec - start.tv_usec) / 1000.0;
//    std::cout << "Time: Normal: Init: " << time_init << " ms" << std::endl;
    // viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, imageRaf->getWidth(), imageRaf->getHeight());
    // draw
    for (int layer = 0; layer < texArrNml->layers(); ++layer)
    {
        progArrNml.setUniformValue("layer", layer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboArrNml[layer]);

        glDrawArrays(GL_TRIANGLE_FAN, 0, nVertsQuad);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

//    timeval draw; gettimeofday(&draw, NULL);
//    double time_draw = (draw.tv_sec - init.tv_sec) * 1000.0 + (draw.tv_usec - init.tv_usec) / 1000.0;
//    std::cout << "Time: Normal: Draw " << time_draw << " ms" << std::endl;

    texArrDep->release();
    vaoArrNml.release();
    progArrNml.release();

//    timeval end; gettimeofday(&end, NULL);
//    double time_normal = (end.tv_sec - draw.tv_sec) * 1000.0 + (end.tv_usec - draw.tv_usec) / 1000.0;
//    std::cout << "Time: Normal: Clean: " << time_normal << " ms" << std::endl;

/*
//    // calculate normal in CPU -- this is going to be painful
//    timeval start; gettimeofday(&start, NULL);
//    for (int layer = 0; layer < imageRaf->getNBins(); ++layer)
//    {
//        int w = imageRaf->getWidth();
//        int h = imageRaf->getHeight();
//        float* normals = new float [3 * w * h];
//        float* depths = &(imageRaf->getDepths().get()[layer * w * h]);
//#pragma omp parallel for
//        for (int y = 0; y < w; ++y)
//#pragma omp parallel for
//        for (int x = 0; x < h; ++x)
//        {
//            // 3 2 1
//            // 4 C 0
//            // 5 6 7
//            float tx = (float(x) + 0.5f) / float(w);
//            float ty = (float(y) + 0.5f) / float(h);
//            float dx = 1.f / float(w);
//            float dy = 1.f / float(h);
//            QPoint pos[8];
//            pos[0] = QPoint(x+1,y+0);
//            pos[1] = QPoint(x+1,y+1);
//            pos[2] = QPoint(x+0,y+1);
//            pos[3] = QPoint(x-1,y+1);
//            pos[4] = QPoint(x-1,y+0);
//            pos[5] = QPoint(x-1,y-1);
//            pos[6] = QPoint(x+0,y-1);
//            pos[7] = QPoint(x+1,y-1);
//            for (int i = 0; i < 8; ++i)
//            {
//                pos[i].rx() = std::min(pos[i].x(), w-1);
//                pos[i].rx() = std::max(pos[i].x(), 0);
//                pos[i].ry() = std::min(pos[i].y(), h-1);
//                pos[i].ry() = std::max(pos[i].y(), 0);
//            }
//            QVector3D coordsCtr(tx, ty, depths[w*y+x]);
//            QVector3D coords[8];
//            coords[0] = QVector3D(tx+dx, ty   , depths[w*pos[0].y()+pos[0].x()]);
//            coords[1] = QVector3D(tx+dx, ty+dy, depths[w*pos[1].y()+pos[1].x()]);
//            coords[2] = QVector3D(tx   , ty+dy, depths[w*pos[2].y()+pos[2].x()]);
//            coords[3] = QVector3D(tx-dx, ty+dy, depths[w*pos[3].y()+pos[3].x()]);
//            coords[4] = QVector3D(tx-dx, ty   , depths[w*pos[4].y()+pos[4].x()]);
//            coords[5] = QVector3D(tx-dx, ty-dy, depths[w*pos[5].y()+pos[5].x()]);
//            coords[6] = QVector3D(tx   , ty-dy, depths[w*pos[6].y()+pos[6].x()]);
//            coords[7] = QVector3D(tx+dx, ty-dy, depths[w*pos[7].y()+pos[7].x()]);
//            QVector3D dirs[8];
//            for (int i = 0; i < 8; ++i)
//                dirs[i] = coords[i] - coordsCtr;
//            QVector3D sum;
//            for (int i = 0; i < 7; ++i)
//            {
//                float delta = 0.001f;
//                QVector3D normal;
//                if (dirs[i].z() > delta || dirs[i+1].z() > delta)
//                    normal = QVector3D(0.f,0.f,0.f);
//                else
//                    normal = QVector3D::crossProduct(dirs[i],dirs[i+1]);
//                sum += normal;
//            }
//            QVector3D theNormal = sum.normalized();
//            normals[3 * (w * y + x) + 0] = (theNormal.x() + 1.f) * 0.5f;
//            normals[3 * (w * y + x) + 1] = (theNormal.y() + 1.f) * 0.5f;
//            normals[3 * (w * y + x) + 2] = (theNormal.z() + 1.f) * 0.5f;
//        }
//        texArrNml->setData(0, layer,
//                QOpenGLTexture::RGB, QOpenGLTexture::Float32,
//                normals);
//        delete [] normals;
//    }
//    timeval end; gettimeofday(&end, NULL);
//    double time_normal = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
//    std::cout << "Time: Normal:  " << time_normal << " ms" << std::endl;
*/
}

void Viewer::initTexNormal()
{
    // texture
    texArrNml = new QOpenGLTexture(QOpenGLTexture::Target2DArray);
    texArrNml->setSize(imageRaf->getWidth(), imageRaf->getHeight());
    texArrNml->setLayers(imageRaf->getNBins());
    texArrNml->setFormat(QOpenGLTexture::RGB32F);
    texArrNml->allocateStorage();
    texArrNml->setWrapMode(QOpenGLTexture::ClampToEdge);
    texArrNml->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    // fbo
    fboArrNml.resize(imageRaf->getNBins());
    glGenFramebuffers(imageRaf->getNBins(), fboArrNml.data());
    for (int iFbo = 0; iFbo < fboArrNml.size(); ++iFbo)
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboArrNml[iFbo]);
        glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texArrNml->textureId(), 0, iFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
}

void Viewer::updateFeatureMap()
{
//    timeval start; gettimeofday(&start, NULL);

    int w = imageRaf->getWidth();
    int h = imageRaf->getHeight();
    featureTracker.setDim(w, h);
    // depths
    std::vector<float*> deps;
    deps.push_back(imageRaf->getDepths().get());
    for (int i = 1; i < nBins; ++i)
        deps.push_back(deps.front() + w*h*i);
    // mask
    mask = std::vector<float>(w*h, 0.f);
    featureTracker.track(deps[8], mask.data());
    nFeatures = featureTracker.getNumFeatures();
    for (auto& p : mask)
        p /= float(nFeatures);
//    std::cout << "nFeatures: " << nFeatures << std::endl;
    // clean up
    if (texFeature)
    {
        assert(!texFeature->isBound());
        delete texFeature; texFeature = NULL;
    }
    // new texture
    texFeature = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texFeature->setSize(w, h);
    texFeature->setFormat(QOpenGLTexture::R32F);
    texFeature->allocateStorage();
    texFeature->setWrapMode(QOpenGLTexture::ClampToEdge);
    texFeature->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    texFeature->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, mask.data());

//    timeval end; gettimeofday(&end, NULL);
//    double time_normal = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
//    std::cout << "Time: Feature: " << time_normal << " ms" << std::endl;
}
