/**
 * @file main.c
 * @brief WebGPU Systems module implementation for Flecs.
 */

#include "flecs_systems_webgpu.h"
/**
 * @file private_api.h
 * @brief Private API definitions for WebGPU systems module.
 */

#ifndef FLECS_SYSTEMS_WEBGPU_PRIVATE_API_H
#define FLECS_SYSTEMS_WEBGPU_PRIVATE_API_H


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

/* Component definitions */
ECS_COMPONENT_DECLARE(WebGPURenderer);
ECS_COMPONENT_DECLARE(WebGPUGeometry);
ECS_COMPONENT_DECLARE(WebGPUMaterial);
ECS_COMPONENT_DECLARE(WebGPUQuery);

/* Geometry type entities */
ECS_DECLARE(WebGPUBoxGeometry);
ECS_DECLARE(WebGPURectangleGeometry);

/* Static renderer instance (singleton pattern) */
static ecs_entity_t webgpu_renderer_instance = 0;

/**
 * WebGPU device error callback
 */
static void webgpu_device_error_callback(WGPUErrorType type, const char* message, void* userdata) {
    const char* type_str;
    switch (type) {
        case WGPUErrorType_Validation: type_str = "Validation"; break;
        case WGPUErrorType_OutOfMemory: type_str = "OutOfMemory"; break;
        case WGPUErrorType_Internal: type_str = "Internal"; break;
        case WGPUErrorType_Unknown: type_str = "Unknown"; break;
        case WGPUErrorType_DeviceLost: type_str = "DeviceLost"; break;
        default: type_str = "Unhandled"; break;
    }
    
    ecs_err("WebGPU %s Error: %s", type_str, message);
    
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    /* Log to browser console for debugging */
    EM_ASM({
        console.error('WebGPU Error:', UTF8ToString($0), UTF8ToString($1));
    }, type_str, message);
#endif
}

/**
 * Adapter request callback
 */
static void webgpu_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
    if (status != WGPURequestAdapterStatus_Success) {
        ecs_err("WebGPU: Failed to request adapter: %s", message ? message : "Unknown error");
        return;
    }
    
    ecs_trace("WebGPU: Adapter acquired successfully");
    
    /* Store adapter in renderer component */
    WebGPURenderer* renderer = (WebGPURenderer*)userdata;
    renderer->adapter = adapter;
}

/**
 * Device request callback
 */
static void webgpu_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
    if (status != WGPURequestDeviceStatus_Success) {
        ecs_err("WebGPU: Failed to request device: %s", message ? message : "Unknown error");
        return;
    }
    
    ecs_trace("WebGPU: Device acquired successfully");
    
    /* Store device in renderer component and set up error callback */
    WebGPURenderer* renderer = (WebGPURenderer*)userdata;
    renderer->device = device;
    renderer->queue = wgpuDeviceGetQueue(device);
    
    wgpuDeviceSetUncapturedErrorCallback(device, webgpu_device_error_callback, NULL);
}

/**
 * Initialize WebGPU renderer component
 */
static void webgpu_init_renderer(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    WebGPURenderer *renderer = ecs_field(it, WebGPURenderer, 0);
    EcsCanvas *canvas = ecs_field(it, EcsCanvas, 1);
    
    ecs_trace("WebGPU: Initializing renderer");
    
    for (int i = 0; i < it->count; i++) {
        /* Initialize allocator */
        renderer[i].allocator = ecs_os_malloc_t(ecs_allocator_t);
        flecs_allocator_init(renderer[i].allocator);
        
        /* Store canvas reference */
        renderer[i].canvas_entity = it->entities[i];
        renderer[i].width = canvas[i].width;
        renderer[i].height = canvas[i].height;
        
        /* Initialize render batches vector */
        ecs_vec_init_t(renderer[i].allocator, &renderer[i].render_batches, webgpu_render_batch_t, 0);
        
        /* Create WebGPU instance */
        WGPUInstanceDescriptor instance_desc = {
            .nextInChain = NULL,
        };
        
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
        renderer[i].instance = wgpuCreateInstance(NULL);
#else
        renderer[i].instance = wgpuCreateInstance(&instance_desc);
#endif
        
        if (!renderer[i].instance) {
            ecs_err("WebGPU: Failed to create instance");
            continue;
        }
        
        /* Create surface from canvas */
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
        WGPUSurfaceDescriptorFromCanvasHTMLSelector canvas_desc = {
            .chain = {
                .next = NULL,
                .sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector,
            },
            .selector = "#canvas", /* Default canvas selector */
        };
        
        WGPUSurfaceDescriptor surface_desc = {
            .nextInChain = (WGPUChainedStruct*)&canvas_desc,
            .label = "WebGPU Canvas Surface",
        };
        
        renderer[i].surface = wgpuInstanceCreateSurface(renderer[i].instance, &surface_desc);
#else
        /* Native surface creation would be platform-specific (GLFW, SDL, etc.) */
        ecs_warn("WebGPU: Native surface creation not implemented");
        renderer[i].surface = NULL;
#endif
        
        /* Request adapter */
        WGPURequestAdapterOptions adapter_options = {
            .nextInChain = NULL,
            .compatibleSurface = renderer[i].surface,
            .powerPreference = WGPUPowerPreference_HighPerformance,
            .forceFallbackAdapter = false,
        };
        
        wgpuInstanceRequestAdapter(renderer[i].instance, &adapter_options, 
                                  webgpu_adapter_callback, &renderer[i]);
        
        /* Create geometry query for renderable entities */
        renderer[i].geometry_query = ecs_query(world, {
            .expr = "[in] flecs.systems.webgpu.Geometry"
        });
        
        ecs_trace("WebGPU: Renderer initialization complete");
    }
    
    /* Disable system after first run */
    ecs_enable(world, it->system, false);
}

/**
 * Main rendering system
 */
