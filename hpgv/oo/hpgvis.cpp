#include "hpgvis.h"
#include <cassert>
#include <cfloat>
#include "hpgv.h"
#include "imageraf.h"
#include "imagergba.h"

//
//
// Data Quantize
//
//

static double theValueMin = 0;
static double theValueMax = 0;
float data_quantize(float value, int varname)
{
    float v = (value - theValueMin) / (theValueMax - theValueMin);
    if (v < 0.0) v = 0.0;
    if (v >= 1.0) v = 1.0 - FLT_EPSILON;
    return v;
}

//
//
// Constructors
//
//

HPGVis::HPGVis()
  : isInit(false),
  	rank(0), root(DEFAULT_ROOT), comm(MPI_COMM_WORLD),
    npx(1), npy(1), npz(1),
    ipx(0), ipy(0), ipz(0),
    block(new block_t), image(NULL)
{
	block->volume_num = 0;
	updateMPI();
	block_add_volume(block, HPGV_DOUBLE, NULL, 0); // add an empty volume
	block_set_quantize(data_quantize);
}

HPGVis::~HPGVis()
{
    if (block) block_finalize(block);
    if (image) delete image;
}

//
//
// Initialization
//
//

void HPGVis::setProcDims(int npx, int npy, int npz)
{
	this->npx = npx;
	this->npy = npy;
	this->npz = npz;
	updateMPI();
}

void HPGVis::initialize()
{
	hpgv_vis_init(comm, root);
}

void HPGVis::initialize(MPI_Comm comm, int root)
{
	this->comm = comm;
	this->root = root;
	updateMPI();

	this->initialize();
}

//
//
// Public Interface
//
//

void HPGVis::setParameter(const hpgv::Parameter& para)
{
	this->para = para;
	hpgv_vis_para(para);
}

void HPGVis::setVolume(const hpgv::Volume& volume)
{
	int size; MPI_Comm_size(comm, &size);
	assert(size == npx * npy * npz);
	block_header_new(rank, comm, npx * npy * npz,
			volume.xCoords.data(), volume.yCoords.data(), volume.zCoords.data(),
			volume.xCoords.size(), volume.yCoords.size(), volume.zCoords.size(),
			npx, npy, npz,
			&block->blk_header);
	block_set_volume(block, 0, HPGV_DOUBLE, volume.data, 0);
}

void HPGVis::render()
{
    block_exchange_boundary(block, para.getImages()[0].volumes[0].id);
	computeMinMax();
	hpgv_vis_render(block, rank, comm, 0);
	updateImage();
}

void HPGVis::render(const hpgv::Volume& volume)
{
	this->setVolume(volume);
	this->render();
}

//
//
// Protected Methods
//
//

void HPGVis::updateMPI()
{
	// 3d index
	MPI_Comm_rank(comm, &rank);
	ipz = rank / (npx * npy);
	ipy = (rank - ipz * npx * npy) / npx;
	ipx = (rank - ipz * npx * npy) % npx;
	// block
	block->mpiid = rank;
	block->mpicomm = comm;
}

void HPGVis::computeMinMax()
{
    double min, max;
    double allmin, allmax;
    int volsize = block->blk_header.blk_grid_size[0]
                * block->blk_header.blk_grid_size[1]
                * block->blk_header.blk_grid_size[2];
    for (int vol = 0; vol < block->volume_num; ++vol)
    {
        double* data = reinterpret_cast<double*>(block->volume_data[vol]->data_original);
        min = max = data[0];
        for (int i = 1; i < volsize; ++i)
        {
            if (min > data[i]) min = data[i];
            if (max < data[i]) max = data[i];
        }

        MPI_Barrier(block->mpicomm);
        MPI_Allreduce(&min, &allmin, 1, MPI_DOUBLE, MPI_MIN, block->mpicomm);
        MPI_Barrier(block->mpicomm);
        MPI_Allreduce(&max, &allmax, 1, MPI_DOUBLE, MPI_MAX, block->mpicomm);
    }
    theValueMin = allmin;
    theValueMax = allmax;
}

void HPGVis::updateImage()
{
    if (rank != root)
        return;
	if (image) delete image; image = NULL;
	// for now
	if (para.getFormat() == HPGV_RAF)
    {
    	// raf
    	int nBins = 16;
    	std::vector<float> alphas(nBins);
        for (int i = 0; i < nBins; ++i)
        {
            float a = 0.f;
            for (int j = i * hpgv::Parameter::tfSize / nBins; j < (i + 1) * hpgv::Parameter::tfSize / nBins; ++j)
            {
                a += para.getImages()[0].tf[4 * j + 3];
            }
            a /= (hpgv::Parameter::tfSize / nBins);
            alphas[i] = a;
        }
    	hpgv::ImageRAF* raf = new hpgv::ImageRAF;
    	raf->setSize(para.getView().width, para.getView().height);
    	raf->setNBins(nBins);
    	raf->setAlphas(alphas);
        raf->setRafs(hpgv_vis_get_databufptr(),
                para.getView().width * para.getView().height * (nBins + 1) * 2 * sizeof(float));
    	// pointer
    	image = raf;

    } else if (para.getFormat() == HPGV_RGBA)
    {
        hpgv::ImageRGBA* rgba = new hpgv::ImageRGBA;
        rgba->setSize(para.getView().width, para.getView().height);
        rgba->set(para.getFormat(), para.getType(), hpgv_vis_get_imageptr());
        // pointer
        image = rgba;
    }
}