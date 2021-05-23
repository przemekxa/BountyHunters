#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>

#include "Message.hpp"

using namespace std;

class Loggable {
public:
    virtual uint64_t getLamport() = 0;
};

class Logger {
private:
    int64_t id;
    string type;
    ostream& stream;
    Loggable* object;

public:

    Logger(Loggable* object, int64_t id, const string& type, ostream& stream = cout) :
        id(id), type(type), stream(stream), object(object) { }

    const Logger& operator()() const {
        stream
            << type
            << " [" << setfill('0') << setw(2) << id << "] "
            << setfill('0') << setw(5) << object->getLamport() << ": ";

        return *this;
    }

    const Logger& operator<< (const Order& order) const {
        stream << "Order(customer = " << order.customer <<
            ", orderLamport = " << order.lamport << ")";
        return *this;
    }

    const Logger& operator<< (const OrderCompletion& completion) const {
        stream << "OrderCompletion(customer = " << completion.customer <<
            ", orderLamport = " << completion.orderLamport << ")";
        return *this;
    }

    const Logger& operator<< (const OrderRequest& request) const {
        stream << "OrderRequest(customer = "
            << request.orderCustomer << ", orderLamport = " << request.orderLamport
            << ", lastOrder = " << request.lastOrderLamport << ")";
        return *this;
    }

    const Logger& operator<< (const OrderRequestAck& ack) const {
        stream << "OrderRequestAck(customer = "
            << ack.orderCustomer << ", orderLamport = "
            << ack.orderLamport << ")";
        return *this;
    }

    const Logger& operator<< (const StoreRequestAck& ack) const {
        stream << "StoreRequestAck(requestLamport = "
            << ack.requestLamport << ")";
        return *this;
    }

    template <typename T>
    const Logger& operator<< (const T& data) const {
        stream << data;
        return *this;
    }

};

#endif