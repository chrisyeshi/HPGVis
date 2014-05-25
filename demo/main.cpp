#include <iostream>
#include <map>
#include <string>
#include <cassert>
#include <cstdlib>
#include <mpi.h>
#include "hpgvis.h"
#include "io/h5reader.h"
#include "parameter.h"

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
    assert(argc == 6);
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
 //    // parameter
	// hpgv::Parameter para;
 //    assert(para.open("../../supernova_nocut.hp"));
	// para.setFormat(formats[argv[4]]);
 //    para.rView().width = 1024;
 //    para.rView().height = 1024;
 //    para.rView().viewport[2] = 1024;
 //    para.rView().viewport[3] = 1024;
 //    para.rImages()[0].sampleSpacing = 1.0;
 //    // loop to read the same file XD
 //    HPGVis vis;
 //    vis.initialize();
 //    vis.setProcDims(npx, npy, npz);
 //    vis.setParameter(para);
 //    while (true)
 //    {
 //        // volume
 //        hpgv::H5Reader reader("/home/chrisyeshi/Dropbox/supernova_600_1580.h5", "/entropy");
 //        reader.configure(MPI_COMM_WORLD, mypx, mypy, mypz, npx, npy, npz);
 //        assert(reader.read());
 //        // render
 //        vis.setVolume(reader.volume());
 //        vis.render();
 //        if (rank == vis.getRoot())
 //        {
 //            static char c = '0';
 //            vis.getImage()->save(std::string(argv[5]) + c + suffixes[argv[4]]);
 //            c++;
 //        }
 //    }

    // volume
    hpgv::H5Reader reader("/home/chrisyeshi/Dropbox/supernova_600_1580.h5", "/entropy");
	reader.configure(MPI_COMM_WORLD, mypx, mypy, mypz, npx, npy, npz);
    bool isRead = reader.read();
	assert(isRead);
	// parameter
	hpgv::Parameter para;
    bool isPara = para.open("../../supernova_nocut.hp");
    assert(isPara);
	para.setFormat(formats[argv[4]]);
    para.rView().width = 1024;
    para.rView().height = 1024;
    para.rView().viewport[2] = 1024;
    para.rView().viewport[3] = 1024;
    para.rImages()[0].sampleSpacing = 10.0;
	// vis
	HPGVis vis;
	vis.initialize();
	vis.setProcDims(npx, npy, npz);
	vis.setParameter(para);
	vis.setVolume(reader.volume());
	vis.render();
	if (rank == vis.getRoot())
		vis.getImage()->save(std::string(argv[5]) + suffixes[argv[4]]);

	MPI_Finalize();
	return 0;
}
