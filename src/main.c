/**
 * @file main.c
 * @brief Flecs WebGPU Engine Demo Application with integrated system implementation
 */

#include "flecs_systems_webgpu.h"
#include "private_api.h"

/* Math types and operations - include cglm directly */
#include <math.h>
#include <cglm/cglm.h>

/* No duplicate definitions needed - using private_api.h */

/* Component declarations and definitions */
ECS_COMPONENT_DECLARE(WebGPURenderer);
ECS_COMPONENT_DECLARE(WebGPUGeometry);
ECS_COMPONENT_DECLARE(WebGPUMaterial);
ECS_COMPONENT_DECLARE(WebGPUQuery);

/* Geometry type entities */
ECS_DECLARE(WebGPUBoxGeometry);
ECS_DECLARE(WebGPURectangleGeometry);

/* Static renderer instance (singleton pattern) */
static ecs_entity_t webgpu_renderer_instance = 0;

/* Global error flag to stop rendering on validation errors */
static bool webgpu_error_occurred = false;

/* Local forward declarations */
static void webgpu_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata);
static void webgpu_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata);

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
    
    /* Set global error flag to stop rendering */
    webgpu_error_occurred = true;
    
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    /* Log to browser console for debugging */
    EM_ASM({
        console.error('WebGPU Error:', UTF8ToString($0), UTF8ToString($1));
        console.error('Stopping render loop due to WebGPU error');
    }, type_str, message);
    
    /* Cancel the Emscripten main loop to prevent further errors */
    emscripten_cancel_main_loop();
#endif
}

/**
 * Adapter request callback
 */
static void webgpu_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('WebGPU: Adapter callback received, status:', $0);
    }, status);
#endif
    
    if (status != WGPURequestAdapterStatus_Success) {
        ecs_err("WebGPU: Failed to request adapter: %s", message ? message : "Unknown error");
#ifdef __EMSCRIPTEN__
        EM_ASM({
            console.error('WebGPU: Adapter request failed');
        });
#endif
        return;
    }
    
    ecs_trace("WebGPU: Adapter acquired successfully");
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('WebGPU: Adapter acquired successfully, now requesting device...');
    });
#endif
    
    /* Store adapter in renderer component */
    WebGPURenderer* renderer = (WebGPURenderer*)userdata;
    renderer->adapter = adapter;
    
    /* Immediately request device from adapter callback */
    WGPUDeviceDescriptor device_desc = {
        .nextInChain = NULL,
        .label = "WebGPU Device",
        .requiredFeatureCount = 0,
        .requiredFeatures = NULL,
        .requiredLimits = NULL,
    };
    
    wgpuAdapterRequestDevice(adapter, &device_desc, webgpu_device_callback, userdata);
}

/**
 * Device request callback
 */
static void webgpu_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('WebGPU: Device callback received, status:', $0);
    }, status);
#endif
    
    if (status != WGPURequestDeviceStatus_Success) {
        ecs_err("WebGPU: Failed to request device: %s", message ? message : "Unknown error");
#ifdef __EMSCRIPTEN__
        EM_ASM({
            console.error('WebGPU: Device request failed');
        });
#endif
        return;
    }
    
    ecs_trace("WebGPU: Device acquired successfully");
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('WebGPU: Device and queue acquired successfully - WebGPU pipeline ready!');
    });
