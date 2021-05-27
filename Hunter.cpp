#include "Hunter.hpp"

Hunter::Hunter(int64_t id, const Config& config) :
    id(id),
    config(config),
    types(),
    logger(this, id, " H")
    { }

uint64_t Hunter::getLamport() {
    const lock_guard<mutex> lock(lamportMutex);
    return lamport;
}

void Hunter::loop() {
    thread backgroundThread(&Hunter::loopBackground, this);
    loopForeground();
    backgroundThread.join();
}

// Increment the max(current, given) lamport value by 1
void Hunter::incrementLamport(uint64_t received) {
    const lock_guard<mutex> lock(lamportMutex);
    lamport = max(lamport, received) + 1;
}

// Increment the current lamport value by 1
void Hunter::incrementLamport() {
    const lock_guard<mutex> lock(lamportMutex);
    lamport += 1;
}

//
// Handling messages
//

// Handle the `Order` message
void Hunter::handleOrder() {
    Order order;
    MPI_Recv(&order, 1, types.order, MPI_ANY_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
    incrementLamport(order.lamport);
    
    {
        lock_guard<mutex> lock(stateMutex);

        logger() <<  "Received " << order << " - ";

        // If the order is rejected - remove from the list fo rejected
        if(auto it = find(rejected.begin(), rejected.end(), order); it != rejected.end()) {
            rejected.erase(it);
            logger << "removing from rejected list\n";
        } else {
            // Add the order to the list
            orders.push_back(order);
            logger << "adding to the list\n";

            // Notify the waiting thread
            if(state == HunterState::Waiting) {
                waitingForNewOrderWait.notify_one();
            }
        }
        
    }
}

// Handle the `OrderRequest` message
void Hunter::handleOrderRequest() {
    OrderRequest request;
    MPI_Recv(&request, 1, types.orderRequest, MPI_ANY_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
    incrementLamport(request.lamport);

    {
        lock_guard<mutex> lock(stateMutex);

        logger() << "Received " << request << " - ";

        // If we are getting the same order
        if(
            state == HunterState::GettingOrder &&
            orders.front().customer == request.orderCustomer &&
            orders.front().lamport == request.orderLamport) {

            logger << "same one as we are waiting for\n";

            // If another Hunter has lower priority - count it as an ACK
            if(request.lastOrderLamport > lastOrderLamport ||
                (request.lastOrderLamport == lastOrderLamport && id < status.MPI_SOURCE)) {
                
                logger() << "Another Hunter failed to get the order\n";

                gettingOrderRemaining -= 1;
                if(gettingOrderRemaining == 0) {
                    logger() << "Got the order\n";
                    gettingOrderGotOrder = true;
                    gettingOrderWait.notify_one();
                }
            }
            // If another hunter has higher priority - we failed
            else {
                gettingOrderRemaining = 0;
                gettingOrderGotOrder = false;
                gettingOrderWait.notify_one();
            }

        } else {

            logger << "we are not trying to get that order - sending back ACK\n";

            // We are not getting the same order -- we can send an ACK
            incrementLamport();
            OrderRequestAck ack { request.orderCustomer, request.orderLamport, getLamport() };
            MPI_Send(&ack,
                1,
                types.orderRequestAck,
                status.MPI_SOURCE,
                Tag::OrderRequestAck,
                MPI_COMM_WORLD);

            Order order { ack.orderCustomer, ack.orderLamport };
            if(auto it = find(orders.begin(), orders.end(), order); it != orders.end()) {
                orders.erase(it);
            } else if (find(rejected.begin(), rejected.end(), order) == rejected.end()) {
                rejected.push_back(order);
            }
        }
    }
}

// Handle the `OrderRequestAck` message
void Hunter::handleOrderRequestAck() {
    OrderRequestAck ack;
    MPI_Recv(&ack, 1, types.orderRequestAck, MPI_ANY_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
    incrementLamport(ack.lamport);
    
    {
        lock_guard<mutex> lock(stateMutex);

        logger() << "Received " << ack << "\n";

        // If we are trying to get the order and the ACK is about our order
        if(
            state == HunterState::GettingOrder &&
            ack.orderCustomer == orders.front().customer &&
            ack.orderLamport == orders.front().lamport) {
            
            gettingOrderRemaining -= 1;
            if(gettingOrderRemaining == 0) {
                logger() << "Got the order\n";
                gettingOrderGotOrder = true;
                gettingOrderWait.notify_one();
            }
        }
    }
}

// Handle the `StoreRequest` message
void Hunter::handleStoreRequest() {
    uint64_t requestLamport;
    MPI_Recv(&requestLamport, 1, MPI_UINT64_T, MPI_ANY_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
    incrementLamport(requestLamport);

    {
        lock_guard<mutex> lock(stateMutex);

        logger() << "Received StoreRequest(lamport = "
            << requestLamport << ") from " << status.MPI_SOURCE << "\n";

        if(
            // If in store...
            state == HunterState::InStore ||
            // ... or waiting to the store with (lower Lamport) OR (same Lamport, lower ID)
            (
                state == HunterState::GettingStore &&
                (
                    waitingForStoreLamport < requestLamport ||
                    (waitingForStoreLamport == requestLamport && id < status.MPI_SOURCE)
                )
            )
        ) {
            // Save on the list
            waitingForStoreHunters[status.MPI_SOURCE] = requestLamport;

        } else {
            // Send ACK
            logger() << "Sending store request ACK to "
                << status.MPI_SOURCE << "\n";
            incrementLamport();
            StoreRequestAck ack { requestLamport, getLamport() };
            MPI_Send(
                &ack,
                1,
                types.storeRequestAck,
                status.MPI_SOURCE,
                Tag::StoreRequestAck,
                MPI_COMM_WORLD);
        }
    }
}

// Handle the `StoreRequestAck` message
void Hunter::handleStoreRequestAck() {
    StoreRequestAck ack;
    MPI_Recv(&ack, 1, types.storeRequestAck, MPI_ANY_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
    incrementLamport(ack.lamport);

    {
        lock_guard<mutex> lock(stateMutex);

        logger() << "Received " << ack
            << " from " << status.MPI_SOURCE << "\n";

        if(state == HunterState::GettingStore && ack.requestLamport == waitingForStoreLamport) {
            waitingForStoreRemaining -= 1;
            if(waitingForStoreRemaining == 0) {
                logger() << "Can get into the store\n";
                waitingForStoreWait.notify_one();
            }
        }
    }
}

// Loop performed by the background (messaging thread)
void Hunter::loopBackground() {
    while(true) {
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch (status.MPI_TAG) {
        case Tag::Order: 
            handleOrder();
            break;
        case Tag::OrderRequest:
            handleOrderRequest();
            break;
        case Tag::OrderRequestAck:
            handleOrderRequestAck();
            break;
        case Tag::StoreRequest:
            handleStoreRequest();
            break;
        case Tag::StoreRequestAck:
            handleStoreRequestAck();
            break;
        default:
            logger() << "ERROR: Unknown message type: " << status.MPI_TAG << "\n";
            break;
        }
    }
}

//
// Main thread logic
//

// Loop performed by the main thread
void Hunter::loopForeground() {
    mt19937_64 generator;
    uniform_int_distribution<int> storeRandom(config.storeWaitMin, config.storeWaitMax);
    uniform_int_distribution<int> missionRandom(config.missionWaitMin, config.missionWaitMax);

    while(true) {

        {
            unique_lock<mutex> lock(stateMutex);

            // STATE: Waiting

            state = HunterState::Waiting;
            incrementLamport();
            logger() << "Waiting for new orders...\n";

            // Wait for a new order
            waitingForNewOrderWait.wait(lock, [this]() { return !orders.empty(); });
            

            // STATE: Getting order

            state = HunterState::GettingOrder;
            gettingOrderRemaining = config.hunterMax - config.hunterMin;
            gettingOrderGotOrder = false;
            incrementLamport();

            logger() << "Trying to get " << orders.front() << "\n";

            // Send a request to the other Hunters
            OrderRequest orderRequest { orders.front().customer, orders.front().lamport, lastOrderLamport, getLamport() };
            for(int i = config.hunterMin; i <= config.hunterMax; i++) {
                if(i == id) continue;
                MPI_Send(&orderRequest, 1, types.orderRequest, i, Tag::OrderRequest, MPI_COMM_WORLD);
                incrementLamport();
            }
            logger() << "Order request sent to other Hunters\n";

            // Wait for all responses
            gettingOrderWait.wait(lock, [this]{ return gettingOrderRemaining <= 0; });

            // If we didn't get the order - start over
            if(!gettingOrderGotOrder) {
                logger() << "Didn't get " << orders.front() << "\n";
                
                orders.pop_front();
                incrementLamport();
                continue;
            }
            logger() << "Got " << orders.front() << "\n";

            // STATE: getting store

            incrementLamport();
            state = HunterState::GettingStore;
            waitingForStoreLamport = getLamport();
            waitingForStoreRemaining = config.hunterMax - config.hunterMin + 1 - config.shopSize;
            waitingForStoreHunters.clear();
            

            logger() << "Trying to get into the store\n";

            // Send request to all the Hunters
            for(int i = config.hunterMin; i <= config.hunterMax; i++) {
                if(i == id) continue;
                MPI_Send(&waitingForStoreLamport, 1, MPI_UINT64_T, i, Tag::StoreRequest, MPI_COMM_WORLD);
            }
            logger() << "Store request to other Hunters sent, waiting...\n";

            // Wait for all responses
            waitingForStoreWait.wait(lock, [this] { return waitingForStoreRemaining <= 0; });


            // STATE: In store

            state = HunterState::InStore;
            incrementLamport();

            logger() << "🏪 In store, shopping\n";
        }

        // Sleep for a random number of seconds - don't block the mutex
        sleep(storeRandom(generator));

        {
            unique_lock<mutex> lock(stateMutex);

            incrementLamport();
            logger() << "Out of the store\n";

            // Send store ACKs to everyone on the waiting list
            StoreRequestAck ack { 0, getLamport() };
            for(const auto[hunter, lamport]: waitingForStoreHunters) {
                ack.requestLamport = lamport;
                MPI_Send(
                    &ack,
                    1,
                    types.storeRequestAck,
                    hunter,
                    Tag::StoreRequestAck,
                    MPI_COMM_WORLD);
            }
            waitingForStoreHunters.clear();
            logger() << "Send ACK to everyone on the store waiting list\n";


            // STATE: Mission

            state = HunterState::Mission;
            incrementLamport();

            logger() << "🚀 On a mission\n";
        }

        // Sleep for a random number of seconds - don't block the mutex
        sleep(missionRandom(generator));

        {
            unique_lock<mutex> lock(stateMutex);

            incrementLamport();
            logger() << "Mission finished\n";

            
            // Send order completion to the Customer
            incrementLamport();
            OrderCompletion completion { orders.front().customer, orders.front().lamport, getLamport() };
            MPI_Send(&completion,
                1,
                types.orderCompletion,
                orders.front().customer,
                Tag::OrderCompletion,
                MPI_COMM_WORLD);
            
            logger() << "Sent " << completion << "\n";

            orders.pop_front();
        }

        //logger() << "--> LOOP DONE \n";
        //sleep(300);

    }
}