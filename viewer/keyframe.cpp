#include "keyframe.h"
#include "json.h"

KeyFrame::KeyFrame() : tf(16, 16)
{

}

KeyFrame::KeyFrame(const Json::Value &json)
{
    frame = json["frame"].asInt();
    timestep = json["timestep"].asInt();
    zoom = json["zoom"].asFloat();
    focal.rx() = json["focal"][0].asFloat();
    focal.ry() = json["focal"][1].asFloat();
    light.setX(json["light"][0].asFloat());
    light.setY(json["light"][1].asFloat());
    light.setZ(json["light"][2].asFloat());
    for (int i = 0; i < 16; ++i)
    {
        tf.colorMap()[4 * i + 0] = json["tf"][i][0].asFloat();
        tf.colorMap()[4 * i + 1] = json["tf"][i][1].asFloat();
        tf.colorMap()[4 * i + 2] = json["tf"][i][2].asFloat();
        tf.colorMap()[4 * i + 3] = json["tf"][i][3].asFloat();
    }
}

Json::Value KeyFrame::toJSON() const
{
    Json::Value json;
    json["frame"] = frame;
    json["timestep"] = timestep;
    json["zoom"] = zoom;
    json["focal"][0] = focal.x();
    json["focal"][1] = focal.y();
    json["light"][0] = light.x();
    json["light"][1] = light.y();
    json["light"][2] = light.z();
    for (int i = 0; i < 16; ++i)
    {
        json["tf"][i][0] = tf.colorMap()[4 * i + 0];
        json["tf"][i][1] = tf.colorMap()[4 * i + 1];
        json["tf"][i][2] = tf.colorMap()[4 * i + 2];
        json["tf"][i][3] = tf.colorMap()[4 * i + 3];
    }
    return json;
}

KeyFrame KeyFrame::interpolate(const KeyFrame &a, const KeyFrame &b, float r)
{
    KeyFrame c;
    c.frame = float(a.frame) * (1.f - r) + float(b.frame) * r + 0.5f;
    c.timestep = float(a.timestep) * (1.f - r) + float(b.timestep) * r + 0.5f;
    c.zoom = a.zoom * (1.f - r) + b.zoom * r;
    c.focal.rx() = a.focal.x() * (1.f - r) + b.focal.x() * r;
    c.focal.ry() = a.focal.y() * (1.f - r) + b.focal.y() * r;
    c.light.setX(a.light.x() * (1.f - r) + b.light.x() * r);
    c.light.setY(a.light.y() * (1.f - r) + b.light.y() * r);
    c.light.setZ(a.light.z() * (1.f - r) + b.light.z() * r);
    c.light.normalize();
    for (int i = 0; i < 16; ++i)
    {
        c.tf.colorMap()[4*i+0] = a.tf.colorMap()[4*i+0] * (1.f - r) + b.tf.colorMap()[4*i+0] * r;
        c.tf.colorMap()[4*i+1] = a.tf.colorMap()[4*i+1] * (1.f - r) + b.tf.colorMap()[4*i+1] * r;
        c.tf.colorMap()[4*i+2] = a.tf.colorMap()[4*i+2] * (1.f - r) + b.tf.colorMap()[4*i+2] * r;
        c.tf.colorMap()[4*i+3] = a.tf.colorMap()[4*i+3] * (1.f - r) + b.tf.colorMap()[4*i+3] * r;
    }
    return c;
}
