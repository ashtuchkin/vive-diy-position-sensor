#include "geometry.h"
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <arm_math.h>
#include "primitives/string_utils.h"
#include "message_logging.h"
#include "led_state.h"


bool intersect_lines(const vec3d &orig1, const vec3d &vec1, const vec3d &orig2, const vec3d &vec2, vec3d *res, float *dist);
void calc_ray_vec(const BaseStationGeometryDef &bs, float angle1, float angle2, vec3d &ray, vec3d &origin);


GeometryBuilder::GeometryBuilder(uint32_t idx, const GeometryBuilderDef &geo_def,
                                 const Vector<BaseStationGeometryDef, num_base_stations> &base_stations) 
    : object_idx_(idx)
    , base_stations_(base_stations)
    , def_(geo_def) {
    assert(idx < max_num_inputs);
    assert(geo_def.sensors.size() > 0);
    if (base_stations.size() != 2)
        throw_printf("2 base stations must be defined to use geometry builders.");
}


PointGeometryBuilder::PointGeometryBuilder(uint32_t idx, const GeometryBuilderDef &geo_def,
                                           const Vector<BaseStationGeometryDef, num_base_stations> &base_stations )
    : GeometryBuilder(idx, geo_def, base_stations)
    , pos_{Timestamp(), idx, FixLevel::kNoSignals, {0.f, 0.f, 0.f}, 0.f, {1.f, 0.f, 0.f, 0.f}} {
    assert(geo_def.sensors.size() == 1);
}


void PointGeometryBuilder::consume(const SensorAnglesFrame& f) {
    // First 2 angles - x, y of station B; second 2 angles - x, y of station C.
    // Coordinate system: Y - Up;  X ->  Z v  (to the viewer)
    // Station 'looks' to inverse Z axis (vector 0;0;-1).
    pos_.time = f.time;
    pos_.fix_level = f.fix_level;

    if (f.fix_level >= FixLevel::kCycleSynced) {
        const SensorLocalGeometry &sens_def = def_.sensors[0];
        const SensorAngles &sens = f.sensors[sens_def.input_idx];

        // Check angles are fresh enough.
        uint32_t max_stale = 0;
        for (int i = 0; i < num_cycle_phases; i++)
            max_stale = std::max(max_stale, f.cycle_idx - sens.updated_cycles[i]);

        if (max_stale < num_cycle_phases * 3) {  // We tolerate stale angles up to 2 cycles old.
            pos_.fix_level = (max_stale < num_cycle_phases) 
                                ? FixLevel::kFullFix : FixLevel::kStaleFix;

            vec3d ray1, ray2, origin1, origin2;
            calc_ray_vec(base_stations_[0], sens.angles[0], sens.angles[1], ray1, origin1);
            calc_ray_vec(base_stations_[1], sens.angles[2], sens.angles[3], ray2, origin2);
            
            intersect_lines(origin1, ray1, origin2, ray2, &pos_.pos, &pos_.pos_delta);

            // Translate object position depending on the position of sensor relative to object.
            for (int i = 0; i < vec3d_size; i++)
                pos_.pos[i] -= sens_def.pos[i];
            
        } else {
            // Angles too stale - cannot calculate position anymore.
            pos_.fix_level = FixLevel::kPartialVis;
        }
    }

    produce(pos_);
}

void PointGeometryBuilder::do_work(Timestamp cur_time) {
    // TODO: Make compatible with multiple geometry objects.
    set_led_state(pos_.fix_level >= FixLevel::kStaleFix ? LedState::kFixFound : LedState::kNoFix);
}

bool PointGeometryBuilder::debug_cmd(HashedWord *input_words) {
    if (*input_words == "geom#"_hash && input_words->idx == object_idx_) {
        input_words++;
        return producer_debug_cmd(this, input_words, "ObjectPosition", object_idx_);
    }
    return false;
}
void PointGeometryBuilder::debug_print(PrintStream &stream) {
    producer_debug_print(this, stream);
}

void vec_cross_product(const vec3d &a, const vec3d &b, vec3d &res) {
    res[0] = a[1]*b[2] - a[2]*b[1];
    res[1] = a[2]*b[0] - a[0]*b[2];
    res[2] = a[0]*b[1] - a[1]*b[0];
}

float vec_length(vec3d &vec) {
    float pow, res;

    arm_power_f32(vec, vec3d_size, &pow); // returns sum of squares
    arm_sqrt_f32(pow, &res);

    return res;
}

