#pragma once
#include "timestamp.h"
#include <stdint.h>
#include "string_utils.h"

class Print;

// Simple worker node pattern - All nodes in the system will have do_work and debug_cmd/debug_print functions called continuously.
class WorkerNode {
public:
    virtual void do_work(Timestamp cur_time) {};

    virtual bool debug_cmd(HashedWord *input_words) { return false; };
    virtual void debug_print(Print &stream) {};

private:
    friend class Workforce;
    inline void set_next(WorkerNode *next) {
        // TODO: assert(!next_);
        next_ = next;
    }
    inline WorkerNode *next() {
        return next_;
    }
    WorkerNode *next_;
};

// This class is used to represent a list of WorkerNode-s and performs actions on all of them.
class Workforce : public WorkerNode {
public:
    void add_worker_to_front(WorkerNode *node) {
        node->set_next(worker_);
        worker_ = node;
    }
    void add_worker_to_back(WorkerNode *node) {
        if (!worker_) {
            worker_ = node;
        } else {
            WorkerNode *last_worker = worker_;
            while (last_worker->next())
                last_worker = last_worker->next();
            last_worker->set_next(node);
        }
    }

    virtual void do_work(Timestamp cur_time) {
        for (WorkerNode *node = worker_; node; node = node->next())
            node->do_work(cur_time);
    }
    virtual bool debug_cmd(HashedWord *input_words) {
        bool res = false;
        for (WorkerNode *node = worker_; node && !res; node = node->next())
            res = node->debug_cmd(input_words);
        return res;
    }
    virtual void debug_print(Print &stream) {
        for (WorkerNode *node = worker_; node; node = node->next())
            node->debug_print(stream);
    }

private:
    WorkerNode *worker_;
};

// We have a workforce singleton available.
extern Workforce workforce;