#endif
    
    /* Store device in renderer component and set up error callback */
    WebGPURenderer* renderer = (WebGPURenderer*)userdata;
    renderer->device = device;
    renderer->queue = wgpuDeviceGetQueue(device);
    
    wgpuDeviceSetUncapturedErrorCallback(device, webgpu_device_error_callback, NULL);
    
    /* Configure canvas context now that we have a device */
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    /* In Emscripten, we need to configure the surface format */
    WGPUSurfaceConfiguration surface_config = {
        .nextInChain = NULL,
        .device = device,
        .format = WGPUTextureFormat_BGRA8Unorm, /* Standard web format */
        .usage = WGPUTextureUsage_RenderAttachment,
        .width = renderer->width,
        .height = renderer->height,
        .presentMode = WGPUPresentMode_Fifo,
        .alphaMode = WGPUCompositeAlphaMode_Auto,
    };
    
    wgpuSurfaceConfigure(renderer->surface, &surface_config);
    
    /* Create depth texture and view with extensive logging */
    EM_ASM({
        console.log('WebGPU: Starting depth texture creation...');
        console.log('WebGPU: Canvas dimensions:', $0, 'x', $1);
    }, renderer->width, renderer->height);
    
    renderer->depth_texture = webgpu_create_depth_texture(device, renderer->width, renderer->height);
    
    if (renderer->depth_texture) {
        EM_ASM({
            console.log('WebGPU: ✓ Depth texture created successfully');
            console.log('WebGPU: Depth texture handle:', $0);
        }, (uint32_t)(uintptr_t)renderer->depth_texture);
        
        renderer->depth_texture_view = webgpu_create_depth_texture_view(renderer->depth_texture);
        
        if (renderer->depth_texture_view) {
            EM_ASM({
                console.log('WebGPU: ✓ Depth texture view created successfully');
                console.log('WebGPU: Depth texture view handle:', $0);
            }, (uint32_t)(uintptr_t)renderer->depth_texture_view);
        } else {
            EM_ASM({
                console.error('WebGPU: ✗ Failed to create depth texture view');
            });
        }
    } else {
        EM_ASM({
            console.error('WebGPU: ✗ Failed to create depth texture');
            console.error('WebGPU: Device handle:', $0);
            console.error('WebGPU: Canvas dimensions:', $1, 'x', $2);
        }, (uint32_t)(uintptr_t)device, renderer->width, renderer->height);
    }
    
    /* Create uniform buffers for camera and lighting */
    EM_ASM({
        console.log('WebGPU: Creating uniform buffers...');
    });
    
    renderer->camera_uniform_buffer = webgpu_create_camera_uniform_buffer(device);
    renderer->light_uniform_buffer = webgpu_create_light_uniform_buffer(device);
    
    if (renderer->camera_uniform_buffer && renderer->light_uniform_buffer) {
        EM_ASM({
            console.log('WebGPU: ✓ Uniform buffers created successfully');
            console.log('WebGPU: Camera buffer:', $0);
            console.log('WebGPU: Light buffer:', $1);
        }, (uint32_t)(uintptr_t)renderer->camera_uniform_buffer, 
           (uint32_t)(uintptr_t)renderer->light_uniform_buffer);
        
        /* Create shaders and pipeline to get bind group layouts */
        const char *vertex_source = basic_vertex_shader_source;
        const char *fragment_source = basic_fragment_shader_source;
        
        WGPUShaderModule vertex_shader = webgpu_create_shader_module(device, vertex_source);
        WGPUShaderModule fragment_shader = webgpu_create_shader_module(device, fragment_source);
        
        if (vertex_shader && fragment_shader) {
            renderer->default_pipeline = webgpu_create_geometry_pipeline(device, vertex_shader, fragment_shader);
            
            if (renderer->default_pipeline) {
                EM_ASM({
                    console.log('WebGPU: ✓ Default render pipeline created');
                });
                
                /* Extract bind group layouts from pipeline - Note: This is a simplified approach */
                /* In a real implementation, we'd create layouts separately and reuse them */
                /* For now, we'll create bind group layouts inline */
                
                /* Create camera bind group layout */
                WGPUBindGroupLayoutEntry camera_layout_entries[] = {
                    {
                        .binding = 0,
                        .visibility = WGPUShaderStage_Vertex,
                        .buffer = {
                            .type = WGPUBufferBindingType_Uniform,
                            .hasDynamicOffset = false,
                            .minBindingSize = sizeof(mat4) * 3,
                        },
                    }
                };
                
                WGPUBindGroupLayoutDescriptor camera_layout_desc = {
                    .label = "Camera Bind Group Layout",
                    .entryCount = 1,
                    .entries = camera_layout_entries,
                };
                
                renderer->camera_layout = wgpuDeviceCreateBindGroupLayout(device, &camera_layout_desc);
                
                /* Create light bind group layout */
                WGPUBindGroupLayoutEntry light_layout_entries[] = {
                    {
                        .binding = 0,
                        .visibility = WGPUShaderStage_Fragment,
                        .buffer = {
                            .type = WGPUBufferBindingType_Uniform,
                            .hasDynamicOffset = false,
                            .minBindingSize = 40, /* Matches our 40-byte Light struct */
                        },
                    }
                };
                
                WGPUBindGroupLayoutDescriptor light_layout_desc = {
                    .label = "Light Bind Group Layout",
                    .entryCount = 1,
                    .entries = light_layout_entries,
                };
                
                renderer->light_layout = wgpuDeviceCreateBindGroupLayout(device, &light_layout_desc);
                
                /* Create bind groups */
                renderer->camera_bind_group = webgpu_create_camera_bind_group(device, renderer->camera_layout, renderer->camera_uniform_buffer);
                renderer->light_bind_group = webgpu_create_light_bind_group(device, renderer->light_layout, renderer->light_uniform_buffer);
                
                if (renderer->camera_bind_group && renderer->light_bind_group) {
                    EM_ASM({
                        console.log('WebGPU: ✓ Uniform buffer bind groups created successfully');
                        console.log('WebGPU: Camera bind group:', $0);
                        console.log('WebGPU: Light bind group:', $1);
                    }, (uint32_t)(uintptr_t)renderer->camera_bind_group,
                       (uint32_t)(uintptr_t)renderer->light_bind_group);
                } else {
                    EM_ASM({
                        console.error('WebGPU: ✗ Failed to create uniform buffer bind groups');
                    });
                }
            } else {
                EM_ASM({
                    console.error('WebGPU: ✗ Failed to create render pipeline');
                });
            }
            
            /* Clean up shader modules */
            wgpuShaderModuleRelease(vertex_shader);
            wgpuShaderModuleRelease(fragment_shader);
        } else {
            EM_ASM({
                console.error('WebGPU: ✗ Failed to create shader modules');
            });
        }
    } else {
        EM_ASM({
            console.error('WebGPU: ✗ Failed to create uniform buffers');
        });
    }
    
    EM_ASM({
        console.log('WebGPU: Surface configured for rendering');
    });
