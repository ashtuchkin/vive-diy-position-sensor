#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/float16.h"
#include "messages.h"

// See data frame description here: 
// https://github.com/nairol/LighthouseRedox/blob/master/docs/Light%20Emissions.md#ootx-frame
// https://github.com/nairol/LighthouseRedox/blob/master/docs/Base%20Station.md#base-station-info-block
struct DecodedDataFrame {
    // The current protocol version is 6. For older protocol versions the data structure might look different.
    static constexpr uint32_t cur_protocol = 6;

    uint8_t protocol : 6;       // Protocol version (bits 5..0).
    uint16_t fw_version : 10;   // Firmware version (bits 15..6).
    uint32_t id;                // Unique identifier of the base station (CRC32 of the 128-bit MCU UID)
    fp16 fcal_phase[2];       // "phase" - probably phase difference between real angle and measured.
    fp16 fcal_tilt[2];        // "tilt" - probably rotation of laser plane
    uint8_t sys_unlock_count;   // Lowest 8 bits of the rotor desynchronization counter
    uint8_t hw_version;         // Hardware version
    fp16 fcal_curve[2];       // "curve"
    int8_t accel_dir[3];        // Orientation vector, scaled so that largest component is always +-127.
    fp16 fcal_gibphase[2];    // "gibbous phase" (normalized angle)
    fp16 fcal_gibmag[2];      // "gibbous magnitude"
    uint8_t mode_current;       // Currently selected mode (default: 0=A, 1=B, 2=C)
    uint8_t sys_faults;         // "fault detect flags" (should be 0)
} __attribute__((packed));
static_assert(sizeof(DecodedDataFrame) == 33, "DataFrame should be 33 bytes long. Check it's byte-level packed.");

// Data Frame Decoder for one base station.
// TODO: Check CRC32.
class DataFrameDecoder
    : public WorkerNode
    , public Consumer<DataFrameBit>
    , public Producer<DataFrame> {
public:
    DataFrameDecoder(uint32_t base_station_idx);
    virtual void consume(const DataFrameBit &bit);

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

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
