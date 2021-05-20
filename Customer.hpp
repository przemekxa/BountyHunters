#ifndef CUSTOMER_HPP
#define CUSTOMER_HPP

#include <list>
#include <mpi.h>

#include "Config.hpp"
#include "Common.hpp"
#include "Message.hpp"

class Customer: Loggable {
private:

    // Configuration

    // Identifier of the Customer
    int64_t id;

    Config config;

    // Datatype of the `Order`
    MPI_Datatype orderType;

    // Datatype of the `OrderCompletion`
    MPI_Datatype orderCompletionType;

    Logger logger;

    // State

    uint64_t lamport = 0;
    list<Order> orders;
    MPI_Status status;

public:

    uint64_t getLamport() {
        return lamport;
    }

    // Place a new order
    void makeOrder() {

        lamport += 1;

        // Create new order and place it on the list
        auto& newOrder = orders.emplace_back(id, lamport);

        //printf("[%lld] %05llu: Placing new Order(%lld, %llu)\n", id, lamport, id, lamport);
        logger.log() << "Placing Order(" << id << ", " << lamport << ")\n";

        // Send a new order to all the hunters
        for(int i = config.hunterMin; i <= config.hunterMax; i++) {
            lamport += 1;
            MPI_Send(&newOrder, 1, orderType, i, Tag::Order, MPI_COMM_WORLD);
        }

    }

    // Receive order from a Hunter
    void receiveOrderCompletion() {
        OrderCompletion receivedOrder;

        MPI_Recv(
            &receivedOrder,
            1,
            orderCompletionType,
            MPI_ANY_SOURCE,
            Tag::OrderCompletion,
            MPI_COMM_WORLD,
            &status);

        logger.log() << "✅ Received OrderCompletion("
            << receivedOrder.customer << ", "
            << receivedOrder.orderLamport << ", "
            << receivedOrder.lamport << ") from " << status.MPI_SOURCE << "\n";

        // Increment the lamport clock
        lamport = max(lamport, receivedOrder.lamport) + 1;

        // Remove the order from the list
        orders.remove_if([&receivedOrder](const Order& order) {
            return 
                order.customer == receivedOrder.customer &&
                order.lamport == receivedOrder.orderLamport;
        });
    }

public:

    Customer(int id, Config config) : logger(this, id, 'C'), config(config) {
        this->id = (int64_t)id;
        this->orderType = Order::datatype();
        this->orderCompletionType = OrderCompletion::datatype();
    };

    void loop() {
        while(true) {
            while(orders.size() < config.maxOrders) {
                makeOrder();
            }
            while(orders.size() > config.minOrders) {
                receiveOrderCompletion();
            }
        }
    }

};

#endif