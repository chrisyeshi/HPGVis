#ifndef __hpgvis__
#define __hpgvis__

#include <mpi.h>
#include "parameter.h"
#include "hpgv_block.h"
#include "volume.h"
#include "image.h"

class HPGVis
{
public:
    //
    //
    // Constructors
    //
    //
    HPGVis();
    ~HPGVis();

    //
    //
    // MPI Specific
    //
    //
    void initialize();
    void initialize(MPI_Comm comm, int root);
    void setProcDims(int npx, int npy, int npz);
    int getRoot() const { return root; }

    //
    //
    // Public Interface
    //
    //
    void setParameter(const hpgv::Parameter& para);
    const hpgv::Parameter& getParameter() const { return para; }
    // We do not take care of the memory management of the volume
    void setVolume(const hpgv::Volume& volume);
    void render();
    void render(const hpgv::Volume& volume);
    const hpgv::Image* getImage() const { return image; }

protected:
    //
    //
    // Protected Methods
    //
    //
    void updateMPI();
    void computeMinMax();
    void updateImage();

private:
    static const int DEFAULT_ROOT = 0;

    bool isInit;
    int root;
    MPI_Comm comm;
    int npx, npy, npz;
    hpgv::Parameter para;
    block_t* block;
    hpgv::Image* image;

    //
    //
    // Computed Variables
    //
    //
    int rank;
    int ipx, ipy, ipz;
};

#endif // __hpgvis__