#endif
}

/**
 * Initialize WebGPU renderer component
 */
static void webgpu_init_renderer(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    WebGPURenderer *renderer = ecs_field(it, WebGPURenderer, 0);
    EcsCanvas *canvas = ecs_field(it, EcsCanvas, 1);
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('WebGPU: Initializing renderer');
    });
#endif
    
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
        EM_ASM({
            console.log('WebGPU: Instance creation attempted');
        });
#else
        renderer[i].instance = wgpuCreateInstance(&instance_desc);
#endif
        
        if (!renderer[i].instance) {
#ifdef __EMSCRIPTEN__
            EM_ASM({
                console.log('WebGPU: Failed to create instance');
            });
#endif
            continue;
        }
        
#ifdef __EMSCRIPTEN__
        EM_ASM({
            console.log('WebGPU: Instance created successfully');
        });
#endif
        
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
        
        /* Request WebGPU adapter asynchronously */
        EM_ASM({
            console.log('WebGPU: Requesting adapter asynchronously...');
        });
        
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
            .terms = { { .id = ecs_id(EcsTransform3) } }
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
    
    /* Exit early if WebGPU error occurred to prevent infinite error loop */
    if (webgpu_error_occurred) {
        ecs_err("WebGPU: Stopping render loop due to previous error");
        return;
    }
    
    /* Check for canvas resize */
    const EcsCanvas *canvas = ecs_get(world, renderer->canvas_entity, EcsCanvas);
    if (canvas && (renderer->width != (uint32_t)canvas->width || renderer->height != (uint32_t)canvas->height)) {
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
        .depthSlice = -1, /* Required for 2D textures - Emscripten treats -1 as undefined */
    };
    
    WGPURenderPassDepthStencilAttachment depth_attachment = {
        .view = renderer->depth_texture_view,
        .depthClearValue = 1.0f,
        .depthLoadOp = WGPULoadOp_Clear,
        .depthStoreOp = WGPUStoreOp_Store,
        /* No stencil operations for depth-only texture (Depth24Plus format) */
        .stencilClearValue = 0,
        .stencilLoadOp = WGPULoadOp_Undefined,
        .stencilStoreOp = WGPUStoreOp_Undefined,
    };
    
    /* Log depth attachment status before render pass creation */
    bool has_depth_attachment = renderer->depth_texture_view != NULL;
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('WebGPU: Creating render pass...');
        console.log('WebGPU: Depth texture view available:', $0 ? 'YES' : 'NO');
        if ($0) {
            console.log('WebGPU: Depth texture view handle:', $1);
            console.log('WebGPU: ✓ Render pass will include depth stencil attachment');
            console.log('WebGPU: Depth operations: Clear(1.0) -> Store');
            console.log('WebGPU: Stencil operations: DISABLED (depth-only texture)');
        } else {
            console.warn('WebGPU: ⚠ Render pass will NOT include depth stencil attachment');
        }
    }, has_depth_attachment, (uint32_t)(uintptr_t)renderer->depth_texture_view);
