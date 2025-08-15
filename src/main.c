/**
 * @file main.c
 * @brief WebGPU Systems module implementation for Flecs.
 */

#include "private_api.h"

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
    
    /* Get current swap chain texture */
    WGPUTextureView back_buffer = wgpuSwapChainGetCurrentTextureView(renderer->swap_chain);
    if (!back_buffer) {
        ecs_warn("WebGPU: Failed to get current swap chain texture");
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
    wgpuSwapChainPresent(renderer->swap_chain);
    
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
    
    if (ptr->swap_chain) {
        wgpuSwapChainRelease(ptr->swap_chain);
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