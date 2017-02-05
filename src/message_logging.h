// This file is used to define logging primitives for Producer-Consumer pairs.
// Each Producer node should call debug_cmd_producer function in its debug_cmd method to enable log of producing.
// Also, a printing function print_value should be defined for each data structure used for Producing.
#pragma once
#include "primitives/producer_consumer.h"
#include "primitives/circular_buffer.h"
#include "primitives/hash.h"
#include <Arduino.h>

template<typename T>
inline void print_value(Print &stream, const T& val); // Please specialize this function for your data below.

template<>
inline void print_value<Pulse>(Print &stream, const Pulse& val) {
    stream.printf("| %d %d %d ", val.input_idx, val.start_time.get_value_unsafe(usec), val.pulse_len.get_value(usec));
}

template<>
inline void print_value<SensorAnglesFrame>(Print &stream, const SensorAnglesFrame& val) {
    stream.printf("TODO");
}

template<>
inline void print_value<DataFrameBit>(Print &stream, const DataFrameBit& val) {
    stream.printf("TODO");
}

template<>
inline void print_value<DataFrame>(Print &stream, const DataFrame& frame) {
    for (uint32_t i = 0; i < frame.bytes.size(); i++)
        stream.printf(" %02X", frame.bytes[i]);
}

template<>
inline void print_value<ObjectGeometry>(Print &stream, const ObjectGeometry& val) {
    stream.printf("TODO");
}


template<typename T>
class PrintableProduceLogger : public ProduceLogger<T> {
public:
    virtual void print_logs(Print &stream) = 0;
};

template<typename T>
class CountingProducerLogger : public PrintableProduceLogger<T> {
public:
    CountingProducerLogger(const char *name, uint32_t idx): name_(name), idx_(idx), counter_(0) {}
    virtual void log_produce(const T& val) {
        counter_++;
    }
    virtual void print_logs(Print &stream) {
        uint32_t has_idx = idx_ != (uint32_t)-1;
        stream.printf("%s%.*u produced %d items", name_, has_idx, has_idx && idx_, counter_);
        counter_ = 0;
    }
private:
    const char *name_;
    uint32_t idx_;
    uint32_t counter_;
};

template<typename T>
class PrintingProducerLogger : public PrintableProduceLogger<T> {
public:
    PrintingProducerLogger(const char *name, uint32_t idx): name_(name), idx_(idx), counter_(0) {}
    virtual void log_produce(const T& val) {
        log_.enqueue(val);
        counter_++;
    }
    virtual void print_logs(Print &stream) {
        bool first = true;
        uint32_t has_idx = idx_ != (uint32_t)-1;
        T val;
        while (log_.dequeue(&val)) {
            if (first) {
                stream.printf("%s%.*u produced: ", name_, has_idx, has_idx && idx_);
                first = false;
            }
            print_value(stream, val);
        }
        if (!first) {
            stream.printf("(%d total)\n", counter_);
            counter_ = 0;
        }
    }
private:
    const char *name_;
    uint32_t idx_;
    uint32_t counter_;
    CircularBuffer<T, 16> log_;
};


template<typename T>
bool producer_debug_cmd(Producer<T> *producer, HashedWord *input_words, const char *name, uint32_t idx = -1) {
    switch (input_words[0].hash) {
        case static_hash("count"): delete producer->set_logger(new CountingProducerLogger<T>(name, idx)); return true;
        case static_hash("show"): delete producer->set_logger(new PrintingProducerLogger<T>(name, idx)); return true;
        case static_hash("off"): delete producer->set_logger(NULL); return true;
    }
    return false;
}

template<typename T>
void producer_debug_print(Producer<T> *producer, Print &stream) {
    if (PrintableProduceLogger<T> *logger = static_cast<PrintableProduceLogger<T>*>(producer->logger()))
        logger->print_logs(stream);
}
