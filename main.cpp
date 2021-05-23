#include <iostream>
#include <mpi.h>

#include "Config.hpp"
#include "Customer.hpp"
#include "Hunter.hpp"

using namespace std;

int main(int argc, char** argv) {
    int tid, threads;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &threads);
    MPI_Comm_rank(MPI_COMM_WORLD, &tid);

    Config config = Config::fromArgs(argc, argv);

    if(tid < config.hunterMin) {
        Customer customer(tid, config);
        customer.loop();
    } else {
        Hunter hunter(tid, config);
        hunter.loop();
    }
    
    MPI_Finalize();
    return 0;
}