#include <mpi.h>
#include <iostream>

#include "Config.hpp"
#include "Common.hpp"
#include "Message.hpp"
#include "Customer.hpp"
#include "Hunter.hpp"

using namespace std;

int main(int argc, char** argv) {
    int tid, threads;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &threads);
    MPI_Comm_rank(MPI_COMM_WORLD, &tid);

    Config config {
        .shopSize = 1,
	    .minOrders = 2,
	    .maxOrders = 5,
        .hunterMin = 1,
        .hunterMax = 2,
        .storeWaitMin = 1,
        .storeWaitMax = 3,
        .missionWaitMin = 1,
        .missionWaitMax = 3,
    };

    if(tid == 0) {
        Customer customer(tid, config);
        customer.loop();
    } else {
        Hunter hunter(tid, config);
        hunter.loop();
    }
    
    MPI_Finalize();
    return 0;
}