void calc_ray_vec(const BaseStationGeometryDef &bs, float angle1, float angle2, vec3d &res, vec3d &origin) {
    vec3d a = {arm_cos_f32(angle1), 0, -arm_sin_f32(angle1)};  // Normal vector to X plane
    vec3d b = {0, arm_cos_f32(angle2), arm_sin_f32(angle2)};   // Normal vector to Y plane

    vec3d ray = {};
    vec_cross_product(b, a, ray); // Intersection of two planes -> ray vector.
    float len = vec_length(ray);
    arm_scale_f32(ray, 1/len, ray, vec3d_size); // Normalize ray length.

    arm_matrix_instance_f32 source_rotation_matrix = {3, 3, const_cast<float*>(bs.mat)};
    arm_matrix_instance_f32 ray_vec = {3, 1, ray};
    arm_matrix_instance_f32 ray_rotated_vec = {3, 1, res};
    arm_mat_mult_f32(&source_rotation_matrix, &ray_vec, &ray_rotated_vec);

    // TODO: Make geometry adjustments within base station.
    vec3d rotated_origin_delta = {};
    //vec3d base_origin_delta = {-0.025f, -0.025f, 0.f};  // Rotors are slightly off center in base station.
    // arm_matrix_instance_f32 origin_vec = {3, 1, base_origin_delta};
    // arm_matrix_instance_f32 origin_rotated_vec = {3, 1, rotated_origin_delta};
    // arm_mat_mult_f32(&source_rotation_matrix, &origin_vec, &origin_rotated_vec);
    arm_add_f32(const_cast<vec3d&>(bs.origin), rotated_origin_delta, origin, vec3d_size);
}


bool intersect_lines(const vec3d &orig1, const vec3d &vec1, const vec3d &orig2, const vec3d &vec2, vec3d *res, float *dist) {
    // Algoritm: http://geomalgorithms.com/a07-_distance.html#Distance-between-Lines

    vec3d w0 = {};
    arm_sub_f32(const_cast<vec3d&>(orig1), const_cast<vec3d&>(orig2), w0, vec3d_size);

    float a, b, c, d, e;
    arm_dot_prod_f32(const_cast<vec3d&>(vec1), const_cast<vec3d&>(vec1), vec3d_size, &a);
    arm_dot_prod_f32(const_cast<vec3d&>(vec1), const_cast<vec3d&>(vec2), vec3d_size, &b);
    arm_dot_prod_f32(const_cast<vec3d&>(vec2), const_cast<vec3d&>(vec2), vec3d_size, &c);
    arm_dot_prod_f32(const_cast<vec3d&>(vec1), w0, vec3d_size, &d);
    arm_dot_prod_f32(const_cast<vec3d&>(vec2), w0, vec3d_size, &e);

    float denom = a * c - b * b;
    if (fabs(denom) < 1e-5f)
        return false;

    // Closest point to 2nd line on 1st line
    float t1 = (b * e - c * d) / denom;
    vec3d pt1 = {};
    arm_scale_f32(const_cast<vec3d&>(vec1), t1, pt1, vec3d_size);
    arm_add_f32(pt1, const_cast<vec3d&>(orig1), pt1, vec3d_size);

    // Closest point to 1st line on 2nd line
    float t2 = (a * e - b * d) / denom;
    vec3d pt2 = {};
    arm_scale_f32(const_cast<vec3d&>(vec2), t2, pt2, vec3d_size);
    arm_add_f32(pt2, const_cast<vec3d&>(orig2), pt2, vec3d_size);

    // Result is in the middle
    vec3d tmp = {};
    arm_add_f32(pt1, pt2, tmp, vec3d_size);
    arm_scale_f32(tmp, 0.5f, *res, vec3d_size);

    // Dist is distance between pt1 and pt2
    arm_sub_f32(pt1, pt2, tmp, vec3d_size);
    *dist = vec_length(tmp);

    return true;
}

// ======= CoordinateSystemConverter ==========================================
CoordinateSystemConverter::CoordinateSystemConverter(float mat[9]) {
    memcpy(mat_, mat, sizeof(mat_));
}

std::unique_ptr<CoordinateSystemConverter> CoordinateSystemConverter::create(CoordSysType type, const CoordSysDef& def) {
    switch (type) {
        case CoordSysType::kDefault: return nullptr; // Do nothing.
        case CoordSysType::kNED: return CoordinateSystemConverter::NED(def);
        default: throw_printf("Unknown coord sys type: %d", type);
    }
}

