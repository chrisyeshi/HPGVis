#ifndef VIEWER_H
#define VIEWER_H

#include <QGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include "TF.h"
#include "imageraf.h"
#include "featuretracker.h"

class Viewer : public QGLWidget
{
    Q_OBJECT
public:
    explicit Viewer(QWidget *parent = 0);
    ~Viewer();

    //
    //
    // The interface
    //
    //
    void renderRAF(const hpgv::ImageRAF& image);
    void snapshot(const std::string& filename);
    void getFeatureMap(const hpgv::ImageRAF& image);  // fet

signals:

public slots:
    //
    //
    // Public slots
    //
    //
    void tfChanged(mslib::TF& tf);

protected:
    //
    //
    // Inherited from QGLWidget
    //
    //
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void wheelEvent(QWheelEvent* e);

    //
    //
    // My functions
    //
    //
    void EnableHighlighting(float x, float y);
    void DisableHighlighting();
    void initQuadVbo();
    void updateProgram();
    void updateShaderMVP();
    void updateVAO();
    void initTF();
    void updateTexRAF();
    void updateTexNormal();

private:
    //
    //
    // Static const variables
    //
    //
    static const int nVertsQuad = 4;
    static const int nFloatsPerVertQuad = 2;
    static const int nBins = 16;
    static const int nAlphaBins = 17;

    //
    //
    // Variables
    //
    //
    QMatrix4x4 matModel, matView, matProj;
    QOpenGLBuffer vboQuad;
    QOpenGLTexture texTf, texAlpha;
    QOpenGLTexture *texArrRaf, *texArrDep, *texFeature;
    QOpenGLShaderProgram progRaf;
    QOpenGLVertexArrayObject vao;
    float zoomFactor;
    QPointF focal;
    QPointF cursorPrev;
    QOpenGLTexture* texArrNml;

    hpgv::ImageRAF imageRaf;
    bool HighlightFeatures;
    int SelectedFeature;

    std::vector<float> mask;

    FeatureTracker featureTracker;
    int nFeatures;
    //
    //
    // Helper functions
    //
    //
    int nBytesQuad() const { return nVertsQuad * nFloatsPerVertQuad * sizeof(GLfloat); }
    QMatrix4x4 mvp() const { return matProj * matView * matModel; }
};

#endif // VIEWER_H
