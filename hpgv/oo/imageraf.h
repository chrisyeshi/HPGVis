#ifndef __hpgv_ImageRAF__
#define __hpgv_ImageRAF__

#include <vector>
#include <string>
#include <iostream>
#include "image.h"
#include "boost/shared_ptr.hpp"

namespace hpgv
{

class ImageRAF : public Image
{
public:
    ImageRAF();
    virtual ~ImageRAF() {};

    void setNBins(unsigned int nBins) { alphas.resize(nBins); }
    void setRafs(boost::shared_ptr<float[]> rafs) { this->rafs = rafs; }
    void setRafs(const void *buffer, int nBytes);
    void setAlphas(const std::vector<float>& a);

    unsigned int getNBins() const { return nBins(); }
    unsigned int getNAlphaBins() const { return nAlphaBins(); }
    boost::shared_ptr<float[]> getRafs() const { return rafs; }
    boost::shared_ptr<float[]> getDepths() const { return depths; }
    std::vector<float> getAlphas() const { return alphas; }

    virtual bool read(std::istream& in);
    virtual void write(std::ostream& out) const;

protected:

private:
    boost::shared_ptr<float[]> rafs;
    boost::shared_ptr<float[]> depths;
    std::vector<float> alphas;

    //
    //
    // Helper functions
    //
    //
    unsigned int nBins() const { return alphas.size(); }
    unsigned int nAlphaBins() const { return nBins() + 1; }
    int nFloatsSlice() const { return width * height; }
    int nBytesSlice() const { return nFloatsSlice() * sizeof(float); }
    int nFloatsRafs() const { return width * height * nBins(); }
    int nBytesRafs() const { return nFloatsRafs() * sizeof(float); }
    int nFloatsDepths() const { return width * height * nBins(); }
    int nBytesDepths() const { return nFloatsDepths() * sizeof(float); }
};

} // namespace hpgv

#endif // __hpgv_ImageRAF__
