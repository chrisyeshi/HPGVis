#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cassert>
#include <cstdlib>
#include <mpi.h>
#include "hpgvis.h"
#include "io/h5reader.h"
#include "parameter.h"
#include "json.h"

std::map<std::string, std::string> suffixes;
std::map<std::string, int> formats;

void initMaps()
{
    suffixes["RAF"] = ".raf";
    suffixes["RGBA"] = ".ppm";
    formats["RAF"] = HPGV_RAF;
    formats["RGBA"] = HPGV_RGBA;
}

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
    // print usage
    if (argc != 7)
    {
        std::cout << "Usage: ./par.sh X Y Z CONFIG VOLUME OUTPUT" << std::endl;
        return 0;
    }
    // two maps for inputs
    initMaps();
	// Yay~ MPI
	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size; MPI_Comm_size(MPI_COMM_WORLD, &size);
    int npx = atoi(argv[1]), npy = atoi(argv[2]), npz = atoi(argv[3]);
	assert(size == npx * npy * npz);
    int mypz = rank /(npx * npy);
    int mypx = (rank - mypz * npx * npy) % npx;
    int mypy = (rank - mypz * npx * npy) / npx;
    // volume
    hpgv::H5Reader reader(argv[5], "/qcr");
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
        vis.getImage()->save(std::string(argv[6]) + ".raf");

	MPI_Finalize();
	return 0;
}
