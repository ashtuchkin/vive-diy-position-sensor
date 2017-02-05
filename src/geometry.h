#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/vector.h"
#include "common.h"

struct BaseStationGeometry {
    float mat[9];    // Normalized rotation matrix.
    float origin[3]; // Origin point
};


// Simple class for single-point sensors.
class PointGeometryBuilder
    : public WorkerNode
    , public Consumer<SensorAnglesFrame>
    , public Producer<ObjectGeometry>  {
public:
    PointGeometryBuilder(const Vector<BaseStationGeometry, num_base_stations> &base_stations, uint32_t input_idx);
    virtual void consume(const SensorAnglesFrame& f);

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print& stream);

private:
    const Vector<BaseStationGeometry, num_base_stations> &base_stations_;
    const uint32_t input_idx_;
};


static const int vec3d_size = 3;
typedef float vec3d[vec3d_size];

bool intersect_lines(const vec3d &orig1, const vec3d &vec1, const vec3d &orig2, const vec3d &vec2, vec3d *res, float *dist);
void calc_ray_vec(const BaseStationGeometry &bs, float angle1, float angle2, vec3d &res);
