#include "parameter.h"
#include <cstring>
#include <cassert>
#include <iostream>
#include <fstream>
#include "hpgv_gl.h"

namespace hpgv
{

//
//
// Constructor / Destructor
//
//

Parameter::Parameter()
  : format(HPGV_RGBA)
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

} // namespace hpgv