std::unique_ptr<CoordinateSystemConverter> CoordinateSystemConverter::NED(const CoordSysDef &def) {
    float ne_angle = def.ned.north_angle / 360.0f * (float)M_PI;
    float mat[9] = {
        // Convert Y up -> Z down; then rotate XY around Z clockwise and inverse X & Y
        -arm_cos_f32(ne_angle), 0.0f,  arm_sin_f32(ne_angle),
        -arm_sin_f32(ne_angle), 0.0f, -arm_cos_f32(ne_angle),
        0.0f,          -1.0f,            0.0f,
    };
    return std::make_unique<CoordinateSystemConverter>(mat);
}

void CoordinateSystemConverter::consume(const ObjectPosition& pos) {
    ObjectPosition out_pos(pos);

    // Convert position
    arm_matrix_instance_f32 src_mat = {3, 1, const_cast<float*>(pos.pos)};
    arm_matrix_instance_f32 dest_mat = {3, 1, out_pos.pos};
    arm_matrix_instance_f32 rotation_mat = {3, 3, mat_};
    arm_mat_mult_f32(&rotation_mat, &src_mat, &dest_mat);

    // TODO: Convert quaternion.
    assert(pos.q[0] == 1.0f);

    produce(out_pos);
}

bool CoordinateSystemConverter::debug_cmd(HashedWord *input_words) {
    if (*input_words++ == "coord"_hash) {
        return producer_debug_cmd(this, input_words, "ObjectPosition");
    }
    return false;
}
void CoordinateSystemConverter::debug_print(PrintStream &stream) {
    producer_debug_print(this, stream);
}


// ======= BaseStationGeometryDef I/O ===========================================
// Format: base<idx> origin <x> <y> <z> matrix <9x floats>

void BaseStationGeometryDef::print_def(uint32_t idx, PrintStream &stream) {
    stream.printf("base%d origin", idx);
    for (int j = 0; j < 3; j++)
        stream.printf(" %f", origin[j]);
    stream.printf(" matrix");
    for (int j = 0; j < 9; j++)
        stream.printf(" %f", mat[j]);
    stream.printf("\n");
}

bool BaseStationGeometryDef::parse_def(uint32_t idx, HashedWord *input_words, PrintStream &stream) {
    if (*input_words == "origin"_hash)
        input_words++;
    for (int i = 0; i < 3; i++, input_words++)
        if (!input_words->as_float(&origin[i])) {
            stream.printf("Invalid base station format\n"); return false;
        }
    if (*input_words == "matrix"_hash)
        input_words++;
    for (int i = 0; i < 9; i++, input_words++)
        if (!input_words->as_float(&mat[i])) {
            stream.printf("Invalid base station format\n"); return false;
        }
    return true;
}

// =======  GeometryBuilderDef I/O  ===========================================
// Format: object<idx> [ sensor<idx> <x> <y> <z> ]+

void GeometryBuilderDef::print_def(uint32_t idx, PrintStream &stream) {
    stream.printf("object%d", idx);
    for (uint32_t i = 0; i < sensors.size(); i++) {
        const SensorLocalGeometry &sensor = sensors[i];
        stream.printf(" sensor%d %.4f %.4f %.4f", sensor.input_idx, sensor.pos[0], sensor.pos[1], sensor.pos[2]);
    }
    stream.printf("\n");
}

bool GeometryBuilderDef::parse_def(uint32_t idx, HashedWord *input_words, PrintStream &err_stream) {
    sensors.clear();
    while (*input_words) {
        SensorLocalGeometry sensor;
        if (*input_words != "sensor#"_hash) {
            err_stream.printf("Sensor number (sensor<num>) required for object %d\n", idx); return false;
        }
        sensor.input_idx = input_words->idx;
        input_words++;
        if (!*input_words && sensors.size() == 0) { // Allow skipping coordinates for one-sensor geometry builder.
            sensor.pos[0] = sensor.pos[1] = sensor.pos[2] = 0.f;
            sensors.push(sensor);
            break;
        }
        for (int i = 0; i < vec3d_size; i++) {
            if (!input_words++->as_float(&sensor.pos[i])) {
                err_stream.printf("Invalid position coordinate in object %d\n", idx); return false;
            }
        }
        if (sensors.full()) {
            err_stream.printf("Too many inputs for object %d. Up to %d allowed.\n", idx, sensors.max_size()); return false;
        }
        sensors.push(sensor);
    }
    if (sensors.size() == 0) {
        err_stream.printf("At least one sensor input should be defined for object %d\n", idx); return false;
    }
    if (sensors.size() > 1) {
        err_stream.printf("Multi-sensor objects are currently not supported.\n"); return false;
    }
    return true;
}
