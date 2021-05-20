#include "Customer.hpp"

Customer::Customer(int64_t id, const Config& config) :
    id(id),
    config(config),
    types(),
    logger(this, id, 'C')
    { };

uint64_t Customer::getLamport() {
    return lamport;
}

void Customer::loop() {
    while(true) {
        while(orders.size() < config.maxOrders) {
            makeOrder();
        }
        while(orders.size() > config.minOrders) {
            receiveOrderCompletion();
        }
    }
}

// Place a new order
void Customer::makeOrder() {

    lamport += 1;

    // Create new order and place it on the list
    auto& newOrder = orders.emplace_back(id, lamport);

    //printf("[%lld] %05llu: Placing new Order(%lld, %llu)\n", id, lamport, id, lamport);
    logger.log() << "Placing Order(" << id << ", " << lamport << ")\n";

    // Send a new order to all the hunters
    for(int i = config.hunterMin; i <= config.hunterMax; i++) {
        lamport += 1;
        MPI_Send(&newOrder, 1, types.order, i, Tag::Order, MPI_COMM_WORLD);
    }

}

// Receive order from a Hunter
void Customer::receiveOrderCompletion() {
    OrderCompletion receivedOrder;

    MPI_Recv(
        &receivedOrder,
        1,
        types.orderCompletion,
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