static void webgpu_render_system(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    WebGPURenderer *renderer = ecs_field(it, WebGPURenderer, 0);
    WebGPUQuery *query = ecs_field(it, WebGPUQuery, 1);
    
    if (it->count > 1) {
        ecs_err("WebGPU: Multiple renderer instances not supported");
        return;
    }
    
    if (!renderer->device) {
        /* Device not ready yet */
        return;
    }
    
    /* Check for canvas resize */
    const EcsCanvas *canvas = ecs_get(world, renderer->canvas_entity, EcsCanvas);
    if (canvas && (renderer->width != canvas->width || renderer->height != canvas->height)) {
        renderer->width = canvas->width;
        renderer->height = canvas->height;
        renderer->needs_resize = true;
        ecs_trace("WebGPU: Canvas resize detected: %dx%d", renderer->width, renderer->height);
    }
    
    /* Get current surface texture (modern WebGPU approach) */
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(renderer->surface, &surface_texture);
    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        ecs_warn("WebGPU: Failed to get current surface texture");
        return;
    }
    
    WGPUTextureView back_buffer = wgpuTextureCreateView(surface_texture.texture, NULL);
    if (!back_buffer) {
        ecs_warn("WebGPU: Failed to create texture view");
        return;
    }
    
    /* Begin command encoding */
    WGPUCommandEncoderDescriptor encoder_desc = {
        .label = "WebGPU Frame Command Encoder",
    };
    renderer->command_encoder = wgpuDeviceCreateCommandEncoder(renderer->device, &encoder_desc);
    
    /* Begin render pass */
    WGPURenderPassColorAttachment color_attachment = {
        .view = back_buffer,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = { 0.1f, 0.2f, 0.3f, 1.0f }, /* Dark blue background */
    };
    
    WGPURenderPassDescriptor render_pass_desc = {
        .label = "WebGPU Main Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &color_attachment,
    };
    
    WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(renderer->command_encoder, &render_pass_desc);
    
    /* Gather and render geometry batches */
    webgpu_gather_geometry_batches(world, renderer, query->query);
    webgpu_execute_render_batches(renderer, render_pass);
    
    /* End render pass */
    wgpuRenderPassEncoderEnd(render_pass);
    
    /* Submit commands */
    WGPUCommandBufferDescriptor cmd_buffer_desc = {
        .label = "WebGPU Frame Commands",
    };
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(renderer->command_encoder, &cmd_buffer_desc);
    wgpuQueueSubmit(renderer->queue, 1, &command_buffer);
    
    /* Present frame */
    wgpuSurfacePresent(renderer->surface);
    
    /* Cleanup */
    wgpuTextureViewRelease(back_buffer);
    wgpuCommandBufferRelease(command_buffer);
    wgpuRenderPassEncoderRelease(render_pass);
    wgpuCommandEncoderRelease(renderer->command_encoder);
    
    renderer->frame_index++;
}

/**
 * Component lifecycle functions
 */
ECS_CTOR(WebGPURenderer, ptr, {
    ecs_os_memset_t(ptr, 0, WebGPURenderer);
})

ECS_DTOR(WebGPURenderer, ptr, {
    if (ptr->allocator) {
        ecs_vec_fini_t(ptr->allocator, &ptr->render_batches, webgpu_render_batch_t);
        flecs_allocator_fini(ptr->allocator);
        ecs_os_free(ptr->allocator);
    }
    
    if (ptr->geometry_query) {
        ecs_query_fini(ptr->geometry_query);
    }
    
    /* SwapChain no longer used in modern WebGPU */
    
    if (ptr->surface) {
        wgpuSurfaceRelease(ptr->surface);
    }
    
    if (ptr->queue) {
        wgpuQueueRelease(ptr->queue);
    }
    
    if (ptr->device) {
        wgpuDeviceRelease(ptr->device);
    }
    
    if (ptr->adapter) {
        wgpuAdapterRelease(ptr->adapter);
    }
    
    if (ptr->instance) {
        wgpuInstanceRelease(ptr->instance);
    }
})

/**
 * Module import function
 */
void FlecsSystemsWebGPUImport(ecs_world_t *world) {
    ECS_MODULE(world, FlecsSystemsWebGPU);
    
    /* Import dependencies */
    ECS_IMPORT(world, FlecsComponentsGui);
    ECS_IMPORT(world, FlecsComponentsInput);
    ECS_IMPORT(world, FlecsComponentsGraphics);
    ECS_IMPORT(world, FlecsComponentsTransform);
    ECS_IMPORT(world, FlecsComponentsGeometry);
    ECS_IMPORT(world, FlecsSystemsTransform);
    
    ecs_log_push();
    ecs_trace("WebGPU: Module import started");
    
    /* Define components */
    ECS_COMPONENT_DEFINE(world, WebGPURenderer);
    ECS_COMPONENT_DEFINE(world, WebGPUGeometry);
    ECS_COMPONENT_DEFINE(world, WebGPUMaterial);
    ECS_COMPONENT_DEFINE(world, WebGPUQuery);
    
    /* Set component hooks */
    ecs_set_hooks(world, WebGPURenderer, {
        .ctor = ecs_ctor(WebGPURenderer),
        .dtor = ecs_dtor(WebGPURenderer)
    });
    
    /* Initialize renderer system */
    ECS_SYSTEM(world, webgpu_init_renderer, EcsOnLoad,
        [out] !WebGPURenderer($), 
        [in] flecs.components.gui.Canvas);
        
    ecs_system(world, {
        .entity = webgpu_init_renderer,
        .immediate = true  /* Need direct world access for query creation */
    });
    
    /* Main rendering system */
    ECS_SYSTEM(world, webgpu_render_system, EcsOnStore,
        [in] WebGPURenderer($), 
        [in] WebGPUQuery);
    
    /* Create singleton query component */
    ecs_singleton_set(world, WebGPUQuery, {
        .query = ecs_query(world, { 
            .expr = "[in] flecs.systems.webgpu.Geometry" 
        })
    });
    
    /* Import geometry and material subsystems */
    webgpu_geometry_import(world);
    webgpu_material_import(world);
    
    ecs_trace("WebGPU: Module import completed");
    ecs_log_pop();
}
/**
 * @file geometry/geometry.c
 * @brief Geometry buffer management and primitive generation.
 */


/* Geometry component declarations */
ECS_COMPONENT_DECLARE(WebGPUGeometry);
ECS_DECLARE(WebGPUBoxGeometry);
ECS_DECLARE(WebGPURectangleGeometry);

/* Box vertex data (positions + normals) */
const float box_vertices[] = {
    /* Front face */
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    
    /* Back face */
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
    
    /* Top face */
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    
    /* Bottom face */
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    
    /* Right face */
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    
    /* Left face */
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
};

/* Box indices for triangle rendering */
const uint16_t box_indices[] = {
    0,  1,  2,    0,  2,  3,    /* Front face */
    4,  5,  6,    4,  6,  7,    /* Back face */
    8,  9,  10,   8,  10, 11,   /* Top face */
    12, 13, 14,   12, 14, 15,   /* Bottom face */
    16, 17, 18,   16, 18, 19,   /* Right face */
    20, 21, 22,   20, 22, 23    /* Left face */
};