#endif
    
    WGPURenderPassDescriptor render_pass_desc = {
        .label = "WebGPU Main Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &color_attachment,
        .depthStencilAttachment = has_depth_attachment ? &depth_attachment : NULL,
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
    
    /* Present frame - Skip for Emscripten as it's handled automatically */
#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(renderer->surface);
#else
    /* Emscripten handles presentation automatically with requestAnimationFrame */
#endif
    
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
    
    if (ptr->depth_texture_view) {
        wgpuTextureViewRelease(ptr->depth_texture_view);
    }
    
    if (ptr->depth_texture) {
        wgpuTextureRelease(ptr->depth_texture);
    }
    
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
 * WebGPUGeometry component lifecycle functions
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
    
    if (ptr->vertex_buffer != NULL) {
        wgpuBufferRelease(ptr->vertex_buffer);
    }
    
    if (ptr->index_buffer != NULL) {
        wgpuBufferRelease(ptr->index_buffer);
    }
    
    if (ptr->instance_buffer != NULL) {
        wgpuBufferRelease(ptr->instance_buffer);
    }
    
    if (ptr->pipeline != NULL) {
        wgpuRenderPipelineRelease(ptr->pipeline);
    }
    
    if (ptr->bind_group != NULL) {
        wgpuBindGroupRelease(ptr->bind_group);
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
    
    /* Skip logging to avoid WASM issues */
    
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
        [out] !WebGPURenderer, 
        [in] flecs.components.gui.Canvas);
        
    ecs_system(world, {
        .entity = webgpu_init_renderer,
        .immediate = true  /* Need direct world access for query creation */
    });
    
    /* Main rendering system */
    ECS_SYSTEM(world, webgpu_render_system, EcsOnStore,
        [in] WebGPURenderer, 
        [in] WebGPUQuery);
    
    /* Create singleton query component */
    ecs_singleton_set(world, WebGPUQuery, {
        .query = ecs_query(world, { 
            .terms = { { .id = ecs_id(EcsTransform3) } }
        })
    });
    
    /* Define geometry component with lifecycle hooks */
    ECS_COMPONENT_DEFINE(world, WebGPUGeometry);
    ecs_set_hooks(world, WebGPUGeometry, {
        .ctor = ecs_ctor(WebGPUGeometry),
        .dtor = ecs_dtor(WebGPUGeometry)
    });
    
    /* Create geometry type entities */
    ECS_ENTITY_DEFINE(world, WebGPUBoxGeometry, WebGPUGeometry);
    ECS_ENTITY_DEFINE(world, WebGPURectangleGeometry, WebGPUGeometry);
    
    /* Import material subsystem */
    webgpu_material_import(world);
    
    /* Module import completed */
}

/* Forward declarations for helper functions */
static WGPUBuffer create_instance_buffer(WGPUDevice device, 
                                       const EcsTransform3 *transforms,
                                       const EcsRgb *colors,
                                       uint32_t count);
static bool get_geometry_buffers(ecs_world_t *world,
                                WebGPURenderer *renderer,
                                ecs_id_t geometry_type,
                                WGPUBuffer *vertex_buffer,
                                WGPUBuffer *index_buffer,
                                uint32_t *vertex_count,
                                uint32_t *index_count);
static WGPURenderPipeline create_geometry_pipeline(WebGPURenderer *renderer);

/* Duplicate functions removed - implementations now in render_system.c */


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
    
    size_t buffer_size = count * 19 * sizeof(float); /* 16 + 3 floats per instance */
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
            /* Copy the existing transform matrix directly */
            memcpy(dst, transforms[i].value, 16 * sizeof(float));
        } else {
            /* Identity matrix */
            mat4 identity;
            glm_mat4_identity(identity);
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
    
    /* Create GPU buffer using resource manager function */
    extern WGPUBuffer webgpu_create_buffer(WGPUDevice device, size_t size, WGPUBufferUsage usage, const void *data);
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
    
    extern WGPUBuffer webgpu_create_buffer(WGPUDevice device, size_t size, WGPUBufferUsage usage, const void *data);
    
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
    extern WGPUShaderModule webgpu_create_shader_module(WGPUDevice device, const char *wgsl_source);
    extern WGPURenderPipeline webgpu_create_geometry_pipeline(WGPUDevice device, WGPUShaderModule vertex_shader, WGPUShaderModule fragment_shader);
    
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

/* ============================================================================
 * DEMO APPLICATION CODE
 * ============================================================================ */

/* Global state for demo */
static ecs_world_t *g_world = NULL;
static ecs_entity_t g_renderer = 0;

/* Forward declarations */
static void demo_create_scene(void);
static void main_loop(void);

/**
 * Main loop function for Emscripten
 */
static void main_loop(void) {
    static bool first_run = true;
    static int frame_count = 0;
    
    if (first_run) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            console.log('Main loop: First run starting');
        });
