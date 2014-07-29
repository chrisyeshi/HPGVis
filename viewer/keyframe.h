#ifndef __KEYFRAME_H__
#define __KEYFRAME_H__

#include <iostream>
#include <fstream>
#include <QPointF>
#include <QVector3D>
#include "TF.h"
#include "json-forwards.h"

class KeyFrame
{
public:
    KeyFrame();
    KeyFrame(const Json::Value& json);

    Json::Value toJSON() const;
    static KeyFrame interpolate(const KeyFrame& a, const KeyFrame& b, float r);

    int frame;
    int timestep;
    float zoom;
    QPointF focal;
    QVector3D light;
    mslib::TF tf;
};

#endif //__KEYFRAME_H__
