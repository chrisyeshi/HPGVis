#include "imageraf.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <sys/time.h>

namespace hpgv
{

ImageRAF::ImageRAF()
{
}

ImageRAF::~ImageRAF()
{
}

void ImageRAF::setRafs(const void *buffer, int nBytes)
{
    assert(nBytes == nBytesRafs() + nBytesDepths() + 10 * nBytesSlice());
    const float* inBuf = reinterpret_cast<const float*>(buffer);
    rafs = boost::shared_ptr<float[]>(new float [nFloatsRafs()]);
    depths = boost::shared_ptr<float[]>(new float [nFloatsDepths()]);
    for (unsigned int sliceId = 0; sliceId < nBins(); ++sliceId)
    {
        for (int i = 0; i < width * height; ++i)
        {
            // raf
            rafs[sliceId * nFloatsSlice() + i]
                    = inBuf[(nBins() * 2 + 10) * i + 10 + nBins() * 0 + sliceId];
            // depth
            depths[sliceId * nFloatsSlice() + i]
                    = inBuf[(nBins() * 2 + 10) * i + 10 + nBins() * 1 + sliceId];
        }
    }
}

void ImageRAF::setAlphas(const std::vector<float> &a)
{
    assert(a.size() == nBins());
    alphas = a;
}

bool ImageRAF::read(std::istream& in)
{
    in.read(reinterpret_cast<char*>(&width), sizeof(int));
    in.read(reinterpret_cast<char*>(&height), sizeof(int));
    int format;
    in.read(reinterpret_cast<char*>(&format), sizeof(int));
    int type;
    in.read(reinterpret_cast<char*>(&type), sizeof(int));
    int realnum;
    in.read(reinterpret_cast<char*>(&realnum), sizeof(int));
    int nbins;
    in.read(reinterpret_cast<char*>(&nbins), sizeof(int));
    alphas.resize(nbins);
    in.read(reinterpret_cast<char*>(alphas.data()), alphas.size() * sizeof(float));
    rafs = boost::shared_ptr<float[]>(new float [nFloatsRafs()]);
    in.read(reinterpret_cast<char*>(rafs.get()), nBytesRafs());
    depths = boost::shared_ptr<float[]>(new float [nFloatsDepths()]);
    in.read(reinterpret_cast<char*>(depths.get()), nBytesDepths());

    return true;
}

void ImageRAF::save(const std::string& filename) const
{
    std::ofstream out((filename + ".raf").c_str(), std::ios::binary);
    assert(out);
    out.write(reinterpret_cast<const char*>(&width),   sizeof(int));
    out.write(reinterpret_cast<const char*>(&height),  sizeof(int));
    int format = 0x201001;
    out.write(reinterpret_cast<const char*>(&format),  sizeof(int)); // HPGV_RAF
    int type = 0x1406;
    out.write(reinterpret_cast<const char*>(&type),    sizeof(int)); // HPGV_FLOAT
    int realnum = 1;
    out.write(reinterpret_cast<const char*>(&realnum), sizeof(int));
    int nbins = alphas.size();
    out.write(reinterpret_cast<const char*>(&nbins), sizeof(int));
    out.write(reinterpret_cast<const char*>(alphas.data()), alphas.size() * sizeof(float));
    out.write(reinterpret_cast<const char*>(rafs.get()), nBytesRafs());
    out.write(reinterpret_cast<const char*>(depths.get()), nBytesDepths());
}

} // namespace hpgv
