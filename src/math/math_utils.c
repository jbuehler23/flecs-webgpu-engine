/**
 * @file math/math_utils.c
 * @brief Math utility functions for 3D transformations.
 */

#include "../private_api.h"

/**
 * Set matrix to identity
 */
void mat4_identity(mat4 m) {
    glm_mat4_identity(m);
}

/**
 * Multiply two 4x4 matrices
 */
void mat4_multiply(mat4 result, const mat4 a, const mat4 b) {
    glm_mat4_mul(a, b, result);
}

/**
 * Apply translation to matrix
 */
void mat4_translate(mat4 m, float x, float y, float z) {
    vec3 translation = {x, y, z};
    glm_translate(m, translation);
}

/**
 * Apply scale to matrix
 */
void mat4_scale(mat4 m, float x, float y, float z) {
    vec3 scale = {x, y, z};
    glm_scale(m, scale);
}

/**
 * Create perspective projection matrix
 */
void mat4_perspective(mat4 m, float fov, float aspect, float near, float far) {
    glm_perspective(fov, aspect, near, far, m);
}

/**
 * Copy vector
 */
void vec3_copy(vec3 dst, const vec3 src) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

/**
 * Add two vectors
 */
void vec3_add(vec3 dst, const vec3 a, const vec3 b) {
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
}