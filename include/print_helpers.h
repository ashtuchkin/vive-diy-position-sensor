#pragma once
#include "primitives/producer_consumer.h"
#include "primitives/timestamp.h"
#include "messages.h"

// Implements PrintStream interface which sends DataChunks as a Producer.
// Note, the data is buffered and the last chunk is sent either on flush(), newline (if not binary), 
// or in destructor.
class DataChunkPrintStream : public PrintStream {
public:
    DataChunkPrintStream(Producer<DataChunk> *producer, Timestamp cur_time, uint32_t stream_idx = 0, bool binary = false)
        : producer_(producer), binary_(binary), chunk_{} 
    {
        chunk_.time = cur_time;
        chunk_.stream_idx = stream_idx;
    }

    ~DataChunkPrintStream() {
        flush();
    }

    // Main printing method overrides. We try to accumulate data for meaningful packets.
	virtual size_t write(const char *buffer, size_t size) {
        if (!size || !buffer) 
            return 0;

        // Write data to current chunk.
        while (true) {
            size_t available_space = chunk_.data.max_size() - chunk_.data.size();
            void *chunk_data_ptr = &chunk_.data[chunk_.data.size()];
            size_t write_bytes = std::min(size, available_space);
            chunk_.data.set_size(chunk_.data.size() + write_bytes);
            memcpy(chunk_data_ptr, buffer, write_bytes);
            if (size <= available_space) 
                break; // Data fits to the DataChunk
            flush(false);
            buffer += write_bytes;
            size -= write_bytes;
        }
        if (!binary_ && !chunk_.data.empty() && chunk_.data[chunk_.data.size()-1] == '\n')
            flush(true);  // Flush line-by-line for texts.
        return size;
    }
	
    void flush(bool last_chunk = true) {
        if (!chunk_.data.empty()) {
            chunk_.last_chunk = last_chunk;
            producer_->produce(chunk_);
            chunk_.data.clear();
        }
    }

private:
    Producer<DataChunk> *producer_;
    bool binary_;
    DataChunk chunk_;
};

// This class acts as a DataChunk consumer, accumulates the data, splits it into lines and 
// provides to child classes in consume_line() method.
class DataChunkLineSplitter : public Consumer<DataChunk> {
public:
    virtual void consume(const DataChunk& chunk) {
        for (uint32_t i = 0; i < chunk.data.size(); i++) {
            char c = chunk.data[i];
            if (c >= ' ' && c < 0x7F) {  // Printable characters
                if (input_str_buf_.size() < input_str_buf_.max_size() - 1)
                    input_str_buf_.push(c);
            } else if (c == '\r' || (c == '\n' && prev_char_ != '\r')) {  // Support \n, \r, \r\n line separators.
                input_str_buf_.push(0);
                input_str_buf_.clear();
                consume_line(&input_str_buf_[0], chunk.time);
            } else if (c == '\b' || c == 0x7F) {  // Backspace/Delete symbol
                if (input_str_buf_.size() > 0)
                    input_str_buf_.set_size(input_str_buf_.size()-1);
            }
            prev_char_ = c;
        }
    }

    // Implement this function to get parsed lines. Note, it's safe to change the characters in 'line', so
    // it's not const.
    virtual void consume_line(char *line, Timestamp time) = 0;

private:
    Vector<char, max_input_str_len> input_str_buf_;
    char prev_char_;
};
