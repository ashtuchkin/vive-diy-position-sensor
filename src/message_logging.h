// This file is used to define logging primitives for Producer-Consumer pairs.
// Each Producer node should call debug_cmd_producer function in its debug_cmd method to enable log of producing.
// Also, a printing function print_value should be defined for each data structure used for Producing.
// NOTE: This shouldn't be included in other header files. Just .cpp files.
#include "primitives/producer_consumer.h"
#include "primitives/circular_buffer.h"
#include "primitives/hash.h"
#include <Arduino.h>

// This function is used to print messages used in Producer-Consumer pattern.
// Please specialize this function for your messages below.
template<typename T>
inline void print_value(Print &stream, const T& val); 

template<>
inline void print_value<Pulse>(Print &stream, const Pulse& val) {
    stream.printf("\nsensor %d, time %dus, len %d ", val.input_idx, val.start_time.get_value(usec), val.pulse_len.get_value(usec));
}

template<>
inline void print_value<SensorAnglesFrame>(Print &stream, const SensorAnglesFrame& val) {
    stream.printf("\n%dms: cycle %u, angles ", 
                  val.time.get_value(msec), val.cycle_idx, val.phase_id);
    for (uint32_t i = 0; i < val.sensors.size(); i++) {
        auto sens = val.sensors[i];
        for (int32_t phase = 0; phase < num_cycle_phases; phase++) {
            int32_t phase_delta = phase - val.phase_id;
            if (phase_delta > 0) phase_delta -= num_cycle_phases;
            if (sens.updated_cycles[phase] == val.cycle_idx + phase_delta)
                stream.printf("%c%.4f ", (phase == val.phase_id) ? '*' : ' ', sens.angles[phase]);
            else
                stream.printf(" ------ ");
        }
    }
}

template<>
inline void print_value<DataFrameBit>(Print &stream, const DataFrameBit& val) {
    stream.printf("\n%dms: base %d, cycle %d, bit %d ", 
                  val.time.get_value(msec), val.base_station_idx, val.cycle_idx, val.bit);
}

template<>
inline void print_value<DataFrame>(Print &stream, const DataFrame& frame) {
    stream.printf("\n%dms: base %d, data ", frame.time.get_value(msec), frame.base_station_idx);
    for (uint32_t i = 0; i < frame.bytes.size(); i++)
        stream.printf("%02X ", frame.bytes[i]);
}

template<>
inline void print_value<ObjectPosition>(Print &stream, const ObjectPosition& val) {
    stream.printf("\n%dms: pos %.3f %.3f %.3f ", val.time.get_value(msec), val.pos[0], val.pos[1], val.pos[2]);
    if (val.q[0] != 1.0f)
        stream.printf("q %.3f %.3f %.3f %.3f ", val.q[0], val.q[1], val.q[2], val.q[3]);
}

template<>
inline void print_value<DataChunk>(Print &stream, const DataChunk& chunk) {
    stream.printf("\n%dms: stream %d, data ", chunk.time.get_value(msec), chunk.stream_idx);
    for (uint32_t i = 0; i < chunk.data.size(); i++)
        stream.printf("%02x ", chunk.data[i]);
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
        stream.printf("%s%.*u: %d items\n", name_, has_idx, has_idx && idx_, counter_);
        counter_ = 0;
    }
private:
    const char *name_;
    uint32_t idx_;
    uint32_t counter_;
};

template<typename T>
class PrintingProducerLogger : public PrintableProduceLogger<T> {
    constexpr static int kValuesToKeep = 16;
public:
    PrintingProducerLogger(const char *name, uint32_t idx): name_(name), idx_(idx), counter_(0) {}
    virtual void log_produce(const T& val) {
        if (log_.full()) 
            log_.pop_front();  // Ensure we have space to write, we're more interested in recent values.
        log_.enqueue(val);
        counter_++;
    }
    virtual void print_logs(Print &stream) {
        bool first = true;
        uint32_t has_idx = idx_ != (uint32_t)-1;
        stream.printf("%s%.*u: ", name_, has_idx, has_idx && idx_);
        while (!log_.empty()) {
            if (first) {
                first = false;
            } else {
                stream.printf("| ");
            }
            print_value(stream, log_.front());
            log_.pop_front();
        }
        if (!first) {
            stream.printf("(%d total)\n", counter_);
            counter_ = 0;
        } else {
            stream.printf("accumulating..\n");
        }
    }
private:
    const char *name_;
    uint32_t idx_;
    uint32_t counter_;
    CircularBuffer<T, kValuesToKeep> log_;
};


template<typename T>
bool producer_debug_cmd(Producer<T> *producer, HashedWord *input_words, const char *name, uint32_t idx = -1) {
    switch (input_words[0].hash) {
        case static_hash("count"): producer->set_logger(std::make_unique<CountingProducerLogger<T>>(name, idx)); return true;
        case static_hash("show"):  producer->set_logger(std::make_unique<PrintingProducerLogger<T>>(name, idx)); return true;
        case static_hash("off"):   producer->set_logger(nullptr); return true;
    }
    return false;
}

template<typename T>
void producer_debug_print(Producer<T> *producer, Print &stream) {
    if (PrintableProduceLogger<T> *logger = static_cast<PrintableProduceLogger<T>*>(producer->logger()))
        logger->print_logs(stream);
}