/* Rectangle vertex data (quad in XY plane) */
const float rectangle_vertices[] = {
    /* Position      Normal        UV */
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
};

/* Rectangle indices */
const uint16_t rectangle_indices[] = {
    0, 1, 2,    0, 2, 3
};

/* Geometry data counts */
const uint32_t box_vertex_count = sizeof(box_vertices) / (8 * sizeof(float));
const uint32_t box_index_count = sizeof(box_indices) / sizeof(uint16_t);
const uint32_t rectangle_vertex_count = sizeof(rectangle_vertices) / (8 * sizeof(float));
const uint32_t rectangle_index_count = sizeof(rectangle_indices) / sizeof(uint16_t);

/**
 * Initialize box geometry
 */
void webgpu_init_box_geometry(ecs_world_t *world, WebGPUGeometry *geometry) {
    ecs_trace("WebGPU: Initializing box geometry");
    
    geometry->component_id = ecs_id(EcsBox);
    geometry->vertex_count = sizeof(box_vertices) / (8 * sizeof(float));  /* 8 floats per vertex */
    geometry->index_count = sizeof(box_indices) / sizeof(uint16_t);
    
    /* Create query for entities with EcsBox component */
    geometry->query = ecs_query(world, {
        .terms = {{
            .id = ecs_id(EcsTransform3),
            .inout = EcsIn
        }, {
            .id = ecs_id(EcsRgb),
            .inout = EcsIn
        }, {
            .id = ecs_id(EcsBox),
            .inout = EcsIn
        }}
    });
    
    if (!geometry->query) {
        ecs_err("WebGPU: Failed to create box geometry query");
        return;
    }
    
    /* Initialize instance data vectors */
    geometry->allocator = ecs_os_malloc_t(ecs_allocator_t);
    flecs_allocator_init(geometry->allocator);
    
    ecs_vec_init_t(geometry->allocator, &geometry->transform_data, mat4, 0);
    ecs_vec_init_t(geometry->allocator, &geometry->color_data, vec3, 0);
    ecs_vec_init_t(geometry->allocator, &geometry->material_data, float, 0);  /* Placeholder */
    
    ecs_trace("WebGPU: Box geometry initialized");
}

/**
 * Initialize rectangle geometry
 */
void webgpu_init_rectangle_geometry(ecs_world_t *world, WebGPUGeometry *geometry) {
    ecs_trace("WebGPU: Initializing rectangle geometry");
    
    geometry->component_id = ecs_id(EcsRectangle);
    geometry->vertex_count = sizeof(rectangle_vertices) / (8 * sizeof(float));
    geometry->index_count = sizeof(rectangle_indices) / sizeof(uint16_t);
    
    /* Create query for entities with EcsRectangle component */
    geometry->query = ecs_query(world, {
        .terms = {{
            .id = ecs_id(EcsTransform3),
            .inout = EcsIn
        }, {
            .id = ecs_id(EcsRgb),
            .inout = EcsIn
        }, {
            .id = ecs_id(EcsRectangle),
            .inout = EcsIn
        }}
    });
    
    if (!geometry->query) {
        ecs_err("WebGPU: Failed to create rectangle geometry query");
        return;
    }
    
    /* Initialize instance data vectors */
    geometry->allocator = ecs_os_malloc_t(ecs_allocator_t);
    flecs_allocator_init(geometry->allocator);
    
    ecs_vec_init_t(geometry->allocator, &geometry->transform_data, mat4, 0);
    ecs_vec_init_t(geometry->allocator, &geometry->color_data, vec3, 0);
    ecs_vec_init_t(geometry->allocator, &geometry->material_data, float, 0);
    
    ecs_trace("WebGPU: Rectangle geometry initialized");
}

/**
 * Populate geometry instance buffers from Flecs query
 */
void webgpu_populate_geometry_buffers(WebGPUGeometry *geometry, ecs_query_t *query) {
    if (!query || !geometry->allocator) {
        return;
    }
    
    const ecs_world_t *world = ecs_get_world(query);
    ecs_allocator_t *a = geometry->allocator;
    
    /* Clear previous instance data */
    ecs_vec_clear(&geometry->transform_data);
    ecs_vec_clear(&geometry->color_data);
    
    /* Iterate query results and gather instance data */
    ecs_iter_t it = ecs_query_iter(world, query);
    while (ecs_query_next(&it)) {
        EcsTransform3 *transforms = ecs_field(&it, EcsTransform3, 0);
        EcsRgb *colors = ecs_field(&it, EcsRgb, 1);
        void *geometry_data = ecs_field_w_size(&it, it.sizes[2], 2);
        
        uint32_t current_count = ecs_vec_count(&geometry->transform_data);
        
        /* Grow vectors to accommodate new instances */
        ecs_vec_grow_t(a, &geometry->transform_data, mat4, it.count);
        ecs_vec_grow_t(a, &geometry->color_data, vec3, it.count);
        
        /* Copy transform matrices */
        if (transforms) {
            for (int i = 0; i < it.count; i++) {
                mat4 *dst_transform = ecs_vec_get_t(&geometry->transform_data, mat4, current_count + i);
                
                /* Convert EcsTransform3 to mat4 */
                mat4_identity(*dst_transform);
                
                /* Apply translation */
                (*dst_transform)[12] = transforms[i].value[3][0];  /* X */
                (*dst_transform)[13] = transforms[i].value[3][1];  /* Y */
                (*dst_transform)[14] = transforms[i].value[3][2];  /* Z */
                
                /* Copy rotation and scale from transform matrix */
                for (int row = 0; row < 3; row++) {
                    for (int col = 0; col < 3; col++) {
                        (*dst_transform)[row * 4 + col] = transforms[i].value[row][col];
                    }
                }
            }
        }
        
        /* Copy colors */
        if (colors) {
            if (ecs_field_is_self(&it, 1)) {
                /* Per-entity colors */
                for (int i = 0; i < it.count; i++) {
                    vec3 *dst_color = ecs_vec_get_t(&geometry->color_data, vec3, current_count + i);
                    (*dst_color)[0] = colors[i].r;
                    (*dst_color)[1] = colors[i].g;
                    (*dst_color)[2] = colors[i].b;
                }
            } else {
                /* Shared color for all entities */
                for (int i = 0; i < it.count; i++) {
                    vec3 *dst_color = ecs_vec_get_t(&geometry->color_data, vec3, current_count + i);
                    (*dst_color)[0] = colors[0].r;
                    (*dst_color)[1] = colors[0].g;
                    (*dst_color)[2] = colors[0].b;
                }
            }
        }
        
        /* Apply geometry-specific scaling */
        if (geometry->component_id == ecs_id(EcsBox)) {
            EcsBox *boxes = (EcsBox*)geometry_data;
            for (int i = 0; i < it.count; i++) {
                mat4 *transform = ecs_vec_get_t(&geometry->transform_data, mat4, current_count + i);
                
                /* Scale by box dimensions */
                (*transform)[0] *= boxes[i].width;   /* Scale X */
                (*transform)[5] *= boxes[i].height;  /* Scale Y */
                (*transform)[10] *= boxes[i].depth;  /* Scale Z */
            }
        } else if (geometry->component_id == ecs_id(EcsRectangle)) {
            EcsRectangle *rectangles = (EcsRectangle*)geometry_data;
            for (int i = 0; i < it.count; i++) {
                mat4 *transform = ecs_vec_get_t(&geometry->transform_data, mat4, current_count + i);
                
                /* Scale by rectangle dimensions */
                (*transform)[0] *= rectangles[i].width;   /* Scale X */
                (*transform)[5] *= rectangles[i].height;  /* Scale Y */
            }
        }
    }
    
    geometry->instance_count = ecs_vec_count(&geometry->transform_data);
}

