#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "common.h"

// Data Frame Decoder for one base station.
// See data frame description here: 
// https://github.com/nairol/LighthouseRedox/blob/master/docs/Light%20Emissions.md#ootx-frame
// https://github.com/nairol/LighthouseRedox/blob/master/docs/Base%20Station.md#base-station-info-block
// Frame is 33 bytes long. NOTE: If we would want to decode float16, we can use ARM specific __fp16 type.
// TODO: Check CRC32.
class DataFrameDecoder
    : public WorkerNode
    , public Consumer<DataFrameBit>
    , public Producer<DataFrame> {
public:
    DataFrameDecoder(uint32_t base_station_idx);
    virtual void consume(const DataFrameBit &bit);

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print& stream);

private:
    void reset();

    const uint32_t base_station_idx_;  // Base station data we decode. 
    uint32_t prev_cycle_idx_;  // Last cycle idx for which we saw a bit.
    uint32_t preamble_len_;
    bool skip_one_set_bit_;
    uint8_t cur_byte_;
    uint32_t cur_bit_idx_;
    int32_t data_idx_;
    int32_t data_frame_len_;
    DataFrame data_frame_;
};
