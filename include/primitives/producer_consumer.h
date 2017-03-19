#pragma once
#include <cassert>
#include <memory>
#include <list>

// Very simple, low-overhead Producer/Consumer pattern.
// To use, inherit from Consumer/Producer as needed, implement consume() then call pipe() and produce()
template<typename T>
class Consumer {
public:
    // Function that needs to be overloaded by consumer to process message of given type.
    virtual void consume(const T& val) = 0;
};


template<typename T>
class ProduceLogger {
public:
    virtual void log_produce(const T& val) = 0;
    virtual ~ProduceLogger() = default;
};


// Produce a value of type T. Template parameter out_idx can be used if a class wants to produce 
// value of the same type to different output channels. In that case, you'll need to provide full signature, i.e.
// Producer<ValueType, 1>::produce(ValueType());
// Also, full signature is usually required if multiple types can be produced, unless "using Producer<T>::produce;" 
// is given for each type.
template<typename T, int out_idx = 0>
class Producer {
public:
    // This method connects producer to consumer.
    void pipe(Consumer<T> *consumer) {
        consumers_.push_front(consumer);
    }

    // This method should be called to send the value to all connected consumers.
    void produce(const T& val) {
        for (auto &cons : consumers_)
            cons->consume(val);
        if (logger_)
            logger_->log_produce(val);
    }

    // This method is an optimization so that the values which don't have consumers wouldn't have to be calculated.
    bool has_consumers() {
        return !consumers_.empty();
    }

    // Mostly used for debugging purposes, this method allows external parties to set up a logger for this producer.
    // Note: this class will own this logger and dispose it when not needed anymore.
    inline void set_logger(std::unique_ptr<ProduceLogger<T>> logger) {
        logger_ = std::move(logger);
    }
    inline ProduceLogger<T> *logger() const {
        return logger_.get();
    }

    virtual ~Producer() {};
private:
    std::list<Consumer<T> *> consumers_;
    std::unique_ptr<ProduceLogger<T>> logger_;
};

