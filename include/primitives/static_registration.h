#pragma once

// Class to help with (almost) compile-time class registration.
// To use, add static instance of this class to the class that needs registering and provide creation
// function. Then, you'll be able to iterate over all creation functions.
template<typename T>
struct StaticRegistrar {
    StaticRegistrar(const T &val)
        : val_(val) {
        next_ = instance;
        instance = this;
    }
    struct iterator {
        const T &operator*() const { return cur->val_; }
        iterator &operator++() { cur = cur->next_; return *this; }
        bool operator!=(const iterator &it) const { return cur != it.cur; }
        friend StaticRegistrar;
    protected:
        iterator(const StaticRegistrar *val): cur(val) {}
    private:
        const StaticRegistrar *cur;
    };
    struct iteration_range {
        iterator begin() { return instance; }
        iterator end() { return nullptr; }
    };
    static iteration_range iterate() { return {}; } 

private:
    const T val_;
    StaticRegistrar *next_;
    static StaticRegistrar *instance;
};

template<typename T>
StaticRegistrar<T> *StaticRegistrar<T>::instance = nullptr;
