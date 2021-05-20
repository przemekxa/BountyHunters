#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

class Loggable {
public:
    virtual uint64_t getLamport() = 0;
};

class Logger {
private:
    int64_t id;
    char type;
    ostream& stream;
    Loggable* object;

public:

    Logger(Loggable* object, int64_t id, char type, ostream& stream = cout) :
        object(object), id(id), type(type), stream(stream) { }

    Logger& log() {
        stream
            << type
            << " [" << setfill('0') << setw(2) << id << "] "
            << setfill('0') << setw(5) << object->getLamport() << ": ";

        return *this;
    }

    template <typename T>
    Logger& operator<< (const T& data) {
        stream << data;
        return *this;
    }

};

#endif