#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <cstdint>
#include <mpi.h>

using namespace std;

namespace Tag {
    // Order sent from the Customer to the Hunter
    const int Order = 100;
    // Completed order sent from the Hunter to the Customer
    const int OrderCompletion = 101;

    const int OrderRequest = 102;

    const int OrderRequestAck = 103;

    const int StoreRequest = 104;

    const int StoreRequestAck = 105;
}



struct Order {
    int64_t customer;
    uint64_t lamport;

    Order() : customer(0), lamport(0) {}

    Order(int64_t customer, uint64_t lamport) {
        this->customer = customer;
        this->lamport = lamport;
    }

    bool operator==(const Order& other) const {
        return customer == other.customer && lamport == other.lamport;
    }

    static MPI_Datatype datatype() {
        MPI_Datatype orderType;
        int lengths[2] = {1, 1};
        MPI_Datatype types[2] = { MPI_INT64_T, MPI_UINT64_T };

        MPI_Aint offsets[2];
        offsets[0] = offsetof(Order, customer);
        offsets[1] = offsetof(Order, lamport);

        MPI_Type_create_struct(2, lengths, offsets, types, &orderType);
        MPI_Type_commit(&orderType);

        return orderType;
    }

};

struct OrderCompletion {
    int64_t customer;
    uint64_t orderLamport;
    uint64_t lamport;

    static MPI_Datatype datatype() {
        MPI_Datatype orderType;
        int lengths[3] = {1, 1, 1};
        MPI_Datatype types[3] = { MPI_INT64_T, MPI_UINT64_T, MPI_UINT64_T };

        MPI_Aint offsets[3];
        offsets[0] = offsetof(OrderCompletion, customer);
        offsets[1] = offsetof(OrderCompletion, orderLamport);
        offsets[2] = offsetof(OrderCompletion, lamport);

        MPI_Type_create_struct(3, lengths, offsets, types, &orderType);
        MPI_Type_commit(&orderType);

        return orderType;
    }

};

struct OrderRequest {
    int64_t orderCustomer;
    uint64_t orderLamport;
    uint64_t lastOrderLamport;
    uint64_t lamport;

    static MPI_Datatype datatype() {
        MPI_Datatype orderType;
        int lengths[4] = {1, 1, 1, 1};
        MPI_Datatype types[4] = { MPI_INT64_T, MPI_UINT64_T, MPI_UINT64_T, MPI_UINT64_T };

        MPI_Aint offsets[4];
        offsets[0] = offsetof(OrderRequest, orderCustomer);
        offsets[1] = offsetof(OrderRequest, orderLamport);
        offsets[2] = offsetof(OrderRequest, lastOrderLamport);
        offsets[3] = offsetof(OrderRequest, lamport);

        MPI_Type_create_struct(4, lengths, offsets, types, &orderType);
        MPI_Type_commit(&orderType);

        return orderType;
    }
};

struct OrderRequestAck {
    int64_t orderCustomer;
    uint64_t orderLamport;
    uint64_t lamport;

    static MPI_Datatype datatype() {
        MPI_Datatype orderType;
        int lengths[3] = {1, 1, 1};
        MPI_Datatype types[3] = { MPI_INT64_T, MPI_UINT64_T, MPI_UINT64_T };

        MPI_Aint offsets[3];
        offsets[0] = offsetof(OrderRequestAck, orderCustomer);
        offsets[1] = offsetof(OrderRequestAck, orderLamport);
        offsets[2] = offsetof(OrderRequestAck, lamport);

        MPI_Type_create_struct(3, lengths, offsets, types, &orderType);
        MPI_Type_commit(&orderType);

        return orderType;
    }
};

struct StoreRequestAck {
    uint64_t requestLamport;
    uint64_t lamport;

    static MPI_Datatype datatype() {
        MPI_Datatype orderType;
        int lengths[2] = {1, 1};
        MPI_Datatype types[2] = { MPI_UINT64_T, MPI_UINT64_T };

        MPI_Aint offsets[2];
        offsets[0] = offsetof(StoreRequestAck, requestLamport);
        offsets[1] = offsetof(StoreRequestAck, lamport);

        MPI_Type_create_struct(2, lengths, offsets, types, &orderType);
        MPI_Type_commit(&orderType);

        return orderType;
    }
};

struct Datatype {
    MPI_Datatype order = Order::datatype();
    MPI_Datatype orderCompletion = OrderCompletion::datatype();
    MPI_Datatype orderRequest = OrderRequest::datatype();
    MPI_Datatype orderRequestAck = OrderRequestAck::datatype();
    MPI_Datatype storeRequestAck = StoreRequestAck::datatype();
};

#endif