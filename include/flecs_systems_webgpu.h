#ifndef FLECS_SYSTEMS_WEBGPU_H
#define FLECS_SYSTEMS_WEBGPU_H

/* Comment out this line when using as DLL */
#define flecs_systems_webgpu_STATIC

/* This generated file contains includes for project dependencies */
#ifndef FLECS_SYSTEMS_WEBGPU_BAKE_CONFIG_H
#define FLECS_SYSTEMS_WEBGPU_BAKE_CONFIG_H

/* Headers of public dependencies */
#include "flecs.h"
#include "flecs_components_gui.h"
#include "flecs_components_input.h"
#include "flecs_components_graphics.h"
#include "flecs_components_transform.h"
#include "flecs_components_geometry.h"
#include "flecs_systems_transform.h"

/* Convenience macro for exporting symbols */
#ifndef flecs_systems_webgpu_STATIC
#if defined(flecs_systems_webgpu_EXPORTS) && (defined(_MSC_VER) || defined(__MINGW32__))
  #define FLECS_SYSTEMS_WEBGPU_API __declspec(dllexport)
#elif defined(flecs_systems_webgpu_EXPORTS)
  #define FLECS_SYSTEMS_WEBGPU_API __attribute__((__visibility__("default")))
#elif defined(_MSC_VER)
  #define FLECS_SYSTEMS_WEBGPU_API __declspec(dllimport)
#else
  #define FLECS_SYSTEMS_WEBGPU_API
#endif
#else
  #define FLECS_SYSTEMS_WEBGPU_API
#endif

#endif

/* WebGPU Headers */
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #include <webgpu/webgpu.h>
#else
    #include <webgpu/webgpu.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup c_addons_systems_webgpu Systems WebGPU
 * @ingroup c_addons
 * WebGPU rendering system for web-native Flecs applications.
 *
 * @{
 */

/* Configuration */
#define WEBGPU_MAX_INSTANCES_PER_BATCH 1000
#define WEBGPU_MAX_LIGHTS 32
#define WEBGPU_SHADER_CACHE_SIZE 64

/* Forward declarations */
typedef struct WebGPURenderer WebGPURenderer;
typedef struct WebGPUGeometry WebGPUGeometry;
typedef struct WebGPUMaterial WebGPUMaterial;
typedef struct WebGPUQuery WebGPUQuery;

/* Component declarations */
ECS_COMPONENT_DECLARE(WebGPURenderer);
ECS_COMPONENT_DECLARE(WebGPUGeometry);
ECS_COMPONENT_DECLARE(WebGPUMaterial);
ECS_COMPONENT_DECLARE(WebGPUQuery);

/* Core renderer component */
typedef struct WebGPURenderer {
    /* WebGPU Core */
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUSwapChain swap_chain;
    
    /* Canvas properties */
    ecs_entity_t canvas_entity;        // Reference to EcsCanvas component
    uint32_t width, height;            // Current canvas dimensions
    WGPUTextureFormat surface_format;  // Surface pixel format
    
    /* Rendering queries */
    ecs_query_t *geometry_query;       // Query for renderable entities
    
    /* Resource management */
    ecs_allocator_t *allocator;        // Custom allocator for GPU resources
    ecs_vec_t render_batches;          // Batched rendering operations
    
    /* Frame state */
    WGPUCommandEncoder command_encoder;
    uint32_t frame_index;
    bool needs_resize;
} WebGPURenderer;

/* Geometry buffer management */
typedef struct WebGPUGeometry {
    /* Component type this geometry handles */
    ecs_id_t component_id;             // EcsBox, EcsRectangle, etc.
    
    /* Static geometry buffers */
    WGPUBuffer vertex_buffer;          // Vertex positions and normals
    WGPUBuffer index_buffer;           // Triangle indices
    uint32_t vertex_count;
    uint32_t index_count;
    
    /* Instance data buffers */
    WGPUBuffer instance_buffer;        // Transform matrices and colors
    ecs_allocator_t *allocator;
    ecs_vec_t transform_data;          // mat4 transforms (CPU)
    ecs_vec_t color_data;             // RGB colors (CPU)
    ecs_vec_t material_data;          // Material properties (CPU)
    uint32_t instance_count;
    
    /* Rendering state */
    WGPURenderPipeline pipeline;       // Graphics pipeline for this geometry
    WGPUBindGroup bind_group;         // Resource bindings
    
    /* Query for entities with this geometry type */
    ecs_query_t *query;
} WebGPUGeometry;

/* Material properties */
typedef struct WebGPUMaterial {
    /* PBR material properties */
    float base_color[4];               // RGBA base color
    float metallic;                    // Metallic factor [0,1]
    float roughness;                   // Roughness factor [0,1]
    float emissive_factor;             // Emissive intensity
    
    /* Texture resources */
    WGPUTexture diffuse_texture;       // Albedo/diffuse texture
    WGPUTexture normal_texture;        // Normal map texture
    WGPUTexture material_texture;      // Metallic/roughness/AO texture
    WGPUSampler texture_sampler;       // Texture sampling configuration
    
    /* GPU resources */
    WGPUBindGroup material_bind_group; // Material uniform bindings
    WGPUBuffer uniform_buffer;         // Material uniform data
} WebGPUMaterial;

/* Query component for dynamic geometry queries */
typedef struct WebGPUQuery {
    ecs_query_t *query;                // Flecs query handle
} WebGPUQuery;

/* Module import function */
FLECS_SYSTEMS_WEBGPU_API
void FlecsSystemsWebGPUImport(ecs_world_t *world);

/** @} */

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifndef FLECS_NO_CPP

namespace flecs {
namespace systems {

class webgpu {
public:
    webgpu(flecs::world& ecs) {
        // Load module contents
        FlecsSystemsWebGPUImport(ecs);

        // Bind C++ types with module contents
        ecs.module<flecs::systems::webgpu>();
    }
};

}
}

#endif // FLECS_NO_CPP
#endif // __cplusplus

#endif // FLECS_SYSTEMS_WEBGPU_H