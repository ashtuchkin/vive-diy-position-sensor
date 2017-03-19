#include "data_frame_decoder.h"
#include "message_logging.h"

DataFrameDecoder::DataFrameDecoder(uint32_t base_station_idx)
    : base_station_idx_(base_station_idx)
    , prev_cycle_idx_(0)
    , preamble_len_(0)
    , skip_one_set_bit_(false)
    , cur_byte_(0)
    , cur_bit_idx_(0)
    , data_idx_(0)
    , data_frame_len_(0)
    , data_frame_{} {
}

void DataFrameDecoder::consume(const DataFrameBit& frame_bit) {
    // Check we have correct base station.
    if (frame_bit.base_station_idx != base_station_idx_)
        return;
    
    // Check we have correct cycle idx.
    if (prev_cycle_idx_ != 0 && frame_bit.cycle_idx != prev_cycle_idx_ + 1) {
        reset();
        return;
    }
    prev_cycle_idx_ = frame_bit.cycle_idx;

    // Start processing of next bit.
    bool bit = frame_bit.bit;

    // Skip one high bit if needed.
    if (skip_one_set_bit_) {
        if (bit)
            skip_one_set_bit_ = false;
        else
            reset();
        return;
    }

    // Searching for preamble (17 zeros).
    if (preamble_len_ != 17) {
        if (bit) {
            preamble_len_ = 0; // high bit -> reset preamble
        } else {
            preamble_len_++;
            if (preamble_len_ == 17) {
                skip_one_set_bit_ = true;
                data_idx_ = -2; // 2 bytes for data len.
            }
        }
        return;
    }

    // Read 8 bits into accumulator.
    cur_byte_ = (cur_byte_ << 1) | bit;
    cur_bit_idx_++;
    if (cur_bit_idx_ != 8)
        return;

    // Skip one high bit every 16 bit.
    if (data_idx_ & 1)
        skip_one_set_bit_ = true;

    // Process & write accumulated data byte
    if (data_idx_ < 0)
        data_frame_len_ |= cur_byte_ << ((2+data_idx_) * 8);
    else if (data_idx_ < data_frame_len_)
        data_frame_.bytes.push(cur_byte_);
    cur_byte_ = 0;
    cur_bit_idx_ = 0;
    data_idx_++;

    if (data_idx_ == (data_frame_len_|1) + 4) { // Round up to 16bit words, plus 4 byte CRC32 at the end (which we skip).
        // Received full frame - write it.
        data_frame_.time = frame_bit.time;
        data_frame_.base_station_idx = base_station_idx_;
        produce(data_frame_);
        reset();
    }
}

void DataFrameDecoder::reset() {
    prev_cycle_idx_ = 0;
    skip_one_set_bit_ = false;
    preamble_len_ = 0;
    cur_byte_ = 0;
    cur_bit_idx_ = 0;
    data_frame_len_ = 0;
    data_frame_.bytes.clear();
}

bool DataFrameDecoder::debug_cmd(HashedWord *input_words) {
    if (*input_words == "dataframe#"_hash && input_words->idx == base_station_idx_) {
        input_words++;
        return producer_debug_cmd(this, input_words, "DataFrame", base_station_idx_);
    }
    return false;
}

void DataFrameDecoder::debug_print(PrintStream &stream) {
    producer_debug_print(this, stream);
}
