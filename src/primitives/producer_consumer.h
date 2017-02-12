#pragma once

// Very simple, low-overhead Producer/Consumer pattern that allows 1:N fan-out and N:1 fan-in (not N:M)
// To use, inherit from Consumer/Producer as needed, implement consume() then call connect() and produce()
template<typename T>
class Consumer {
public:
    Consumer() : next_(0) {}

    // Function that needs to be overloaded by consumer to process message of given type.
    virtual void consume(const T& val) = 0;

private:
    // We're storing pointer to next consumer right here to avoid dynamic memory allocation.
    template<typename TT, int out_idx> friend class Producer;
    Consumer<T> *next_;
};


template<typename T>
class ProduceLogger {
public:
    virtual void log_produce(const T& val) = 0;
    virtual ~ProduceLogger() {}
};


// Produce a value of type T. Template parameter out_idx can be used if a class wants to produce 
// value of the same type to different output channels. In that case, you'll need to provide full signature, i.e.
// Producer<ValueType, 1>::produce(ValueType());
// Also, full signature is usually required if multiple types can be produced, unless "using Producer<T>::produce;" 
// is given for each type.
template<typename T, int out_idx = 0>
class Producer {
public:
    Producer() : consumer_(0), logger_(0) {}

    // This method 'connects' (subscribes) consumer to producer.
    void connect(Consumer<T> *consumer) {
        // TODO: assert(!consumer->next_cons_);  // This assert will make sure we don't have N:M fan-out.
        consumer->next_ = consumer_;
        consumer_ = consumer;
    }

    // Mostly used for debugging purposes, this method allows external parties to set up a logger for this producer.
    inline ProduceLogger<T> *set_logger(ProduceLogger<T> *logger) {
        auto *prev_logger = logger_;
        logger_ = logger;
        return prev_logger;
    }
    inline ProduceLogger<T> *logger() const {
        return logger_;
    }

    ~Producer() {
        if (logger_) {
            delete logger_;
            logger_ = 0;
        }
    }
protected:
    // This method should be called to send the value to all connected consumers.
    void produce(const T& val) {
        for (Consumer<T> *cons = consumer_; cons; cons = cons->next_)
            cons->consume(val);
        if (logger_)
            logger_->log_produce(val);
    }

    // This method is an optimization so that the values which don't have consumers wouldn't have to be calculated.
    bool has_consumers() {
        return consumer_;
    }

private:
    Consumer<T> *consumer_;
    ProduceLogger<T> *logger_;
};

