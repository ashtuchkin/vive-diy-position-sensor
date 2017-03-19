#pragma once
#include "timestamp.h"
#include "string_utils.h"
#include <list>
#include <memory>

// Simple worker node pattern. 
// To create a worker node, inherit from this interface and override functions needed.
class WorkerNode {
public:
    // This function is called continuously in a loop. Analogous to the loop() function in Arduino.
    virtual void do_work(Timestamp cur_time) {};

    // This function will be called once before starting to run the real pipeline.
    // Analogous to setup() function in Arduino, place any hard-to-undo hardware setup here.
    // It won't be called if the pipeline is created just for config validation. For common setup, use constructor.
    virtual void start() {};

    // Process debug command. Called when a command is available and should return true if this module
    // recognized and processed this command.
    virtual bool debug_cmd(HashedWord *input_words) { return false; };

    // Print current debugging information about this module. This function is called ~ every second.
    virtual void debug_print(PrintStream &stream) {};

    // Virtual destructor to help with correct deletions.
    virtual ~WorkerNode() = default;
};

// Pipeline defines an ordered list of WorkerNodes that work together.
// Pipeline 'owns' all the nodes and all of them will be deleted when the pipeline is deleted.
class Pipeline : public WorkerNode {
public:
    Pipeline() 
        : stop_requested_(false) 
    {}

    // Adding WorkerNodes to this pipeline. Note, after this Pipeline starts to own them and will
    // destruct them when needed. Suggested usage:
    // SpecializedNode *node = pipeline->add_front(std::make_unique<SpecializedNode>());
    template<typename T> 
    T *add_front(std::unique_ptr<T> node) { 
        T *res = node.get();
        nodes_.emplace_front(std::move(node));
        return res;
    }

    template<typename T> 
    T *add_back(std::unique_ptr<T> node) {
        T *res = node.get();
        nodes_.emplace_back(std::move(node));
        return res;
    }

    // Helper function to run this pipeline.
    void run() {
        // Setup all hardware changes needed to run this pipeline.
        start();

        // Process incoming work until finished.
        while (!stop_requested_)
            do_work(Timestamp::cur_time());
        
        // TODO someday: create method end(), symmetrical to start().
    }

    void stop() {
        stop_requested_ = true;
    }

    bool is_stop_requested() {
        return stop_requested_;
    }

    // Define WorkerNode functions to work on all nodes in order.
    virtual void do_work(Timestamp cur_time) {
        for (auto& node : nodes_) 
            node->do_work(cur_time);
    }
    virtual void start() {
        for (auto& node : nodes_)
            node->start();
    }
    virtual bool debug_cmd(HashedWord *input_words) {
        for (auto& node : nodes_) 
            if (node->debug_cmd(input_words))
                return true;
         return false; 
    }
    virtual void debug_print(PrintStream &stream) {
        for (auto& node : nodes_) 
            node->debug_print(stream); 
    }

protected:
    // Owning list of nodes. All nodes here will have the same lifecycle as the pipeline itself.
    std::list<std::unique_ptr<WorkerNode>> nodes_;

    // Flag that this pipeline should be stopped.
    bool stop_requested_;
};
