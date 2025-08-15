// Comment out this line when using as DLL
#define flecs_systems_webgpu_STATIC
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
#elif defined(WEBGPU_BACKEND_DAWN)
    #include <webgpu.h>
#else
    /* Mock WebGPU types for non-WebGPU builds */
    typedef struct { unsigned long long id; } WGPUInstance;
    typedef struct { unsigned long long id; } WGPUDevice;
    typedef struct { unsigned long long id; } WGPUQueue;
    typedef struct { unsigned long long id; } WGPUBuffer;
    typedef struct { unsigned long long id; } WGPURenderPipeline;
    typedef struct { unsigned long long id; } WGPUShaderModule;
    typedef struct { unsigned long long id; } WGPUTexture;
    typedef struct { unsigned long long id; } WGPUBindGroup;
    typedef struct { unsigned long long id; } WGPUAdapter;
    typedef struct { unsigned long long id; } WGPUSurface;
    typedef struct { unsigned long long id; } WGPUSwapChain;
    typedef struct { unsigned long long id; } WGPUCommandEncoder;
    typedef struct { unsigned long long id; } WGPURenderPassEncoder;
    typedef struct { unsigned long long id; } WGPUBindGroupLayout;
    typedef struct { unsigned long long id; } WGPUPipelineLayout;
    typedef struct { unsigned long long id; } WGPUSampler;
    typedef struct { unsigned long long id; } WGPUTextureView;
    typedef struct { unsigned long long id; } WGPUCommandBuffer;
    
    typedef unsigned int WGPUBufferUsageFlags;
    typedef unsigned int WGPUTextureFormat;
    typedef unsigned int WGPUIndexFormat;
    typedef unsigned int WGPUVertexFormat;
    typedef unsigned int WGPUVertexStepMode;
    typedef unsigned int WGPUPrimitiveTopology;
    typedef unsigned int WGPUFrontFace;
    typedef unsigned int WGPUCullMode;
    typedef unsigned int WGPUBlendOperation;
    typedef unsigned int WGPUBlendFactor;
    typedef unsigned int WGPUCompareFunction;
    typedef unsigned int WGPULoadOp;
    typedef unsigned int WGPUStoreOp;
    typedef unsigned int WGPUShaderStage;
    typedef unsigned int WGPUBufferBindingType;
    typedef unsigned int WGPUColorWriteMask;
    typedef unsigned int WGPURequestAdapterStatus;
    typedef unsigned int WGPURequestDeviceStatus;
    typedef unsigned int WGPUErrorType;
    typedef unsigned int WGPUPowerPreference;
    typedef unsigned int WGPUSType;
    typedef unsigned int WGPUTextureDimension;
    typedef unsigned int WGPUTextureUsageFlags;
    
    #define WGPU_WHOLE_SIZE 0xFFFFFFFFFFFFFFFFULL
    
    /* Mock function declarations */
    static inline WGPUInstance wgpuCreateInstance(void* desc) { return (WGPUInstance){0}; }
    static inline void wgpuInstanceRelease(WGPUInstance instance) { (void)instance; }
    static inline void wgpuAdapterRelease(WGPUAdapter adapter) { (void)adapter; }
    static inline void wgpuDeviceRelease(WGPUDevice device) { (void)device; }
    static inline void wgpuQueueRelease(WGPUQueue queue) { (void)queue; }
    static inline void wgpuBufferRelease(WGPUBuffer buffer) { (void)buffer; }
    static inline void wgpuRenderPipelineRelease(WGPURenderPipeline pipeline) { (void)pipeline; }
    static inline void wgpuShaderModuleRelease(WGPUShaderModule module) { (void)module; }
    static inline void wgpuTextureRelease(WGPUTexture texture) { (void)texture; }
    static inline void wgpuBindGroupRelease(WGPUBindGroup bindGroup) { (void)bindGroup; }
    static inline void wgpuSurfaceRelease(WGPUSurface surface) { (void)surface; }
    static inline void wgpuSwapChainRelease(WGPUSwapChain swapChain) { (void)swapChain; }
    static inline void wgpuCommandEncoderRelease(WGPUCommandEncoder encoder) { (void)encoder; }
    static inline void wgpuRenderPassEncoderRelease(WGPURenderPassEncoder encoder) { (void)encoder; }
    static inline void wgpuCommandBufferRelease(WGPUCommandBuffer buffer) { (void)buffer; }
    static inline void wgpuTextureViewRelease(WGPUTextureView view) { (void)view; }
    static inline void wgpuBindGroupLayoutRelease(WGPUBindGroupLayout layout) { (void)layout; }
    static inline void wgpuPipelineLayoutRelease(WGPUPipelineLayout layout) { (void)layout; }
    
    /* For now, just print warnings for mock implementations */
    #define wgpuDeviceCreateBuffer(...) ((WGPUBuffer){0})
    #define wgpuDeviceCreateTexture(...) ((WGPUTexture){0})
    #define wgpuDeviceCreateShaderModule(...) ((WGPUShaderModule){0})
    #define wgpuDeviceCreateRenderPipeline(...) ((WGPURenderPipeline){0})
    #define wgpuDeviceCreateBindGroupLayout(...) ((WGPUBindGroupLayout){0})
    #define wgpuDeviceCreatePipelineLayout(...) ((WGPUPipelineLayout){0})
    #define wgpuDeviceCreateBindGroup(...) ((WGPUBindGroup){0})
    #define wgpuDeviceGetQueue(...) ((WGPUQueue){0})
    #define wgpuDeviceCreateCommandEncoder(...) ((WGPUCommandEncoder){0})
    #define wgpuBufferGetMappedRange(...) NULL
    #define wgpuBufferUnmap(...) do {} while(0)
    #define wgpuQueueWriteBuffer(...) do {} while(0)
    #define wgpuQueueSubmit(...) do {} while(0)
    #define wgpuDeviceSetUncapturedErrorCallback(...) do {} while(0)
    #define wgpuInstanceRequestAdapter(...) do {} while(0)
    #define wgpuInstanceCreateSurface(...) ((WGPUSurface){0})
    #define wgpuSwapChainGetCurrentTextureView(...) ((WGPUTextureView){0})
    #define wgpuCommandEncoderBeginRenderPass(...) ((WGPURenderPassEncoder){0})
    #define wgpuCommandEncoderFinish(...) ((WGPUCommandBuffer){0})
    #define wgpuRenderPassEncoderEnd(...) do {} while(0)
    #define wgpuRenderPassEncoderSetPipeline(...) do {} while(0)
    #define wgpuRenderPassEncoderSetVertexBuffer(...) do {} while(0)
    #define wgpuRenderPassEncoderSetIndexBuffer(...) do {} while(0)
    #define wgpuRenderPassEncoderDrawIndexed(...) do {} while(0)
    #define wgpuSwapChainPresent(...) do {} while(0)
    
    #warning "Building without WebGPU support - using mock implementation"
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

/* Forward declarations for components */

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
    
    /* Canvas properties */
    ecs_entity_t canvas_entity;        // Reference to EcsCanvas component
    uint32_t width, height;            // Current canvas dimensions
    WGPUTextureFormat surface_format;  // Surface pixel format
    
    /* Rendering queries */
    ecs_query_t *geometry_query;       // Query for renderable entities
    
    /* Resource management */
    ecs_allocator_t *allocator;        // Custom allocator for GPU resources
    ecs_vec_t render_batches;          // Batched rendering operations
    WGPURenderPipeline default_pipeline; // Default geometry rendering pipeline
    
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
