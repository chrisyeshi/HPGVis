#ifndef LIGHTCTRL_H
#define LIGHTCTRL_H

#include <QWidget>
#include <QVector3D>

class LightCtrl : public QWidget
{
    Q_OBJECT
public:
    explicit LightCtrl(QWidget *parent = 0);

    QVector3D getLightDir() const { return lightDir; }

signals:
    void lightDirChanged(QVector3D);

public slots:

protected:
    virtual void paintEvent(QPaintEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);

private:
    static const float margin;

    QVector3D lightDir;
};

#endif // LIGHTCTRL_H