#endif
        first_run = false;
    }
    
    frame_count++;
    
    if (!g_world) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            console.log('Main loop: No world, canceling loop');
        });
#endif
        emscripten_cancel_main_loop();
        return;
    }
    
    /* Wrap ECS progress in try-catch style error handling */
    bool progress_success = false;
    
#ifdef __EMSCRIPTEN__
    /* Use Emscripten's exception handling */
    EM_ASM({
        try {
            Module.ecs_progress_safe = true;
        } catch(e) {
            Module.ecs_progress_safe = false;
            console.error('Pre-progress error:', e);
        }
    });
#endif
    
    /* Run the ECS frame with error checking */
    bool should_continue = false;
    
    /* Simple error boundary */
    if (g_world) {
        should_continue = ecs_progress(g_world, 0);
        progress_success = true;
        
        if (frame_count % 60 == 0) {
#ifdef __EMSCRIPTEN__
            EM_ASM({
                console.log('Frame:', $0, 'ECS running normally');
            }, frame_count);
#endif
        }
    }
    
    if (!should_continue || !progress_success) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            console.log('ECS progress stopping: continue=', $0, 'success=', $1);
        }, should_continue, progress_success);
#endif
        emscripten_cancel_main_loop();
    }
}

/**
 * Initialize the demo world
 */
static void demo_init() {
    /* Create Flecs world - disable logging to avoid WASM issues */
    g_world = ecs_init();
    if (!g_world) {
        return;
    }
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('Flecs world created successfully');
    });
#endif
    
    /* Import WebGPU systems */
    FlecsSystemsWebGPUImport(g_world);
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('WebGPU systems imported successfully');
    });
#endif

#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('About to create renderer entity');
    });
#endif
    
    /* Create renderer entity with canvas component */
    g_renderer = ecs_new(g_world);
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('Renderer entity created, adding EcsCanvas');
    });
#endif
    
    ecs_set(g_world, g_renderer, EcsCanvas, {
        .width = 1024,
        .height = 600
    });
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('EcsCanvas added, adding WebGPURenderer');
    });
#endif
    
    ecs_add(g_world, g_renderer, WebGPURenderer);
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('WebGPURenderer added, adding WebGPUQuery');
    });
#endif
    
    ecs_add(g_world, g_renderer, WebGPUQuery);
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('Renderer entity created');
    });
#endif
    
    /* Manually trigger renderer initialization for web */
    const EcsCanvas *canvas = ecs_get(g_world, g_renderer, EcsCanvas);
    WebGPURenderer *renderer = ecs_get_mut(g_world, g_renderer, WebGPURenderer);
    if (canvas && renderer) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            console.log('Attempting manual WebGPU initialization');
        });
