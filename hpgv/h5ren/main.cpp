#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cassert>
#include <cstdlib>
#include <mpi.h>
#include "hpgvis.h"
#include "h5reader.h"
#include "parameter.h"
#include "json.h"

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
    // print usage
    if (argc != 8)
    {
        std::cout << "Usage: ./par.sh X Y Z CONFIG HDF5 DATASET OUTPUT" << std::endl;
        return 0;
    }
	// Yay~ MPI
	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size; MPI_Comm_size(MPI_COMM_WORLD, &size);
    int npx = atoi(argv[1]), npy = atoi(argv[2]), npz = atoi(argv[3]);
	assert(size == npx * npy * npz);
    int mypz = rank /(npx * npy);
    int mypx = (rank - mypz * npx * npy) % npx;
    int mypy = (rank - mypz * npx * npy) / npx;
    // volume
    hpgv::H5Reader reader(argv[5], argv[6]);
    reader.configure(MPI_COMM_WORLD, mypx, mypy, mypz, npx, npy, npz);
    if (!reader.read())
        exit(1);
	// parameter
	hpgv::Parameter para;
    if (!para.open(argv[4]))
        exit(1);
	// vis
	HPGVis vis;
	vis.initialize();
	vis.setProcDims(npx, npy, npz);
	vis.setParameter(para);
    vis.setVolume(reader.volume());
	vis.render();
	if (rank == vis.getRoot())
        vis.getImage()->save(std::string(argv[7]) + ".raf");

	MPI_Finalize();
	return 0;
}
