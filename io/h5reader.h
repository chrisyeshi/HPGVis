#ifndef __hpgv_H5Reader_h__
#define __hpgv_H5Reader_h__

#include <string>
#include <mpi.h>
#include "boost/shared_ptr.hpp"
#include "volume.h"

namespace hpgv
{

class H5Reader
{
public:
    H5Reader(const std::string& filename, const std::string& dataset);

    void configure(MPI_Comm mpicomm, int idx, int idy, int idz, int npx, int npy, int npz);
    bool read();
    hpgv::Volume volume() const;

protected:
private:
    std::string filename;
    std::string dataset;
    MPI_Comm mpicomm;
    int idx, idy, idz;
    int npx, npy, npz;
    int nx, ny, nz;
    boost::shared_ptr<double[]> data;
    boost::shared_ptr<double[]> x, y, z;
};

} // namespace hpgv

#endif // __hpgv_H5Reader_h__