#endif
        
        /* Simple WebGPU initialization */
        renderer->width = canvas->width;
        renderer->height = canvas->height;
        renderer->canvas_entity = g_renderer;
        
        /* Try to create WebGPU instance */
        renderer->instance = wgpuCreateInstance(NULL);
        if (renderer->instance) {
#ifdef __EMSCRIPTEN__
            EM_ASM({
                console.log('WebGPU instance created successfully');
            });
#endif
            
            /* Create surface for web canvas */
            WGPUSurfaceDescriptorFromCanvasHTMLSelector canvas_desc = {
                .chain = {
                    .next = NULL,
                    .sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector,
                },
                .selector = "#canvas", /* HTML canvas element */
            };
            
            WGPUSurfaceDescriptor surface_desc = {
                .nextInChain = (WGPUChainedStruct*)&canvas_desc,
                .label = "WebGPU Canvas Surface",
            };
            
            renderer->surface = wgpuInstanceCreateSurface(renderer->instance, &surface_desc);
            if (renderer->surface) {
#ifdef __EMSCRIPTEN__
                EM_ASM({
                    console.log('WebGPU surface created successfully');
                });
#endif
            } else {
#ifdef __EMSCRIPTEN__
                EM_ASM({
                    console.log('Failed to create WebGPU surface');
                });
#endif
            }
        } else {
#ifdef __EMSCRIPTEN__
            EM_ASM({
                console.log('Failed to create WebGPU instance');
            });
#endif
        }
        
        /* Now request WebGPU adapter */
        if (renderer->instance && renderer->surface) {
            EM_ASM({
                console.log('WebGPU: Requesting adapter from manual init...');
            });
            
            WGPURequestAdapterOptions adapter_options = {
                .nextInChain = NULL,
                .compatibleSurface = renderer->surface,
                .powerPreference = WGPUPowerPreference_HighPerformance,
                .forceFallbackAdapter = false,
            };
            
            wgpuInstanceRequestAdapter(renderer->instance, &adapter_options, 
                                      webgpu_adapter_callback, renderer);
        }
        
        ecs_modified(g_world, g_renderer, WebGPURenderer);
    }
    
    /* Create some demo entities */
    demo_create_scene();
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('Demo scene created with 5 entities');
    });
#endif
}

/**
 * Create demo scene with some basic entities
 */
static void demo_create_scene() {
    if (!g_world) return;
    
    /* Create a few demo boxes */
    for (int i = 0; i < 5; i++) {
        ecs_entity_t entity = ecs_new(g_world);
        
        /* Add transform component */
        ecs_set(g_world, entity, EcsTransform3, {
            .value = GLM_MAT4_IDENTITY_INIT
        });
        
        /* Set position in transform matrix */
        EcsTransform3 *transform = ecs_get_mut(g_world, entity, EcsTransform3);
        if (transform) {
            glm_translate(transform->value, (vec3){i * 2.0f - 4.0f, 0.0f, -5.0f});
            ecs_modified(g_world, entity, EcsTransform3);
        }
        
        /* Add color component */
        ecs_set(g_world, entity, EcsRgb, {
            .r = (float)i / 5.0f, 
            .g = 0.5f, 
            .b = 1.0f - (float)i / 5.0f
        });
        
        /* Add box geometry component */
        ecs_add(g_world, entity, EcsBox);
    }
}

/* Deferred main loop setup for safer Emscripten integration */
static void setup_main_loop_deferred() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('Deferred: Setting up main loop');
    });
    
    if (!g_world) {
        EM_ASM({
            console.error('Deferred: No world available for main loop');
        });
        return;
    }
    
    /* Set target FPS */
    ecs_set_target_fps(g_world, 60);
    
    /* Use setTimeout to defer the actual main loop setup */
    EM_ASM({
        console.log('Deferred: Starting main loop with timeout');
        setTimeout(function() {
            try {
                Module._setup_emscripten_main_loop();
            } catch(e) {
                console.error('Failed to setup main loop:', e);
            }
        }, 100); // Small delay to ensure everything is ready
    });
#endif
}

/* Exported function for JavaScript to call */
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
void setup_emscripten_main_loop() {
    EM_ASM({
        console.log('Setting up Emscripten main loop from JavaScript');
    });
    
    /* This is the actual main loop setup */
    emscripten_set_main_loop(main_loop, 0, 1);
    
    EM_ASM({
        console.log('Main loop setup complete');
    });
}
#endif

/**
 * Main application entry point - like tower_defense
 */
int main() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('Main function starting (WebAssembly)');
    });
#endif
    
    /* Initialize the demo */
    demo_init();
    
    if (!g_world) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            console.error('Failed to initialize engine: no world created');
        });
#endif
        return -1;
    }
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('Demo initialized successfully, setting up main loop');
    });
    
    /* For Emscripten, defer the main loop setup to avoid unwind issues */
    setup_main_loop_deferred();
    
    EM_ASM({
        console.log('Main function completed, main loop will start asynchronously');
    });
    
    return 0;
#else
    /* Native application loop */
    ecs_set_target_fps(g_world, 60);
    
    while (ecs_progress(g_world, 0)) {
        /* Continue until quit */
    }
    
    /* Cleanup */
    ecs_fini(g_world);
    return 0;
#endif
}