/**
 * Component lifecycle functions
 */
ECS_CTOR(WebGPUGeometry, ptr, {
    ecs_os_memset_t(ptr, 0, WebGPUGeometry);
})

ECS_DTOR(WebGPUGeometry, ptr, {
    if (ptr->allocator) {
        ecs_vec_fini_t(ptr->allocator, &ptr->transform_data, mat4);
        ecs_vec_fini_t(ptr->allocator, &ptr->color_data, vec3);
        ecs_vec_fini_t(ptr->allocator, &ptr->material_data, float);
        flecs_allocator_fini(ptr->allocator);
        ecs_os_free(ptr->allocator);
    }
    
    if (ptr->query) {
        ecs_query_fini(ptr->query);
    }
    
    if (ptr->vertex_buffer.id) {
        wgpuBufferRelease(ptr->vertex_buffer);
    }
    
    if (ptr->index_buffer.id) {
        wgpuBufferRelease(ptr->index_buffer);
    }
    
    if (ptr->instance_buffer.id) {
        wgpuBufferRelease(ptr->instance_buffer);
    }
    
    if (ptr->pipeline.id) {
        wgpuRenderPipelineRelease(ptr->pipeline);
    }
    
    if (ptr->bind_group.id) {
        wgpuBindGroupRelease(ptr->bind_group);
    }
})

/**
 * System to populate geometry buffers each frame
 */
static void webgpu_populate_geometry_system(ecs_iter_t *it) {
    WebGPUGeometry *geometry = ecs_field(it, WebGPUGeometry, 0);
    
    for (int i = 0; i < it->count; i++) {
        if (geometry[i].query) {
            webgpu_populate_geometry_buffers(&geometry[i], geometry[i].query);
        }
    }
}

/**
 * Geometry subsystem import
 */
void webgpu_geometry_import(ecs_world_t *world) {
    ecs_trace("WebGPU: Importing geometry subsystem");
    
    /* Define geometry component with lifecycle hooks */
    ecs_set_hooks(world, WebGPUGeometry, {
        .ctor = ecs_ctor(WebGPUGeometry),
        .dtor = ecs_dtor(WebGPUGeometry)
    });
    
    /* Create geometry type entities */
    ECS_ENTITY_DEFINE(world, WebGPUBoxGeometry, WebGPUGeometry);
    ECS_ENTITY_DEFINE(world, WebGPURectangleGeometry, WebGPUGeometry);
    
    /* Initialize geometry instances */
    WebGPUGeometry *box_geom = ecs_ensure(world, WebGPUBoxGeometry, WebGPUGeometry);
    webgpu_init_box_geometry(world, box_geom);
    ecs_modified(world, WebGPUBoxGeometry, WebGPUGeometry);
    
    WebGPUGeometry *rect_geom = ecs_ensure(world, WebGPURectangleGeometry, WebGPUGeometry);
    webgpu_init_rectangle_geometry(world, rect_geom);
    ecs_modified(world, WebGPURectangleGeometry, WebGPUGeometry);
    
    /* System to populate geometry buffers */
    ECS_SYSTEM(world, webgpu_populate_geometry_system, EcsPreStore, 
        [inout] WebGPUGeometry);
    
    ecs_trace("WebGPU: Geometry subsystem imported");
}
/**
 * @file math/math_utils.c
 * @brief Math utility functions for 3D transformations.
 */


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
/**
 * @file rendering/render_system.c
 * @brief Core rendering system implementation for WebGPU.
 */


/**
 * Create instance data buffer for a batch of entities
 */
static WGPUBuffer create_instance_buffer(WGPUDevice device, 
                                       const EcsTransform3 *transforms,
                                       const EcsRgb *colors,
                                       uint32_t count) {
    if (!device || count == 0) {
        return (WGPUBuffer){0};
    }
    
    size_t buffer_size = count * WEBGPU_BYTES_PER_INSTANCE;
    float *instance_data = ecs_os_malloc(buffer_size);
    if (!instance_data) {
        ecs_err("Failed to allocate instance data buffer");
        return (WGPUBuffer){0};
    }
    
    /* Pack transform matrices and colors into instance buffer */
    for (uint32_t i = 0; i < count; i++) {
        float *dst = &instance_data[i * 19]; /* 16 + 3 floats per instance */
        
        /* Transform matrix (16 floats) */
        if (transforms) {
            /* Convert EcsTransform3 to 4x4 matrix */
            mat4 transform_matrix;
            mat4_identity(transform_matrix);
            mat4_translate(transform_matrix, 
                          transforms[i].value[0], 
                          transforms[i].value[1], 
                          transforms[i].value[2]);
            memcpy(dst, transform_matrix, 16 * sizeof(float));
        } else {
            /* Identity matrix */
            mat4 identity;
            mat4_identity(identity);
            memcpy(dst, identity, 16 * sizeof(float));
        }
        
        /* Color (3 floats) */
        if (colors) {
            dst[16] = colors[i].r;
            dst[17] = colors[i].g;
            dst[18] = colors[i].b;
        } else {
            /* Default white color */
            dst[16] = 1.0f;
            dst[17] = 1.0f;
            dst[18] = 1.0f;
        }
    }
    
    /* Create GPU buffer */
    WGPUBuffer buffer = webgpu_create_buffer(device, buffer_size, 
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst, instance_data);
    
    ecs_os_free(instance_data);
    return buffer;
}

