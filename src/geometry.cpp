#include "geometry.h"
#include <arm_math.h>
#include "message_logging.h"
#include <assert.h>


// NE angle = Angle(North - X axis).
static const float ne_angle = 110.0f / 360.0f * (float)M_PI;
static float ned_rotation[9] = {
    // Convert Y up -> Z down; then rotate XY around Z clockwise and inverse X & Y
    -cosf(ne_angle), 0.0f,  sinf(ne_angle),
    -sinf(ne_angle), 0.0f, -cosf(ne_angle),
     0.0f,          -1.0f,            0.0f,
};
static arm_matrix_instance_f32 ned_rotation_mat = {3, 3, ned_rotation};
void convert_to_ned(const float pt[3], float (*ned)[3]);


PointGeometryBuilder::PointGeometryBuilder(const Vector<BaseStationGeometry, num_base_stations> &base_stations, uint32_t input_idx)
    : base_stations_(base_stations)
    , input_idx_(input_idx) {
    assert(base_stations.size() >= 2);
}


void PointGeometryBuilder::consume(const SensorAnglesFrame& f) {
    // First 2 angles - x, y of station B; second 2 angles - x, y of station C.
    // Y - Up;  X ->   Z v
    // Station ray is inverse Z axis.

    // Use only full frames (30Hz). In future, we can make this check configurable if 120Hz rate needed.
    if (f.phase_id != 3)  
        return;

    const float *angles = f.sensors[0].angles;
    //Serial.printf("Angles: %.4f %.4f %.4f %.4f\n", angles[0], angles[1], angles[2], angles[3]);

    vec3d ray1 = {};
    calc_ray_vec(base_stations_[0], angles[0], angles[1], ray1);
    //Serial.printf("Ray1: %f %f %f\n", ray1[0], ray1[1], ray1[2]);

    vec3d ray2 = {};
    calc_ray_vec(base_stations_[1], angles[2], angles[3], ray2);
    //Serial.printf("Ray2: %f %f %f\n", ray2[0], ray2[1], ray2[2]);


    ObjectGeometry geo = {.time = f.time, .xyz = {}, .q = {1.f, 0.f, 0.f, 0.f}};

    float dist;
    intersect_lines(base_stations_[0].origin, ray1, 
                    base_stations_[1].origin, ray2, &geo.xyz, &dist);

    // Convert to NED coordinate system.
    // TODO: Make configurable.
    {
        float ned[3];
        convert_to_ned(geo.xyz, &ned);
        std::swap(geo.xyz, ned);
    }

    produce(geo);
}

bool PointGeometryBuilder::debug_cmd(HashedWord *input_words) {
    if (*input_words == "geom#"_hash && input_words->idx == input_idx_) {
        input_words++;
        return producer_debug_cmd(this, input_words, "ObjectGeometry", input_idx_);
    }
    return false;
}
void PointGeometryBuilder::debug_print(Print& stream) {
    producer_debug_print(this, stream);
}


void convert_to_ned(const float pt[3], float (*ned)[3]) {
    // Convert to NED.
    arm_matrix_instance_f32 pt_mat = {3, 1, const_cast<float*>(pt)};
    arm_matrix_instance_f32 ned_mat = {3, 1, *ned};
    arm_mat_mult_f32(&ned_rotation_mat, &pt_mat, &ned_mat);
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

void calc_ray_vec(const BaseStationGeometry &bs, float angle1, float angle2, vec3d &res) {
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


// ======= BaseStationGeometry I/O ========================================
#include "Print.h"
#include "primitives/string_utils.h"

// Format:
// b<id> origin <x> <y> <z> matrix <9 floats>
void BaseStationGeometry::print_def(uint32_t idx, Print &stream) {
    stream.printf("b%d origin", idx);
    for (int j = 0; j < 3; j++)
        stream.printf(" %f", origin[j]);
    stream.printf(" matrix");
    for (int j = 0; j < 9; j++)
        stream.printf(" %f", mat[j]);
    stream.println();
}

bool BaseStationGeometry::parse_def(HashedWord *input_words, Print &stream) {
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
