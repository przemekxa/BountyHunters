#ifndef CUSTOMER_HPP
#define CUSTOMER_HPP

#include <list>
#include <mpi.h>

#include "Config.hpp"
#include "Common.hpp"
#include "Message.hpp"

class Customer: Loggable {
private:

    // Identifier of the Customer
    const int64_t id;

    // Configuration of the program
    const Config config;

    // Datatypes used by the MPI
    const Datatype types;

    // Logger which prints Lamport values
    const Logger logger;

    // State

    // Lamport clock
    uint64_t lamport = 0;

    // List of uncompleted orders
    list<Order> orders;

    // Status used by the MPI_Recv
    MPI_Status status;

    // Place a new order
    void makeOrder();

    // Receive order from a Hunter (blocks the thread)
    void receiveOrderCompletion();

public:

    Customer(int64_t id, const Config& config);

    // Return the current lamport value
    uint64_t getLamport() override;

    // Main loop
    void loop();

};

#endif