/**
 * Get geometry buffers for a specific geometry type
 */
static bool get_geometry_buffers(ecs_world_t *world,
                                WebGPURenderer *renderer,
                                ecs_id_t geometry_type,
                                WGPUBuffer *vertex_buffer,
                                WGPUBuffer *index_buffer,
                                uint32_t *vertex_count,
                                uint32_t *index_count) {
    
    if (geometry_type == ecs_id(EcsBox)) {
        /* Box geometry from geometry.c */
        extern const float box_vertices[];
        extern const uint16_t box_indices[];
        extern const uint32_t box_vertex_count;
        extern const uint32_t box_index_count;
        
        *vertex_buffer = webgpu_create_buffer(renderer->device,
            box_vertex_count * 8 * sizeof(float), /* pos(3) + normal(3) + uv(2) */
            WGPUBufferUsage_Vertex, box_vertices);
            
        *index_buffer = webgpu_create_buffer(renderer->device,
            box_index_count * sizeof(uint16_t),
            WGPUBufferUsage_Index, box_indices);
            
        *vertex_count = box_vertex_count;
        *index_count = box_index_count;
        return true;
    }
    
    if (geometry_type == ecs_id(EcsRectangle)) {
        /* Rectangle geometry from geometry.c */
        extern const float rectangle_vertices[];
        extern const uint16_t rectangle_indices[];
        extern const uint32_t rectangle_vertex_count;
        extern const uint32_t rectangle_index_count;
        
        *vertex_buffer = webgpu_create_buffer(renderer->device,
            rectangle_vertex_count * 8 * sizeof(float),
            WGPUBufferUsage_Vertex, rectangle_vertices);
            
        *index_buffer = webgpu_create_buffer(renderer->device,
            rectangle_index_count * sizeof(uint16_t),
            WGPUBufferUsage_Index, rectangle_indices);
            
        *vertex_count = rectangle_vertex_count;
        *index_count = rectangle_index_count;
        return true;
    }
    
    ecs_warn("WebGPU: Unsupported geometry type: %s", ecs_get_name(world, geometry_type));
    return false;
}

/**
 * Create render pipeline for geometry type
 */
static WGPURenderPipeline create_geometry_pipeline(WebGPURenderer *renderer) {
    /* Load embedded shaders */
    extern const char *basic_vertex_shader_source;
    extern const char *basic_fragment_shader_source;
    
    /* Create shader modules */
    WGPUShaderModule vertex_shader = webgpu_create_shader_module(
        renderer->device, basic_vertex_shader_source);
    WGPUShaderModule fragment_shader = webgpu_create_shader_module(
        renderer->device, basic_fragment_shader_source);
    
    if (!vertex_shader || !fragment_shader) {
        ecs_err("WebGPU: Failed to create shader modules");
        return NULL;
    }
    
    /* Create pipeline */
    WGPURenderPipeline pipeline = webgpu_create_geometry_pipeline(
        renderer->device, vertex_shader, fragment_shader);
    
    /* Cleanup shader modules */
    wgpuShaderModuleRelease(vertex_shader);
    wgpuShaderModuleRelease(fragment_shader);
    
    return pipeline;
}

/**
 * Gather renderable entities into batches by geometry type
 */
void webgpu_gather_geometry_batches(ecs_world_t *world, 
                                   WebGPURenderer *renderer, 
                                   ecs_query_t *query) {
    if (!world || !renderer || !query) {
        return;
    }
    
    /* Clear previous batches */
    ecs_vec_clear(&renderer->render_batches);
    
    /* Iterate through geometry components */
    ecs_id_t geometry_types[] = { ecs_id(EcsBox), ecs_id(EcsRectangle) };
    size_t num_geometry_types = sizeof(geometry_types) / sizeof(geometry_types[0]);
    
    for (size_t i = 0; i < num_geometry_types; i++) {
        ecs_id_t geometry_type = geometry_types[i];
        
        /* Create dynamic query for this geometry type */
        ecs_query_t *geometry_query = ecs_query(world, {
            .terms = {
                { .id = ecs_id(EcsTransform3), .inout = EcsIn },
                { .id = ecs_id(EcsRgb), .inout = EcsIn, .oper = EcsOptional },
                { .id = geometry_type, .inout = EcsIn }
            }
        });
        
        if (!geometry_query) {
            continue;
        }
        
        /* Count entities for this geometry type */
        uint32_t entity_count = 0;
        ecs_iter_t count_it = ecs_query_iter(world, geometry_query);
        while (ecs_query_next(&count_it)) {
            entity_count += count_it.count;
        }
        
        if (entity_count == 0) {
            ecs_query_fini(geometry_query);
            continue;
        }
        
        /* Allocate batch data */
        EcsTransform3 *transforms = ecs_os_malloc(entity_count * sizeof(EcsTransform3));
        EcsRgb *colors = ecs_os_malloc(entity_count * sizeof(EcsRgb));
        
        /* Gather entity data */
        uint32_t entity_index = 0;
        ecs_iter_t gather_it = ecs_query_iter(world, geometry_query);
        while (ecs_query_next(&gather_it)) {
            EcsTransform3 *batch_transforms = ecs_field(&gather_it, EcsTransform3, 0);
            EcsRgb *batch_colors = ecs_field(&gather_it, EcsRgb, 1);
            
            for (int j = 0; j < gather_it.count; j++) {
                /* Copy transform */
                transforms[entity_index] = batch_transforms[j];
                
                /* Copy color or use default */
                if (batch_colors) {
                    colors[entity_index] = batch_colors[j];
                } else {
                    colors[entity_index] = (EcsRgb){ .r = 1.0f, .g = 1.0f, .b = 1.0f };
                }
                
                entity_index++;
            }
        }
        
        /* Create render batch */
        webgpu_render_batch_t *batch = ecs_vec_append_t(
            renderer->allocator, &renderer->render_batches, webgpu_render_batch_t);
        
        batch->geometry_type = geometry_type;
        batch->instance_count = entity_count;
        batch->transforms = transforms;
        batch->colors = colors;
        
        /* Get geometry buffers */
        if (!get_geometry_buffers(world, renderer, geometry_type,
                                 &batch->vertex_buffer, &batch->index_buffer,
                                 &batch->vertex_count, &batch->index_count)) {
            ecs_warn("WebGPU: Failed to get geometry buffers for type: %s", 
                    ecs_get_name(world, geometry_type));
            continue;
        }
        
        /* Create instance buffer */
        batch->instance_buffer = create_instance_buffer(
            renderer->device, transforms, colors, entity_count);
        
        /* Create pipeline if needed */
        if (!renderer->default_pipeline) {
            renderer->default_pipeline = create_geometry_pipeline(renderer);
        }
        batch->pipeline = renderer->default_pipeline;
        
        ecs_query_fini(geometry_query);
        
        ecs_trace("WebGPU: Created batch for %s with %d instances",
                 ecs_get_name(world, geometry_type), entity_count);
    }
}

