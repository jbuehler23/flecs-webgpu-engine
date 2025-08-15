/**
 * @file math/math_utils.c
 * @brief Math utility functions for 3D transformations.
 */

#include "../private_api.h"

/**
 * Set matrix to identity
 */
void mat4_identity(mat4 m) {
    for (int i = 0; i < 16; i++) {
        m[i] = 0.0f;
    }
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

/**
 * Multiply two 4x4 matrices
 */
void mat4_multiply(mat4 result, const mat4 a, const mat4 b) {
    mat4 temp;
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                temp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    
    for (int i = 0; i < 16; i++) {
        result[i] = temp[i];
    }
}

/**
 * Apply translation to matrix
 */
void mat4_translate(mat4 m, float x, float y, float z) {
    m[12] += x;
    m[13] += y;
    m[14] += z;
}

/**
 * Apply scale to matrix
 */
void mat4_scale(mat4 m, float x, float y, float z) {
    m[0] *= x;
    m[5] *= y;
    m[10] *= z;
}

/**
 * Create perspective projection matrix
 */
void mat4_perspective(mat4 m, float fov, float aspect, float near, float far) {
    float tan_half_fov = tanf(fov * 0.5f);
    
    mat4_identity(m);
    
    m[0] = 1.0f / (aspect * tan_half_fov);
    m[5] = 1.0f / tan_half_fov;
    m[10] = -(far + near) / (far - near);
    m[11] = -1.0f;
    m[14] = -(2.0f * far * near) / (far - near);
    m[15] = 0.0f;
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