#ifndef HUNTER_HPP
#define HUNTER_HPP

#include <cstdint>
#include <list>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <random>
#include <thread>
#include <algorithm>
#include <mpi.h>

#include <unistd.h>

#include "Config.hpp"
#include "Common.hpp"
#include "Message.hpp"

using namespace std;

enum class HunterState {
    Waiting,
    GettingOrder,
    GettingStore,
    InStore,
    Mission
};

class Hunter : Loggable {
private:

    // Identifier of the Hunter
    const int64_t id;

    // Configuration of the program
    const Config config;

    // Datatypes used by the MPI
    const Datatype types;

    // Logger which prints Lamport values
    const Logger logger;


    // Lamport value from the completion of last task
    uint64_t lastOrderLamport = 0;

    // Current state
    HunterState state = HunterState::Waiting;
    mutex stateMutex;

    // Lamport value (Do not use directly!)
    uint64_t lamport = 0;
    mutex lamportMutex;

    // Pending orders
    list<Order> orders;

    // Rejected orders
    list<Order> rejected;

    // Status used by the MPI_Recv
    MPI_Status status;

    // 
    // States
    //

    // Waiting
    condition_variable  waitingForNewOrderWait;

    // Getting order
    condition_variable  gettingOrderWait;
    int64_t             gettingOrderRemaining = 0;
    bool                gettingOrderGotOrder = false;

    // Waiting in line for the store
    uint64_t                            waitingForStoreLamport = 0;
    condition_variable                  waitingForStoreWait;
    int64_t                             waitingForStoreRemaining = 0;
    unordered_map<int64_t, uint64_t>    waitingForStoreHunters;

    // Increment the max(current, given) lamport value by 1
    void incrementLamport(uint64_t received);

    // Increment the current lamport value by 1
    void incrementLamport();


    // Handle the `Order` message
    void handleOrder();

    // Handle the `OrderRequest` message
    void handleOrderRequest();

    // Handle the `OrderRequestAck` message
    void handleOrderRequestAck();

    // Handle the `StoreRequest` message
    void handleStoreRequest();

    // Handle the `StoreRequestAck` message
    void handleStoreRequestAck();


    // Loop performed by the background (messaging thread)
    void loopBackground();

    // Loop performed by the main thread
    void loopForeground();

public:

    Hunter(int64_t id, const Config& config);

    // Return the current lamport value
    uint64_t getLamport() override;

    // Main loop
    void loop();
};

#endif