/**
 * Execute all gathered render batches
 */
void webgpu_execute_render_batches(WebGPURenderer *renderer, 
                                  WGPURenderPassEncoder render_pass) {
    if (!renderer || !render_pass) {
        return;
    }
    
    int32_t batch_count = ecs_vec_count(&renderer->render_batches);
    webgpu_render_batch_t *batches = ecs_vec_first(&renderer->render_batches);
    
    for (int32_t i = 0; i < batch_count; i++) {
        webgpu_render_batch_t *batch = &batches[i];
        
        if (!batch->pipeline || !batch->vertex_buffer || 
            !batch->index_buffer || !batch->instance_buffer) {
            ecs_warn("WebGPU: Skipping invalid batch for geometry type: %llu",
                    batch->geometry_type);
            continue;
        }
        
        /* Set pipeline */
        wgpuRenderPassEncoderSetPipeline(render_pass, batch->pipeline);
        
        /* Bind vertex buffers */
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, 
            batch->vertex_buffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1,
            batch->instance_buffer, 0, WGPU_WHOLE_SIZE);
            
        /* Bind index buffer */
        wgpuRenderPassEncoderSetIndexBuffer(render_pass, batch->index_buffer,
            WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
        
        /* Draw indexed with instancing */
        wgpuRenderPassEncoderDrawIndexed(render_pass,
            batch->index_count, batch->instance_count, 0, 0, 0);
        
        ecs_trace("WebGPU: Rendered batch with %d instances, %d indices",
                 batch->instance_count, batch->index_count);
    }
    
    /* Cleanup batch data */
    for (int32_t i = 0; i < batch_count; i++) {
        webgpu_render_batch_t *batch = &batches[i];
        
        ecs_os_free(batch->transforms);
        ecs_os_free(batch->colors);
        
        /* GPU resources will be cleaned up by renderer destructor */
    }
    
    ecs_vec_clear(&renderer->render_batches);
}
/**
 * @file resources/resource_manager.c
 * @brief WebGPU resource management implementation.
 */

#include <string.h>

/**
 * Create a WebGPU buffer with optional initial data
 */
WGPUBuffer webgpu_create_buffer(WGPUDevice device, size_t size, WGPUBufferUsage usage, const void *data) {
    if (!device || size == 0) {
        ecs_err("webgpu_create_buffer: Invalid parameters");
        return (WGPUBuffer){0};
    }
    
    WGPUBufferDescriptor buffer_desc = {
        .label = "WebGPU Buffer",
        .usage = usage,
        .size = size,
        .mappedAtCreation = data != NULL,
    };
    
    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &buffer_desc);
    if (!buffer) {
        ecs_err("webgpu_create_buffer: Failed to create buffer");
        return NULL;
    }
    
    /* Upload initial data if provided */
    if (data) {
        void *mapped_data = wgpuBufferGetMappedRange(buffer, 0, size);
        if (mapped_data) {
            memcpy(mapped_data, data, size);
            wgpuBufferUnmap(buffer);
        } else {
            ecs_err("webgpu_create_buffer: Failed to map buffer for initial data");
            wgpuBufferRelease(buffer);
            return NULL;
        }
    }
    
    return buffer;
}

/**
 * Update buffer contents with new data
 */
void webgpu_update_buffer(WGPUDevice device, WGPUQueue queue, WGPUBuffer buffer, const void *data, size_t size, size_t offset) {
    if (!device || !queue || !buffer || !data || size == 0) {
        ecs_err("webgpu_update_buffer: Invalid parameters");
        return;
    }
    
    wgpuQueueWriteBuffer(queue, buffer, offset, data, size);
}

/**
 * Create a 2D texture with specified format
 */
WGPUTexture webgpu_create_texture_2d(WGPUDevice device, uint32_t width, uint32_t height, WGPUTextureFormat format) {
    if (!device || width == 0 || height == 0) {
        ecs_err("webgpu_create_texture_2d: Invalid parameters");
        return (WGPUTexture){0};
    }
    
    WGPUTextureDescriptor texture_desc = {
        .label = "WebGPU 2D Texture",
        .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
        .dimension = WGPUTextureDimension_2D,
        .size = {
            .width = width,
            .height = height,
            .depthOrArrayLayers = 1,
        },
        .format = format,
        .mipLevelCount = 1,
        .sampleCount = 1,
    };
    
    WGPUTexture texture = wgpuDeviceCreateTexture(device, &texture_desc);
    if (!texture) {
        ecs_err("webgpu_create_texture_2d: Failed to create texture");
        return NULL;
    }
    
    return texture;
}

/**
 * Create shader module from WGSL source
 */
WGPUShaderModule webgpu_create_shader_module(WGPUDevice device, const char *wgsl_source) {
    if (!device || !wgsl_source) {
        ecs_err("webgpu_create_shader_module: Invalid parameters");
        return NULL;
    }
    
    WGPUShaderSourceWGSL wgsl_source_desc = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_ShaderSourceWGSL,
        },
        .code = {
            .data = wgsl_source,
            .length = strlen(wgsl_source),
        },
    };
    
    WGPUShaderModuleDescriptor shader_desc = {
        .label = "WebGPU Shader Module",
        .nextInChain = (WGPUChainedStruct*)&wgsl_source_desc,
    };
    
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(device, &shader_desc);
    if (!shader) {
        ecs_err("webgpu_create_shader_module: Failed to create shader module");
        return NULL;
    }
    
    return shader;
}

