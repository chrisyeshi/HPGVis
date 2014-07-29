#include "parameter.h"
#include <cstring>
#include <cassert>
#include <iostream>
#include <fstream>
#include "hpgv_gl.h"
#include "json.h"

namespace hpgv
{

//
//
// Constructor / Destructor
//
//

Parameter::Parameter()
  : format(HPGV_RGBA), autoMinmax(true)
{

}

Parameter::~Parameter()
{

}

//
//
// I/O
//
//

bool Parameter::open(const std::string& filename)
{
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    if ("json" == ext || "JSON" == ext)
    { // Json config file
        std::ifstream fin(filename.c_str());
        Json::Value root;
        Json::Reader reader;
        bool parsed = reader.parse(fin, root);
        if (!parsed) return parsed;
        return this->fromJSON(root);
    }
    // Binary config file
    std::ifstream fin(filename.c_str(), std::ios::binary);
    if (!fin) return false;
    fin.seekg(0, fin.end);
    int filesize = fin.tellg();
    fin.seekg(0, fin.beg);
    char* buffer = new char [filesize];
    fin.read(buffer, filesize);
    this->deserialize(buffer);
    delete [] buffer;
    return true;
}

void Parameter::save(const std::string& filename) const
{
    std::vector<char> vec = this->serialize();
    std::ofstream fout(filename.c_str(), std::ios::binary);
    assert(fout);
    fout.write(vec.data(), vec.size());
}

std::vector<char> Parameter::serialize() const
{
    int intSize = sizeof(int);
    int matSize = sizeof(double) * 16;
    int int4Size = intSize * 4;
    int floatSize = sizeof(float);
    int tfMemSize = tfSize * sizeof(float) * 4;
    int lightSize = sizeof(float) * 4;
    int charSize = sizeof(char);
    int uint64Size = sizeof(uint64_t);
    int binTicksSize = floatSize * (HPGV_RAF_BIN_NUM + 1);
    // buffer size
    int bufferSize = 0;
    bufferSize += intSize * 3;  // colormap
    bufferSize += intSize * 2;  // image format and type
    bufferSize += matSize * 2 + int4Size + intSize * 2 + floatSize * 2; // view
    bufferSize += intSize;  // nImages
    for (unsigned int i = 0; i < images.size(); ++i)
    {
        // particle
        bufferSize += intSize + floatSize + intSize;
        if (images[i].particle.count == 1)
            bufferSize += tfMemSize + charSize + lightSize;
        // volume
        bufferSize += intSize + floatSize;  // nVolumes + SampleSpacing
        if (images[i].volumes.size() > 0)
        {
            bufferSize += intSize * images[i].volumes.size();    // id
            bufferSize += tfMemSize;    // tf
            for (unsigned int j = 0; j < images[i].volumes.size(); ++j)
                bufferSize += charSize + lightSize; // light
        }
        bufferSize += intSize + binTicksSize; // adaptive binning
    }
    // totalbyte also takes a uint64Size
    bufferSize += uint64Size;
    // data
    std::vector<char> buffer(bufferSize);
    char* ptr = &(buffer[0]);
    // colormap
    memcpy(ptr, &colormap.size, intSize); ptr += intSize;
    memcpy(ptr, &colormap.format, intSize); ptr += intSize;
    memcpy(ptr, &colormap.type, intSize); ptr += intSize;
    // image format and type
    memcpy(ptr, &format, intSize); ptr += intSize;
    memcpy(ptr, &type, intSize); ptr += intSize;
    // view
    memcpy(ptr, &(view.modelview[0]), matSize); ptr += matSize;
    memcpy(ptr, &(view.projection[0]), matSize); ptr += matSize;
    memcpy(ptr, &(view.viewport[0]), int4Size); ptr += int4Size;
    memcpy(ptr, &view.width, intSize); ptr += intSize;
    memcpy(ptr, &view.height, intSize); ptr += intSize;
    memcpy(ptr, &view.angle, floatSize); ptr += floatSize;
    memcpy(ptr, &view.scale, floatSize); ptr += floatSize;
    // images
    int nImages = images.size();
    memcpy(ptr, &nImages, intSize); ptr += intSize;
    for (int i = 0; i < nImages; ++i)
    {
        const Image& image = images[i];
        // particle
        memcpy(ptr, &image.particle.count, intSize); ptr += intSize;
        memcpy(ptr, &image.particle.radius, floatSize); ptr += floatSize;
        memcpy(ptr, &image.particle.volume, intSize); ptr += intSize;
        if (image.particle.count == 1)
        {
            memcpy(ptr, &(image.particle.tf[0]), tfMemSize); ptr += tfMemSize;
            char withlighting = image.particle.light.enable ? 1 : 0;
            memcpy(ptr, &withlighting, charSize); ptr += charSize;
            memcpy(ptr, &(image.particle.light.parameter[0]), lightSize); ptr += lightSize;
        }
        // volume
        int nVolumes = image.volumes.size();
        memcpy(ptr, &nVolumes, intSize); ptr += intSize;
        memcpy(ptr, &image.sampleSpacing, floatSize); ptr += floatSize;
        if (nVolumes > 0)
        {
            std::vector<int> volIds(nVolumes);
            for (int i = 0; i < nVolumes; ++i)
                volIds[i] = image.volumes[i].id;
            memcpy(ptr, &(volIds[0]), intSize * nVolumes); ptr += intSize * nVolumes;
            memcpy(ptr, &(image.tf[0]), tfMemSize); ptr += tfMemSize;
            for (int i = 0; i < nVolumes; ++i)
            {
                char withlighting = image.volumes[i].light.enable ? 1 : 0;
                memcpy(ptr, &withlighting, charSize); ptr += charSize;
                memcpy(ptr, &(image.volumes[i].light.parameter[0]), lightSize); ptr += lightSize;
            }
        }
        // adaptive binning
//        memcpy(ptr, &image.enableAdaptive, intSize); ptr += intSize;
//        memcpy(ptr, image.binTicks, binTicksSize); ptr += binTicksSize;
    }
    // total byte
    uint64_t totalbyte = bufferSize;
    memcpy(ptr, &totalbyte, uint64Size); ptr += uint64Size;
    assert(totalbyte == ptr - &(buffer[0]));
    return buffer;
}

bool Parameter::deserialize(const char * head)
{
    char const * ptr = head;
    int intSize = sizeof(int);
    int matSize = sizeof(double) * 16;
    int int4Size = intSize * 4;
    int floatSize = sizeof(float);
    int tfMemSize = tfSize * sizeof(float) * 4;
    int lightSize = sizeof(float) * 4;
    int charSize = sizeof(char);
    int uint64Size = sizeof(uint64_t);
    int binTicksSize = floatSize * (HPGV_RAF_BIN_NUM + 1);

    // colormap
    memcpy(&colormap.size, ptr, intSize); ptr += intSize;
    memcpy(&colormap.format, ptr, intSize); ptr += intSize;
    memcpy(&colormap.type, ptr, intSize); ptr += intSize;
    // image format and type
    memcpy(&format, ptr, intSize); ptr += intSize;
    memcpy(&type, ptr, intSize); ptr += intSize;
    // view
    memcpy(&(view.modelview[0]), ptr, matSize); ptr += matSize;
    memcpy(&(view.projection[0]), ptr, matSize); ptr += matSize;
    memcpy(&(view.viewport[0]), ptr, int4Size); ptr += int4Size;
    memcpy(&view.width, ptr, intSize); ptr += intSize;
    memcpy(&view.height, ptr, intSize); ptr += intSize;
    memcpy(&view.angle, ptr, floatSize); ptr += floatSize;
    memcpy(&view.scale, ptr, floatSize); ptr += floatSize;
    // images
    int nImages = 0;
    memcpy(&nImages, ptr, intSize); ptr += intSize;
    images.resize(nImages);
    for (int i = 0; i < nImages; ++i)
    {
        Image& image = images[i];
        // particle
        memcpy(&image.particle.count, ptr, intSize); ptr += intSize;
        assert(image.particle.count == 0 || image.particle.count == 1);
        memcpy(&image.particle.radius, ptr, floatSize); ptr += floatSize;
        memcpy(&image.particle.volume, ptr, intSize); ptr += intSize;
        if (image.particle.count == 1)
        {
            image.particle.tf = boost::shared_ptr<float[tfSize * 4]>(new float [tfSize * 4]);
            memcpy(image.particle.tf.get(), ptr, tfMemSize); ptr += tfMemSize;
            char withlighting = 0;
            memcpy(&withlighting, ptr, charSize); ptr += charSize;
            image.particle.light.enable = withlighting == 1 ? true : false;
            memcpy(&(image.particle.light.parameter[0]), ptr, lightSize); ptr += lightSize;
        }
        // volume
        int nVolumes = 0;
        memcpy(&nVolumes, ptr, intSize); ptr += intSize;
        image.volumes.resize(nVolumes);
        memcpy(&image.sampleSpacing, ptr, floatSize); ptr += floatSize;
        if (nVolumes > 0)
        {
            // id
            std::vector<int> volIds(nVolumes);
            memcpy(&(volIds[0]), ptr, intSize * nVolumes); ptr += intSize * nVolumes;
            // tf
            image.tf = boost::shared_ptr<float[tfSize * 4]>(new float [tfSize * 4]);
            memcpy(image.tf.get(), ptr, tfMemSize);
            
            ptr += tfMemSize;
            // light
            for (int i = 0; i < nVolumes; ++i)
            {
                Volume& vol = image.volumes[i];
                vol.id = volIds[i];
                char withlighting = 0;
                memcpy(&withlighting, ptr, charSize); ptr += charSize;
                vol.light.enable = withlighting == 1 ? true : false;
                memcpy(&(vol.light.parameter[0]), ptr, lightSize); ptr += lightSize;
            }
        }
        // binticks
//        memcpy(&image.enableAdaptive, ptr, intSize); ptr += intSize;
//        memcpy(image.binTicks, ptr, binTicksSize); ptr += binTicksSize;
    }
    // total byte
    uint64_t totalbyte = 0;
    memcpy(&totalbyte, ptr, uint64Size); ptr += uint64Size;
    uint64_t ptrbyte = ptr - head;
    if (totalbyte != ptrbyte)
        return false;
    // yeah!!!!!
    return true;    
}

bool Parameter::deserialize(const std::vector<char>& buffer)
{
    if (buffer.empty())
        return false;
    return this->deserialize(&buffer[0]);
}

Json::Value Parameter::toJSON() const
{
    Json::Value root;
    // format
    if (HPGV_RAF == format)
        root["format"] = "RAF";
    else
        root["format"] = "RGBA";
    // type
    if (HPGV_FLOAT == type)
        root["type"] = "float";
    else
        root["type"] = "double";
    // view
    for (int i = 0; i < 16; ++i)
    {
        root["view"]["modelview"][i] = view.modelview[i];
        root["view"]["projection"][i] = view.projection[i];
    }
    for (int i = 0; i < 4; ++i)
        root["view"]["viewport"][i] = view.viewport[i];
    root["view"]["width"] = view.width;
    root["view"]["height"] = view.height;
    root["view"]["angle"] = view.angle;
    root["view"]["scale"] = view.scale;
    // images
    for (unsigned int i = 0; i < images.size(); ++i)
    {
        root["images"][i]["sampleSpacing"] = images[i].sampleSpacing;
        for (int j = 0; j < HPGV_RAF_BIN_NUM + 1; ++j)
            root["images"][i]["binTicks"][j] = images[i].binTicks[j];
    }
    // minmax
    if (!autoMinmax)
    {
        root["minmax"][0] = minmax[0];
        root["minmax"][1] = minmax[1];
    }

    return root;
}

bool Parameter::fromJSON(const Json::Value& root)
{
    // format
    if ("RAF" == root["format"].asString())
        format = HPGV_RAF;
    else if ("RGBA" == root["format"].asString())
        format = HPGV_RGBA;
    // type
    if ("float" == root["type"].asString())
        type = HPGV_FLOAT;
    else if ("double" == root["type"].asString())
        type = HPGV_DOUBLE;
    // view
    for (int i = 0; i < 16; ++i)
    {
        view.modelview[i] = root["view"]["modelview"][i].asDouble();
        view.projection[i] = root["view"]["projection"][i].asDouble();
    }
    for (int i = 0; i < 4; ++i)
        view.viewport[i] = root["view"]["viewport"][i].asInt();
    view.width = root["view"]["width"].asInt();
    view.height = root["view"]["height"].asInt();
    view.angle = root["view"]["angle"].asFloat();
    view.angle = root["view"]["scale"].asFloat();
    // images
    images.resize(root["images"].size());
    for (unsigned int i = 0; i < root["images"].size(); ++i)
    {
        images[i].volumes.resize(1);
        images[i].sampleSpacing = root["images"][i]["sampleSpacing"].asFloat();
        if (root["images"][i]["binTicks"].isNull())
        { // standard binticks
            images[i].binTicks[0] = 0.0;
            images[i].binTicks[HPGV_RAF_BIN_NUM] = 1.0;
            for (int j = 1; j < HPGV_RAF_BIN_NUM; ++j)
                images[i].binTicks[j] = 1.0 / float(HPGV_RAF_BIN_NUM) * float(j);

        } else
        { // custom binticks
            for (int j = 0; j < HPGV_RAF_BIN_NUM + 1; ++j)
                images[i].binTicks[j] = root["images"][i]["binTicks"][j].asFloat();
        }
    }
    // minmax
    if (root["minmax"].isNull())
    {
        autoMinmax = true;
    } else
    {
        autoMinmax = false;
        minmax[0] = root["minmax"][0].asDouble();
        minmax[1] = root["minmax"][1].asDouble();
    }

    return true;
}

} // namespace hpgv
