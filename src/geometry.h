#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/vector.h"
#include "common.h"

class Print;
class HashedWord;

struct BaseStationGeometry {
    float mat[9];    // Normalized rotation matrix.
    float origin[3]; // Origin point

    void print_def(uint32_t idx, Print &err_stream);
    bool parse_def(HashedWord *input_words, Print &err_stream);
};


// Simple class for single-point sensors.
class PointGeometryBuilder
    : public WorkerNode
    , public Consumer<SensorAnglesFrame>
    , public Producer<ObjectGeometry>  {
public:
    PointGeometryBuilder(const Vector<BaseStationGeometry, num_base_stations> &base_stations, uint32_t input_idx);
    virtual void consume(const SensorAnglesFrame& f);
    virtual void do_work(Timestamp cur_time);

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print& stream);

private:
    const Vector<BaseStationGeometry, num_base_stations> &base_stations_;
    const uint32_t input_idx_;
    Timestamp last_success_;
};

// Helper node to convert coordinates to a different coordinate system.
class CoordinateSystemConverter
    : public WorkerNode
    , public Consumer<ObjectGeometry>
    , public Producer<ObjectGeometry> {
public:
    CoordinateSystemConverter(float m[9]);

    // Convert from standard Vive coordinate system to NED.
    // Needs angle between North and X axis, in degrees.
    static std::unique_ptr<CoordinateSystemConverter> NED(float angle_in_degrees);

    virtual void consume(const ObjectGeometry& geo);
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print& stream);

private:
    float mat_[9];
};