/**
 * @file private_api.h
 * @brief Private API definitions for WebGPU systems module.
 */

#ifndef FLECS_SYSTEMS_WEBGPU_PRIVATE_API_H
#define FLECS_SYSTEMS_WEBGPU_PRIVATE_API_H

#include <flecs_systems_webgpu.h>

/* GLM for math operations */
#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

/* Math types and operations */
/* Always use cglm since it's available in the Flecs ecosystem */
#include <cglm/cglm.h>

/* Helper to convert cglm mat4 to flat array when needed */
static inline void mat4_to_array(mat4 src, float dst[16]) {
    memcpy(dst, src, sizeof(float) * 16);
}

/* Render batch for efficient GPU submission */
typedef struct {
    ecs_id_t geometry_type;           /* Component ID (EcsBox, EcsRectangle, etc.) */
    uint32_t instance_count;          /* Number of instances in this batch */
    
    /* Component data arrays */
    EcsTransform3 *transforms;        /* Transform matrices */
    EcsRgb *colors;                  /* Instance colors */
    void *geometry_data;             /* Geometry-specific data */
    
    /* GPU resources */
    WGPURenderPipeline pipeline;      /* Graphics pipeline */
    WGPUBindGroup bind_group;        /* Resource bindings */
    WGPUBuffer vertex_buffer;        /* Static vertex data */
    WGPUBuffer index_buffer;         /* Static index data */
    WGPUBuffer instance_buffer;      /* Instance data */
    uint32_t vertex_count;
    uint32_t index_count;
} webgpu_render_batch_t;

/* Resource management */
typedef struct {
    ecs_allocator_t *allocator;
    ecs_vec_t buffers;              /* WGPUBuffer pool */
    ecs_vec_t textures;             /* WGPUTexture pool */
    ecs_vec_t pipelines;            /* WGPURenderPipeline cache */
} webgpu_resource_pool_t;

/* Shader cache entry */
typedef struct {
    const char *name;
    WGPUShaderModule vertex_module;
    WGPUShaderModule fragment_module;
    WGPURenderPipeline pipeline;
} webgpu_shader_cache_entry_t;

/* Forward declarations */
struct WebGPURenderer;
struct WebGPUGeometry;

/* Internal API functions */

/* Geometry management */
void webgpu_geometry_import(ecs_world_t *world);
void webgpu_init_box_geometry(ecs_world_t *world, struct WebGPUGeometry *geometry);
void webgpu_init_rectangle_geometry(ecs_world_t *world, struct WebGPUGeometry *geometry);
void webgpu_populate_geometry_buffers(struct WebGPUGeometry *geometry, ecs_query_t *query);

/* Material system */
void webgpu_material_import(ecs_world_t *world);
void webgpu_create_default_material(ecs_world_t *world, struct WebGPUMaterial *material);

/* Rendering pipeline */
void webgpu_gather_geometry_batches(ecs_world_t *world, struct WebGPURenderer *renderer, ecs_query_t *query);
void webgpu_execute_render_batches(struct WebGPURenderer *renderer, WGPURenderPassEncoder render_pass);

/* Resource management */
webgpu_resource_pool_t* webgpu_create_resource_pool(ecs_allocator_t *allocator);
void webgpu_destroy_resource_pool(webgpu_resource_pool_t *pool);
WGPUBuffer webgpu_create_buffer(WGPUDevice device, size_t size, WGPUBufferUsage usage, const void *data);
void webgpu_update_buffer(WGPUDevice device, WGPUQueue queue, WGPUBuffer buffer, const void *data, size_t size, size_t offset);
WGPUTexture webgpu_create_texture_2d(WGPUDevice device, uint32_t width, uint32_t height, WGPUTextureFormat format);
WGPUTexture webgpu_create_depth_texture(WGPUDevice device, uint32_t width, uint32_t height);
WGPUTextureView webgpu_create_depth_texture_view(WGPUTexture depth_texture);
WGPURenderPipeline webgpu_create_geometry_pipeline(WGPUDevice device, WGPUShaderModule vertex_shader, WGPUShaderModule fragment_shader);
WGPUBuffer webgpu_create_camera_uniform_buffer(WGPUDevice device);
WGPUBuffer webgpu_create_light_uniform_buffer(WGPUDevice device);
WGPUBindGroup webgpu_create_camera_bind_group(WGPUDevice device, WGPUBindGroupLayout layout, WGPUBuffer uniform_buffer);
WGPUBindGroup webgpu_create_light_bind_group(WGPUDevice device, WGPUBindGroupLayout layout, WGPUBuffer uniform_buffer);

/* Shader utilities */
WGPUShaderModule webgpu_create_shader_module(WGPUDevice device, const char *wgsl_source);

/* Shader sources (embedded) */
extern const char *basic_vertex_shader_source;
extern const char *basic_fragment_shader_source;

/* Math utilities */
void mat4_identity(mat4 m);
void mat4_multiply(mat4 result, const mat4 a, const mat4 b);
void mat4_translate(mat4 m, float x, float y, float z);
void mat4_scale(mat4 m, float x, float y, float z);
void mat4_perspective(mat4 m, float fov, float aspect, float near, float far);
void vec3_copy(vec3 dst, const vec3 src);
void vec3_add(vec3 dst, const vec3 a, const vec3 b);

/* Platform-specific helpers */
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
void webgpu_setup_canvas_resize_callback(ecs_entity_t canvas_entity);
void webgpu_update_canvas_size(ecs_world_t *world, ecs_entity_t canvas_entity);
#endif

/* Debug and profiling */
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
void webgpu_profile_frame_start(const char* frame_name);
void webgpu_profile_frame_end(const char* frame_name);
#else
#define webgpu_profile_frame_start(name) do {} while(0)
#define webgpu_profile_frame_end(name) do {} while(0)
#endif

/* Constants */
#define WEBGPU_BYTES_PER_VERTEX (3 * sizeof(float))  /* Position only for now */
#define WEBGPU_BYTES_PER_INSTANCE (16 * sizeof(float) + 3 * sizeof(float))  /* mat4 + rgb */

#ifdef __cplusplus
}
#endif

#endif /* FLECS_SYSTEMS_WEBGPU_PRIVATE_API_H */