/**
 * Create basic render pipeline for geometry rendering
 */
WGPURenderPipeline webgpu_create_geometry_pipeline(WGPUDevice device, WGPUShaderModule vertex_shader, WGPUShaderModule fragment_shader) {
    if (!device || !vertex_shader || !fragment_shader) {
        ecs_err("webgpu_create_geometry_pipeline: Invalid parameters");
        return NULL;
    }
    
    /* Create bind group layouts for uniforms */
    WGPUBindGroupLayoutEntry camera_layout_entries[] = {
        {
            .binding = 0,
            .visibility = WGPUShaderStage_Vertex,
            .buffer = {
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(mat4) * 3, /* view, projection, view_projection matrices */
            },
        }
    };
    
    WGPUBindGroupLayoutDescriptor camera_layout_desc = {
        .label = "Camera Bind Group Layout",
        .entryCount = 1,
        .entries = camera_layout_entries,
    };
    
    WGPUBindGroupLayout camera_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &camera_layout_desc);
    
    WGPUBindGroupLayoutEntry light_layout_entries[] = {
        {
            .binding = 0,
            .visibility = WGPUShaderStage_Fragment,
            .buffer = {
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(vec3) * 3 + sizeof(float), /* direction, color, ambient, intensity */
            },
        }
    };
    
    WGPUBindGroupLayoutDescriptor light_layout_desc = {
        .label = "Light Bind Group Layout",
        .entryCount = 1,
        .entries = light_layout_entries,
    };
    
    WGPUBindGroupLayout light_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &light_layout_desc);
    
    /* Create pipeline layout */
    WGPUBindGroupLayout bind_group_layouts[] = {
        camera_bind_group_layout,
        light_bind_group_layout
    };
    
    WGPUPipelineLayoutDescriptor layout_desc = {
        .label = "Geometry Pipeline Layout",
        .bindGroupLayoutCount = 2,
        .bindGroupLayouts = bind_group_layouts,
    };
    
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);
    
    /* Define vertex attributes */
    WGPUVertexAttribute vertex_attributes[] = {
        /* Position */
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = 0,
            .shaderLocation = 0,
        },
        /* Normal */
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = 3 * sizeof(float),
            .shaderLocation = 1,
        },
        /* UV */
        {
            .format = WGPUVertexFormat_Float32x2,
            .offset = 6 * sizeof(float),
            .shaderLocation = 2,
        }
    };
    
    WGPUVertexBufferLayout vertex_buffer_layout = {
        .arrayStride = 8 * sizeof(float), /* position(3) + normal(3) + uv(2) */
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 3,
        .attributes = vertex_attributes,
    };
    
    /* Define instance attributes */
    WGPUVertexAttribute instance_attributes[] = {
        /* Transform matrix row 0 */
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 0,
            .shaderLocation = 3,
        },
        /* Transform matrix row 1 */
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 4 * sizeof(float),
            .shaderLocation = 4,
        },
        /* Transform matrix row 2 */
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 8 * sizeof(float),
            .shaderLocation = 5,
        },
        /* Transform matrix row 3 */
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 12 * sizeof(float),
            .shaderLocation = 6,
        },
        /* Color */
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = 16 * sizeof(float),
            .shaderLocation = 7,
        }
    };
    
    WGPUVertexBufferLayout instance_buffer_layout = {
        .arrayStride = 19 * sizeof(float), /* mat4(16) + rgb(3) */
        .stepMode = WGPUVertexStepMode_Instance,
        .attributeCount = 5,
        .attributes = instance_attributes,
    };
    
    WGPUVertexBufferLayout vertex_layouts[] = {
        vertex_buffer_layout,
        instance_buffer_layout
    };
    
    /* Create render pipeline */
    WGPUColorTargetState color_target = {
        .format = WGPUTextureFormat_BGRA8Unorm, /* Typical swap chain format */
        .writeMask = WGPUColorWriteMask_All,
        .blend = &(WGPUBlendState){
            .color = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            },
            .alpha = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_One,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            },
        },
    };
    
    WGPUFragmentState fragment_state = {
        .module = fragment_shader,
        .entryPoint = "fs_main",
        .targetCount = 1,
        .targets = &color_target,
    };
    
    WGPURenderPipelineDescriptor pipeline_desc = {
        .label = "Geometry Render Pipeline",
        .layout = pipeline_layout,
        .vertex = {
            .module = vertex_shader,
            .entryPoint = "vs_main",
            .bufferCount = 2,
            .buffers = vertex_layouts,
        },
        .fragment = &fragment_state,
        .primitive = {
            .topology = WGPUPrimitiveTopology_TriangleList,
            .stripIndexFormat = WGPUIndexFormat_Undefined,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_Back,
        },
        .depthStencil = &(WGPUDepthStencilState){
            .format = WGPUTextureFormat_Depth24Plus,
            .depthWriteEnabled = true,
            .depthCompare = WGPUCompareFunction_Less,
            .stencilReadMask = 0,
            .stencilWriteMask = 0,
        },
        .multisample = {
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = false,
        },
    };
    
    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);
    
    /* Cleanup intermediate objects */
    wgpuBindGroupLayoutRelease(camera_bind_group_layout);
    wgpuBindGroupLayoutRelease(light_bind_group_layout);
    wgpuPipelineLayoutRelease(pipeline_layout);
    
    if (!pipeline) {
        ecs_err("webgpu_create_geometry_pipeline: Failed to create render pipeline");
        return NULL;
    }
    
    return pipeline;
}

/**
 * Create uniform buffer with camera matrices
 */
WGPUBuffer webgpu_create_camera_uniform_buffer(WGPUDevice device) {
    /* Camera uniform buffer: view_matrix, projection_matrix, view_projection_matrix */
    size_t buffer_size = sizeof(mat4) * 3;
    
    return webgpu_create_buffer(device, buffer_size, 
        WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, NULL);
}

/**
 * Create uniform buffer with light data
 */
WGPUBuffer webgpu_create_light_uniform_buffer(WGPUDevice device) {
    /* Light uniform buffer: direction(vec3), color(vec3), ambient(vec3), intensity(float) */
    /* Note: Need to account for WGSL alignment requirements */
    size_t buffer_size = sizeof(vec4) * 3 + sizeof(float) * 4; /* Padded for alignment */
    
    return webgpu_create_buffer(device, buffer_size,
        WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, NULL);
}

