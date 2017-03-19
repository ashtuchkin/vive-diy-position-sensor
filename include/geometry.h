#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/vector.h"
#include "messages.h"

// Naive 3d vector type.
constexpr int vec3d_size = 3;
typedef float vec3d[vec3d_size];

// Stored definition of Base Stations
struct BaseStationGeometryDef {
    float mat[9];  // Normalized rotation matrix.
    vec3d origin;  // Origin point

    void print_def(uint32_t idx, PrintStream &stream);
    bool parse_def(uint32_t idx, HashedWord *input_words, PrintStream &err_stream);
};

struct SensorLocalGeometry {
    uint32_t input_idx;
    vec3d pos;  // Position of the sensor relative to the object.
};

// Stored definition of GeometryBuilder
struct GeometryBuilderDef {
    Vector<SensorLocalGeometry, 4> sensors;

    void print_def(uint32_t idx, PrintStream &stream);
    bool parse_def(uint32_t idx, HashedWord *input_words, PrintStream &err_stream);
};

// Parent, abstract class for GeometryBuilders.
class GeometryBuilder
    : public WorkerNode
    , public Consumer<SensorAnglesFrame>
    , public Producer<ObjectPosition>  {
public:
    GeometryBuilder(uint32_t idx, const GeometryBuilderDef &geo_def,
                    const Vector<BaseStationGeometryDef, num_base_stations> &base_stations);
    
protected:
    uint32_t object_idx_;
    const Vector<BaseStationGeometryDef, num_base_stations> &base_stations_;
    GeometryBuilderDef def_;
};

// Simple class for single-point sensors.
class PointGeometryBuilder : public GeometryBuilder {
public:
    PointGeometryBuilder(uint32_t idx, const GeometryBuilderDef &geo_def,
                         const Vector<BaseStationGeometryDef, num_base_stations> &base_stations);
    virtual void consume(const SensorAnglesFrame& f);
    virtual void do_work(Timestamp cur_time);

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

private:
    ObjectPosition pos_;
};


// Stored type and definition for CoordinateSystemConverter.
enum class CoordSysType {
    kDefault,  // No conversion.
    kNED,      // North-East-Down.
};

union CoordSysDef {
    struct {
        float north_angle;  // Angle between North and X axis, in degrees.
    } ned;
};

// Helper node to convert coordinates to a different coordinate system.
class CoordinateSystemConverter
    : public WorkerNode
    , public Consumer<ObjectPosition>
    , public Producer<ObjectPosition> {
public:
    CoordinateSystemConverter(float m[9]);

    static std::unique_ptr<CoordinateSystemConverter> create(CoordSysType type, const CoordSysDef& def);

    // Convert from standard Vive coordinate system to NED.
    // Needs angle between North and X axis, in degrees.
    static std::unique_ptr<CoordinateSystemConverter> NED(const CoordSysDef &def);

    virtual void consume(const ObjectPosition& geo);
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

private:
    float mat_[9];
};