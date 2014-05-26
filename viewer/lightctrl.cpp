#include "lightctrl.h"
#include <QPaintEvent>
#include <QPainter>
#include <QRadialGradient>
#include <iostream>
#include <cmath>

const float LightCtrl::margin = 15.f;

LightCtrl::LightCtrl(QWidget *parent) :
    QWidget(parent), lightDir(0.f, 0.f, -1.f)
{
}

void LightCtrl::paintEvent(QPaintEvent *e)
{
    float w = e->rect().width();
    float h = e->rect().height();
    QPointF center(w / 2.f, h / 2.f);
    float r = w / 2.f - margin;

    float x = lightDir.x() * w / 2.f + center.x();
    float y = h - (lightDir.y() * h / 2.f + center.y());
    QPainter cursor(this);
    cursor.setRenderHint(QPainter::Antialiasing, true);
    QRadialGradient gradient(center, w / 2.f, QPointF(x, y));
    gradient.setColorAt(0, QColor::fromRgbF(1.0, 1.0, 1.0));
    gradient.setColorAt(1, QColor::fromRgbF(0.1, 0.1, 0.1));
    QBrush brush(gradient);
    cursor.setBrush(brush);
    cursor.drawEllipse(center, r, r);
//    QPen pen(QColor::fromRgbF(0.2, 0.2, 0.85), 0, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
//    cursor.setPen(pen);
//    cursor.drawEllipse(center, 0.7 * r, 0.7 * r);
}

void LightCtrl::mouseMoveEvent(QMouseEvent *e)
{
    float w = width();
    float h = height();
    QPointF center(w / 2.f, h / 2.f);
    QPointF pos = e->localPos();
    pos.setY(float(height()) - pos.y());
    float x = 2.f * (pos.x() - center.x()) / w;
    float y = 2.f * (pos.y() - center.y()) / h;
    QVector2D two(x, y);
    float twoDist = std::min(two.length(), 0.999f);
    two = two.normalized() * twoDist;
    float r = 1.f;
    float z = -sqrt(r*r - two.lengthSquared());
    QVector3D three(two.x(), two.y(), z);
    three.normalize();
    lightDir = three;
    repaint();
    emit lightDirChanged(lightDir);
}
