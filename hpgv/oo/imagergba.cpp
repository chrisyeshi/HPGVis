#include "imagergba.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstring>
#include "hpgv_gl.h"

namespace hpgv
{

ImageRGBA::ImageRGBA()
  : format(HPGV_RGBA), type(HPGV_UNSIGNED_BYTE), data(NULL)
{
}

ImageRGBA::ImageRGBA(const ImageRGBA& image)
  : Image(image), format(image.format), type(image.type), data(NULL)
{
    assert(this->width == image.width);
    this->data = new char [nBytesData()];
    memcpy(this->data, image.data, nBytesData());
}

ImageRGBA& ImageRGBA::operator=(const ImageRGBA& image)
{
    this->width = image.width;
    this->height = image.height;
    this->format = image.format;
    this->type = image.type;
    if (this->data)
    {
        delete [] this->data;
        this->data = NULL;
    }
    this->data = new char [nBytesData()];
    memcpy(this->data, image.data, nBytesData());
    return *this;
}

ImageRGBA::~ImageRGBA()
{
	if (data)
    {
        delete [] data;
        data = NULL;
    }
}

void ImageRGBA::set(int format, int type, const void* data)
{
	assert(format == HPGV_RGBA || format == HPGV_RGB);
	assert(type == HPGV_FLOAT || type == HPGV_UNSIGNED_SHORT || type == HPGV_UNSIGNED_BYTE);
	this->format = format;
	this->type = type;
	if (this->data)
    {
        delete [] this->data;
        this->data = NULL;
    }
	this->data = new char [nBytesData()];
	memcpy(this->data, reinterpret_cast<const char*>(data), nBytesData());
}

bool ImageRGBA::read(std::istream& in)
{
    // Not implemented yet....
    assert(false);
	return false;
}

void ImageRGBA::save(const std::string& filename) const
{
	assert(data);
    std::ofstream out((filename + ".ppm").c_str(), std::ios::binary);
    assert(out);
    int fsize = hpgv_formatsize(format);
    std::stringstream ss;
    ss << "P6\n" << width << " " << height << "\n255\n";
    std::string header = ss.str();
    out.write(header.c_str(), header.size());
    for (int y = height - 1; y >= 0; --y)
    for (int x = 0; x < width; ++x)
    {
        uint8_t r=0, g=0, b=0;
        int i = width * y + x;
        if (type == HPGV_FLOAT)
        {
            float* fdata = reinterpret_cast<float*>(data);
            float fr = fdata[i * fsize + 0];
            float fg = fdata[i * fsize + 1];
            float fb = fdata[i * fsize + 2];
            r = uint8_t(fr * 0xFF);
            g = uint8_t(fg * 0xFF);
            b = uint8_t(fb * 0xFF);

        } else if (type == HPGV_UNSIGNED_SHORT)
        {
            unsigned short* sdata = reinterpret_cast<unsigned short*>(data);
            r = uint8_t(sdata[i * fsize + 0] * 0xFF / 65535.f);
            g = uint8_t(sdata[i * fsize + 1] * 0xFF / 65535.f);
            b = uint8_t(sdata[i * fsize + 2] * 0xFF / 65535.f);

        } else if (type == HPGV_UNSIGNED_BYTE)
        {
            unsigned char* cdata = reinterpret_cast<unsigned char*>(data);
            r = cdata[i * fsize + 0];
            g = cdata[i * fsize + 1];
            b = cdata[i * fsize + 2];
        }
        out.put(r);
        out.put(g);
        out.put(b);
    }
}

int ImageRGBA::nBytesData() const
{
	int fsize = hpgv_formatsize(format);
	int tsize;
	if (type == HPGV_FLOAT)
		tsize = sizeof(float);
	else if (type == HPGV_UNSIGNED_SHORT)
		tsize = sizeof(unsigned short);
	else
		tsize = sizeof(unsigned char);
	return width * height * fsize * tsize;
}

} // namespace hpgv
