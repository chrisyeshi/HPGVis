#include "h5reader.h"
#include <cassert>
#include <hdf5.h>

namespace hpgv
{

H5Reader::H5Reader(const std::string& filename, const std::string& dataset)
  : filename(filename), dataset(dataset)
{

}

void H5Reader::configure(MPI_Comm mpicomm, int idx, int idy, int idz, int npx, int npy, int npz)
{
    this->mpicomm = mpicomm;
    this->idx = idx;
    this->idy = idy;
    this->idz = idz;
    this->npx = npx;
    this->npy = npy;
    this->npz = npz;
}

bool H5Reader::read()
{
    herr_t h5Status;
    hid_t h5FileId;
    hid_t h5DsetId;
    hid_t acc_tpl = H5Pcreate(H5P_FILE_ACCESS);
    h5Status = H5Pset_fapl_mpio(acc_tpl, mpicomm, MPI_INFO_NULL);
    h5FileId = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, acc_tpl);
    if (h5FileId < 0)
        return false;
    h5Status = H5Pclose(acc_tpl);
    h5DsetId = H5Dopen(h5FileId, dataset.c_str(), H5P_DEFAULT);
    hid_t spac_id = H5Dget_space(h5DsetId);
    hsize_t htotal_size3[3];
    h5Status = H5Sget_simple_extent_dims(spac_id, htotal_size3, NULL);

    nx = htotal_size3[0] / npx;
    ny = htotal_size3[1] / npy;
    nz = htotal_size3[2] / npz;

    hsize_t start[3], count[3];

    start[2] = idx * nx;
    start[1] = idy * ny;
    start[0] = idz * nz;

    count[2] = nx;
    count[1] = ny;
    count[0] = nz;

    int readCount = count[0] * count[1] * count[2];
    data = boost::shared_ptr<double[]>(new double [readCount]);

    h5Status = H5Sselect_hyperslab(spac_id, H5S_SELECT_SET, start, NULL, count, NULL);
    hid_t memspace = H5Screate_simple(3, count, NULL);
    h5Status = H5Dread(h5DsetId, H5T_NATIVE_DOUBLE, memspace, spac_id, H5P_DEFAULT, data.get());

    H5Dclose(h5DsetId);
    H5Fclose(h5FileId);

    x = boost::shared_ptr<double[]>(new double [nx]);
    y = boost::shared_ptr<double[]>(new double [ny]);
    z = boost::shared_ptr<double[]>(new double [nz]);
    for (int i = 0; i < nx; ++i)
        x[i] = start[2] + i;
    for (int i = 0; i < ny; ++i)
        y[i] = start[1] + i;
    for (int i = 0; i < nz; ++i)
        z[i] = start[0] + i;

    return true;
}

hpgv::Volume H5Reader::volume() const
{
    std::vector<double> xCoords(x.get(), x.get() + nx);
    std::vector<double> yCoords(y.get(), y.get() + ny);
    std::vector<double> zCoords(z.get(), z.get() + nz);
    return hpgv::Volume(data.get(), xCoords, yCoords, zCoords);
}

} // namespace hpgv