/**
 * Create bind group for camera uniforms
 */
WGPUBindGroup webgpu_create_camera_bind_group(WGPUDevice device, WGPUBindGroupLayout layout, WGPUBuffer uniform_buffer) {
    WGPUBindGroupEntry entries[] = {
        {
            .binding = 0,
            .buffer = uniform_buffer,
            .offset = 0,
            .size = sizeof(mat4) * 3,
        }
    };
    
    WGPUBindGroupDescriptor bind_group_desc = {
        .label = "Camera Bind Group",
        .layout = layout,
        .entryCount = 1,
        .entries = entries,
    };
    
    return wgpuDeviceCreateBindGroup(device, &bind_group_desc);
}

/**
 * Create bind group for light uniforms
 */
WGPUBindGroup webgpu_create_light_bind_group(WGPUDevice device, WGPUBindGroupLayout layout, WGPUBuffer uniform_buffer) {
    WGPUBindGroupEntry entries[] = {
        {
            .binding = 0,
            .buffer = uniform_buffer,
            .offset = 0,
            .size = sizeof(vec4) * 3 + sizeof(float) * 4,
        }
    };
    
    WGPUBindGroupDescriptor bind_group_desc = {
        .label = "Light Bind Group",
        .layout = layout,
        .entryCount = 1,
        .entries = entries,
    };
    
    return wgpuDeviceCreateBindGroup(device, &bind_group_desc);
}
/**
 * @file shaders/shader_sources.c
 * @brief Embedded WGSL shader sources.
 */


const char *basic_vertex_shader_source = R"(
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
}

struct InstanceInput {
    @location(3) model_matrix_0: vec4<f32>,
    @location(4) model_matrix_1: vec4<f32>,
    @location(5) model_matrix_2: vec4<f32>,
    @location(6) model_matrix_3: vec4<f32>,
    @location(7) color: vec3<f32>,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) color: vec3<f32>,
}

struct Camera {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    view_projection: mat4x4<f32>,
}

@group(0) @binding(0)
var<uniform> camera: Camera;

@vertex
fn vs_main(vertex: VertexInput, instance: InstanceInput) -> VertexOutput {
    let model_matrix = mat4x4<f32>(
        instance.model_matrix_0,
        instance.model_matrix_1,
        instance.model_matrix_2,
        instance.model_matrix_3,
    );
    
    let world_position = model_matrix * vec4<f32>(vertex.position, 1.0);
    let world_normal = normalize((model_matrix * vec4<f32>(vertex.normal, 0.0)).xyz);
    
    var out: VertexOutput;
    out.clip_position = camera.view_projection * world_position;
    out.world_position = world_position.xyz;
    out.world_normal = world_normal;
    out.uv = vertex.uv;
    out.color = instance.color;
    
    return out;
}
)";

const char *basic_fragment_shader_source = R"(
struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) color: vec3<f32>,
}

struct Light {
    direction: vec3<f32>,
    color: vec3<f32>,
    ambient: vec3<f32>,
    intensity: f32,
}

@group(1) @binding(0)
var<uniform> light: Light;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let normal = normalize(in.world_normal);
    let light_dir = normalize(-light.direction);
    
    // Lambertian diffuse lighting
    let ndotl = max(dot(normal, light_dir), 0.0);
    let diffuse = light.color * light.intensity * ndotl;
    
    // Combine lighting with material color
    let final_color = in.color * (light.ambient + diffuse);
    
    return vec4<f32>(final_color, 1.0);
}
)";
/**
 * @file webgpu/dawn_impl.c
 * @brief Minimal Dawn WebGPU implementation for native builds.
 */


#ifdef WEBGPU_BACKEND_DAWN

/* For now, we'll create a minimal stub implementation that allows compilation
   but prints warnings about missing functionality. This allows us to test
   the build system and focus on the web implementation first. */

#include <stdio.h>

/* Minimal Dawn stub implementations */
WGPUInstance wgpuCreateInstance(const WGPUInstanceDescriptor* descriptor) {
    (void)descriptor;
    printf("Dawn: wgpuCreateInstance called (stub)\n");
    return (WGPUInstance){.id = 1};
}

void wgpuInstanceRequestAdapter(WGPUInstance instance,
                               const WGPURequestAdapterOptions* options,
                               WGPURequestAdapterCallback callback,
                               void* userdata) {
    (void)instance;
    (void)options;
    printf("Dawn: wgpuInstanceRequestAdapter called (stub)\n");
    if (callback) {
        callback(WGPURequestAdapterStatus_Success, (WGPUAdapter){.id = 1}, NULL, userdata);
    }
}

WGPUDevice wgpuAdapterRequestDevice(WGPUAdapter adapter,
                                   const WGPUDeviceDescriptor* descriptor,
                                   WGPURequestDeviceCallback callback,
                                   void* userdata) {
    (void)adapter;
    (void)descriptor;
    printf("Dawn: wgpuAdapterRequestDevice called (stub)\n");
    if (callback) {
        callback(WGPURequestDeviceStatus_Success, (WGPUDevice){.id = 1}, NULL, userdata);
    }
    return (WGPUDevice){.id = 1};
}

WGPUQueue wgpuDeviceGetQueue(WGPUDevice device) {
    (void)device;
    return (WGPUQueue){.id = 1};
}

WGPUBuffer wgpuDeviceCreateBuffer(WGPUDevice device, const WGPUBufferDescriptor* descriptor) {
    (void)device;
    (void)descriptor;
    printf("Dawn: wgpuDeviceCreateBuffer called (stub)\n");
    return (WGPUBuffer){.id = 1};
}

void wgpuDeviceSetUncapturedErrorCallback(WGPUDevice device,
                                         WGPUErrorCallback callback,
                                         void* userdata) {
    (void)device;
    (void)callback;
    (void)userdata;
    printf("Dawn: wgpuDeviceSetUncapturedErrorCallback called (stub)\n");
}

/* Add more stub implementations as needed... */
void wgpuInstanceRelease(WGPUInstance instance) { (void)instance; }
void wgpuAdapterRelease(WGPUAdapter adapter) { (void)adapter; }
void wgpuDeviceRelease(WGPUDevice device) { (void)device; }
void wgpuQueueRelease(WGPUQueue queue) { (void)queue; }
void wgpuBufferRelease(WGPUBuffer buffer) { (void)buffer; }

#endif /* WEBGPU_BACKEND_DAWN */
