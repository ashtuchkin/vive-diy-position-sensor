#include "main.h"

static const int vec3d_size = 3;
typedef float vec3d[vec3d_size];

struct lightsource {
    float mat[9];
    vec3d origin;
};

// NE angle = Angle(North - X axis).
static const float ne_angle = 110.0f / 360.0f * (float)M_PI;
static float ned_rotation[9] = {
    // Convert Y up -> Z down; then rotate XY around Z clockwise and inverse X & Y
    -cosf(ne_angle), 0.0f,  sinf(ne_angle),
    -sinf(ne_angle), 0.0f, -cosf(ne_angle),
     0.0f,          -1.0f,            0.0f,
};
static arm_matrix_instance_f32 ned_rotation_mat = {3, 3, ned_rotation};

static lightsource lightsources[2] = {{
    {  -0.88720f,  0.25875f, -0.38201f,
       -0.04485f,  0.77566f,  0.62956f,
        0.45920f,  0.57568f, -0.67656f},
    {  -1.28658f,  2.32719f, -2.04823f}
}, {
    {   0.52584f, -0.64026f,  0.55996f,
        0.01984f,  0.66739f,  0.74445f,
       -0.85035f, -0.38035f,  0.36364f},
    {   1.69860f,  2.62725f,  0.92969f}
}};

//static lightsource lightsources[2] = {{
//    { 0.27049f, -0.49175f, 0.82766f,
//     -0.00982f,  0.85825f, 0.51314f,
//     -0.96267f, -0.14693f, 0.22732f},
//    {1.85689, 2.48381, 0.62419}
//}, {
//    {-0.44084f,  0.47070f, -0.76426f,
//     -0.05869f,  0.83452f,  0.54784f,
//      0.89566f,  0.28637f, -0.34027f},
//    {-2.12202f, 2.20767f, -0.74908f}
//}};

bool intersect_lines(vec3d &orig1, vec3d &vec1, vec3d &orig2, vec3d &vec2, vec3d *res, float *dist);
void calc_ray_vec(lightsource &ls, float angle1, float angle2, vec3d &res);


void update_geometry(input_data& d) {
    // First 2 angles - x, y of station B; second 2 angles - x, y of station C.  Center is 4000. 180 deg = 8333.
    // Y - Up;  X ->   Z v
    // Station ray is inverse Z axis.

    //Serial.printf("Angles: %4d %4d %4d %4d\n", d.angle_lens[0], d.angle_lens[1], d.angle_lens[2], d.angle_lens[3]);
    float angles[num_cycle_phases];
    for (int i = 0; i < num_cycle_phases; i++)
        angles[i] = float(int(d.angle_lens[i]) - angle_center_len) * (float)PI / cycle_period;
    //Serial.printf("Angles: %.4f %.4f %.4f %.4f\n", angles[0], angles[1], angles[2], angles[3]);


    vec3d ray1 = {};
    calc_ray_vec(lightsources[0], angles[0], angles[1], ray1);
    //Serial.printf("Ray1: %f %f %f\n", ray1[0], ray1[1], ray1[2]);

    vec3d ray2 = {};
    calc_ray_vec(lightsources[1], angles[2], angles[3], ray2);
    //Serial.printf("Ray2: %f %f %f\n", ray2[0], ray2[1], ray2[2]);

    float dist;
    vec3d pt = {};
    intersect_lines(lightsources[0].origin, ray1, lightsources[1].origin, ray2, &pt, &dist);

    digitalWriteFast(LED_BUILTIN, (uint8_t)(!digitalReadFast(LED_BUILTIN)));

    // Convert to NED.
    vec3d ned = {};
    arm_matrix_instance_f32 pt_mat = {3, 1, pt};
    arm_matrix_instance_f32 ned_mat = {3, 1, ned};
    arm_mat_mult_f32(&ned_rotation_mat, &pt_mat, &ned_mat);

    Serial.printf("Position: %.3f %.3f %.3f ; dist= %.3f\n", ned[0], ned[1], ned[2], dist);
    send_mavlink_position(ned);
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

void calc_ray_vec(lightsource &ls, float angle1, float angle2, vec3d &res) {
    vec3d a = {arm_cos_f32(angle1), 0, -arm_sin_f32(angle1)};  // Normal vector to X plane
    vec3d b = {0, arm_cos_f32(angle2), arm_sin_f32(angle2)};   // Normal vector to Y plane

    vec3d ray = {};
    vec_cross_product(b, a, ray); // Intersection of two planes -> ray vector.
    float len = vec_length(ray);
    arm_scale_f32(ray, 1/len, ray, vec3d_size); // Normalize ray length.

    arm_matrix_instance_f32 source_rotation_matrix = {3, 3, ls.mat};
    arm_matrix_instance_f32 ray_vec = {3, 1, ray};
    arm_matrix_instance_f32 ray_rotated_vec = {3, 1, res};

    arm_mat_mult_f32(&source_rotation_matrix, &ray_vec, &ray_rotated_vec);
}


bool intersect_lines(vec3d &orig1, vec3d &vec1, vec3d &orig2, vec3d &vec2, vec3d *res, float *dist) {
    // Algoritm: http://geomalgorithms.com/a07-_distance.html#Distance-between-Lines

    vec3d w0 = {};
    arm_sub_f32(orig1, orig2, w0, vec3d_size);

    float a, b, c, d, e;
    arm_dot_prod_f32(vec1, vec1, vec3d_size, &a);
    arm_dot_prod_f32(vec1, vec2, vec3d_size, &b);
    arm_dot_prod_f32(vec2, vec2, vec3d_size, &c);
    arm_dot_prod_f32(vec1, w0, vec3d_size, &d);
    arm_dot_prod_f32(vec2, w0, vec3d_size, &e);

    float denom = a * c - b * b;
    if (fabs(denom) < 1e-5f)
        return false;

    // Closest point to 2nd line on 1st line
    float t1 = (b * e - c * d) / denom;
    vec3d pt1 = {};
    arm_scale_f32(vec1, t1, pt1, vec3d_size);
    arm_add_f32(pt1, orig1, pt1, vec3d_size);

    // Closest point to 1st line on 2nd line
    float t2 = (a * e - b * d) / denom;
    vec3d pt2 = {};
    arm_scale_f32(vec2, t2, pt2, vec3d_size);
    arm_add_f32(pt2, orig2, pt2, vec3d_size);

    // Result is in the middle
    vec3d tmp = {};
    arm_add_f32(pt1, pt2, tmp, vec3d_size);
    arm_scale_f32(tmp, 0.5f, *res, vec3d_size);

    // Dist is distance between pt1 and pt2
    arm_sub_f32(pt1, pt2, tmp, vec3d_size);
    *dist = vec_length(tmp);

    return true;
}


