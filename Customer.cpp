#include "Customer.hpp"

Customer::Customer(int64_t id, const Config& config) :
    id(id),
    config(config),
    types(),
    logger(this, id, "C")
    { };

uint64_t Customer::getLamport() {
    return lamport;
}

void Customer::loop() {
    while(true) {
        while(orders.size() < config.maxOrders) {
            placeOrder();
        }
        logger() << "Reached the limit of not completed orders, waiting for completions\n";
        while(orders.size() > config.minOrders) {
            receiveOrderCompletion();
        }
        logger() << "Fell below the lower limit of not completed orders, placing new orders...\n";
    }
}

// Place a new order
void Customer::placeOrder() {

    lamport += 1;

    // Create new order and place it on the list
    auto& newOrder = orders.emplace_back(id, lamport);

    logger() << "📤 Placing " << newOrder << "\n";

    // Send a new order to all the hunters
    for(int i = config.hunterMin; i <= config.hunterMax; i++) {
        lamport += 1;
        MPI_Send(&newOrder, 1, types.order, i, Tag::Order, MPI_COMM_WORLD);
    }

}

// Receive order from a Hunter
void Customer::receiveOrderCompletion() {
    OrderCompletion completion;

    MPI_Recv(
        &completion,
        1,
        types.orderCompletion,
        MPI_ANY_SOURCE,
        Tag::OrderCompletion,
        MPI_COMM_WORLD,
        &status);

    // Increment the lamport clock
    lamport = max(lamport, completion.lamport) + 1;

    logger() << "✅ Received " << completion << " from " << status.MPI_SOURCE << "\n";

    // Remove the order from the list
    orders.remove_if([&completion](const Order& order) {
        return 
            order.customer == completion.customer &&
            order.lamport == completion.orderLamport;
    });

}
