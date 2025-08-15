# WebGPU Integration Guide

This document details the WebGPU backend integration, covering implementation choices, cross-platform considerations, and web-specific optimizations.

## 🎯 Backend Selection

### Primary: Dawn WebGPU Implementation

**Rationale for Dawn**:
- **Mature Implementation**: Most advanced WebGPU implementation as of 2024
- **Official Emscripten Support**: Native integration via `-sUSE_WEBGPU=1`
- **Superior Debugging**: Better error messages and development experience
- **Browser Compatibility**: Chrome/Chromium's native WebGPU implementation
- **Cross-Platform**: Consistent behavior across web and native targets

### Secondary: wgpu-native Support

**Fallback Option**:
- **Rust Implementation**: Mozilla's WebGPU implementation for Firefox
- **Shared Headers**: Moving toward compatible `webgpu.h` interfaces
- **Development Tool**: Useful for comparison and validation
- **Future Proofing**: Alternative if Dawn development stalls

### Build Configuration Matrix

```cmake
# CMake configuration for different backends
if(EMSCRIPTEN)
    # Web target: Use Emscripten's WebGPU bindings
    target_link_libraries(App PRIVATE webgpu)
    target_compile_definitions(App PRIVATE WEBGPU_BACKEND_EMSCRIPTEN)
    set_target_properties(App PROPERTIES SUFFIX ".html")
elseif(WEBGPU_BACKEND_WGPU)
    # Native target: wgpu-native
    find_package(wgpu REQUIRED)
    target_link_libraries(App PRIVATE wgpu::wgpu)
    target_compile_definitions(App PRIVATE WEBGPU_BACKEND_WGPU)
else()
    # Native target: Dawn (default)
    find_package(Dawn REQUIRED)
    target_link_libraries(App PRIVATE dawn::dawn_native dawn::dawn_proc)
    target_compile_definitions(App PRIVATE WEBGPU_BACKEND_DAWN)
endif()
```

## 🌐 Emscripten Integration

### Compiler Flags

```json
{
    "lang.c": {
        "${target em}": {
            "ldflags": [
                "-sUSE_WEBGPU=1",           // Enable WebGPU support
                "-sASYNCIFY=1",             // Support async operations
                "-sALLOW_MEMORY_GROWTH=1",  // Dynamic memory allocation
                "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
            ],
            "cflags": [
                "-DWEBGPU_BACKEND_EMSCRIPTEN"
            ]
        }
    }
}
```

### WebGPU Context Creation

```c
// Platform-specific WebGPU instance creation
WGPUInstance webgpu_create_instance(void) {
    WGPUInstanceDescriptor desc = {
        .nextInChain = NULL,
    };
    
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    // Emscripten: Pass NULL for web context
    return wgpuCreateInstance(NULL);
#else
    // Native: Use standard descriptor
    return wgpuCreateInstance(&desc);
#endif
}
```

### Canvas Surface Creation

```c
// Web-specific surface creation from HTML canvas
WGPUSurface webgpu_create_surface_from_canvas(WGPUInstance instance, const char* canvas_selector) {
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    WGPUSurfaceDescriptorFromCanvasHTMLSelector canvas_desc = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector,
        },
        .selector = canvas_selector,
    };
    
    WGPUSurfaceDescriptor surface_desc = {
        .nextInChain = (WGPUChainedStruct*)&canvas_desc,
        .label = "WebGPU Canvas Surface",
    };
    
    return wgpuInstanceCreateSurface(instance, &surface_desc);
#else
    // Native platform surface creation would go here
    return NULL; // Implemented per platform (GLFW, SDL, etc.)
#endif
}
```

## 🔧 Device and Adapter Setup

### Adapter Selection Strategy

```c
// Progressive adapter selection for optimal compatibility
typedef struct {
    WGPUPowerPreference power_preference;
    bool force_fallback_adapter;
    bool require_features[32];  // Feature flags to require
} webgpu_adapter_preferences_t;

void webgpu_request_adapter_async(WGPUInstance instance, 
                                  webgpu_adapter_preferences_t* prefs,
                                  webgpu_adapter_callback_t callback) {
    WGPURequestAdapterOptions options = {
        .nextInChain = NULL,
        .compatibleSurface = prefs->surface,
        .powerPreference = prefs->power_preference,
        .forceFallbackAdapter = prefs->force_fallback_adapter,
    };
    
    wgpuInstanceRequestAdapter(instance, &options, 
        webgpu_adapter_received_callback, callback);
}
```

### Feature Detection

```c
// Query supported features for optimal rendering path selection
typedef struct {
    bool depth_clip_control;        // For reversed-Z depth buffer
    bool timestamp_query;           // For GPU profiling
    bool texture_compression_bc;    // For DXT texture support
    bool texture_compression_etc2;  // For mobile texture support
    bool multisampling;            // For anti-aliasing
} webgpu_feature_support_t;

webgpu_feature_support_t webgpu_query_features(WGPUAdapter adapter) {
    webgpu_feature_support_t features = {0};
    
    size_t feature_count = wgpuAdapterEnumerateFeatures(adapter, NULL);
    WGPUFeatureName* supported_features = malloc(feature_count * sizeof(WGPUFeatureName));
    wgpuAdapterEnumerateFeatures(adapter, supported_features);
    
    for (size_t i = 0; i < feature_count; i++) {
        switch (supported_features[i]) {
            case WGPUFeatureName_DepthClipControl:
                features.depth_clip_control = true; break;
            case WGPUFeatureName_TimestampQuery:
                features.timestamp_query = true; break;
            // ... other features
        }
    }
    
    free(supported_features);
    return features;
}
```

## 📊 Resource Management

### Buffer Management Strategy

```c
// Efficient buffer allocation with usage hints
typedef enum {
    WEBGPU_BUFFER_STATIC,   // Rarely updated (geometry)
    WEBGPU_BUFFER_DYNAMIC,  // Updated per frame (uniforms)
    WEBGPU_BUFFER_STREAM,   // Updated multiple times per frame
} webgpu_buffer_usage_hint_t;

WGPUBuffer webgpu_create_buffer_with_hint(WGPUDevice device, 
                                          size_t size,
                                          WGPUBufferUsageFlags usage,
                                          webgpu_buffer_usage_hint_t hint) {
    WGPUBufferDescriptor desc = {
        .size = size,
        .usage = usage,
        .mappedAtCreation = false,
    };
    
    // Optimize based on usage pattern
    switch (hint) {
        case WEBGPU_BUFFER_STATIC:
            // Prefer device memory for static data
            break;
        case WEBGPU_BUFFER_DYNAMIC:
            // Add copy destination for updates
            desc.usage |= WGPUBufferUsage_CopyDst;
            break;
        case WEBGPU_BUFFER_STREAM:
            // Consider mapped buffers for frequent updates
            desc.mappedAtCreation = true;
            break;
    }
    
    return wgpuDeviceCreateBuffer(device, &desc);
}
```

### Texture Loading Pipeline

```c
// Async texture loading with format conversion
typedef struct {
    ecs_entity_t target_entity;
    WGPUDevice device;
    WGPUQueue queue;
    webgpu_texture_complete_callback_t callback;
} webgpu_texture_load_context_t;

void webgpu_load_texture_from_url(const char* url, 
                                  ecs_entity_t entity,
                                  WGPUDevice device,
                                  WGPUQueue queue) {
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    webgpu_texture_load_context_t* ctx = malloc(sizeof(*ctx));
    ctx->target_entity = entity;
    ctx->device = device;
    ctx->queue = queue;
    
    // Use Emscripten's async image loading
    emscripten_async_wget_data(url, ctx, 
        webgpu_image_loaded_callback, 
        webgpu_image_error_callback);
#else
    // Native: Use synchronous loading or platform-specific async API
    webgpu_load_texture_sync(url, entity, device, queue);
#endif
}
```

## 🎨 Shader Management

### WGSL Shader Loading

```c
// Cross-platform shader compilation
typedef struct {
    const char* vertex_wgsl;
    const char* fragment_wgsl;
    WGPUVertexBufferLayout* vertex_layouts;
    size_t vertex_layout_count;
    WGPUBindGroupLayout* bind_group_layouts;
    size_t bind_group_layout_count;
} webgpu_shader_desc_t;

WGPURenderPipeline webgpu_create_render_pipeline(WGPUDevice device,
                                                 const webgpu_shader_desc_t* desc) {
    // Create shader modules
    WGPUShaderModuleWGSLDescriptor vertex_wgsl_desc = {
        .chain = { .sType = WGPUSType_ShaderModuleWGSLDescriptor },
        .code = desc->vertex_wgsl,
    };
    
    WGPUShaderModuleDescriptor vertex_module_desc = {
        .nextInChain = (WGPUChainedStruct*)&vertex_wgsl_desc,
    };
    
    WGPUShaderModule vertex_module = wgpuDeviceCreateShaderModule(device, &vertex_module_desc);
    
    // Similar for fragment shader...
    
    // Create pipeline layout
    WGPUPipelineLayoutDescriptor layout_desc = {
        .bindGroupLayoutCount = desc->bind_group_layout_count,
        .bindGroupLayouts = desc->bind_group_layouts,
    };
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);
    
    // Create render pipeline
    WGPURenderPipelineDescriptor pipeline_desc = {
        .layout = layout,
        .vertex = {
            .module = vertex_module,
            .entryPoint = "vs_main",
            .bufferCount = desc->vertex_layout_count,
            .buffers = desc->vertex_layouts,
        },
        .fragment = &(WGPUFragmentState){
            .module = fragment_module,
            .entryPoint = "fs_main",
            .targetCount = 1,
            .targets = &(WGPUColorTargetState){
                .format = WGPUTextureFormat_BGRA8Unorm,
                .writeMask = WGPUColorWriteMask_All,
            },
        },
        .primitive = {
            .topology = WGPUPrimitiveTopology_TriangleList,
            .cullMode = WGPUCullMode_Back,
        },
        .depthStencil = &(WGPUDepthStencilState){
            .format = WGPUTextureFormat_Depth24Plus,
            .depthWriteEnabled = true,
            .depthCompare = WGPUCompareFunction_Less,
        },
    };
    
    return wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);
}
```

## ⚡ Performance Optimizations

### Command Buffer Optimization

```c
// Minimize command encoder state changes
typedef struct {
    WGPUCommandEncoder encoder;
    WGPURenderPassEncoder current_pass;
    WGPURenderPipeline current_pipeline;
    WGPUBindGroup current_bind_groups[4];
    uint32_t active_bind_group_count;
} webgpu_render_state_t;

void webgpu_draw_batch_optimized(webgpu_render_state_t* state,
                                 const webgpu_draw_batch_t* batch) {
    // Only change pipeline if different
    if (state->current_pipeline != batch->pipeline) {
        wgpuRenderPassEncoderSetPipeline(state->current_pass, batch->pipeline);
        state->current_pipeline = batch->pipeline;
    }
    
    // Only update changed bind groups
    for (uint32_t i = 0; i < batch->bind_group_count; i++) {
        if (i >= state->active_bind_group_count || 
            state->current_bind_groups[i] != batch->bind_groups[i]) {
            wgpuRenderPassEncoderSetBindGroup(state->current_pass, i, 
                batch->bind_groups[i], 0, NULL);
            state->current_bind_groups[i] = batch->bind_groups[i];
        }
    }
    state->active_bind_group_count = batch->bind_group_count;
    
    // Execute draw call
    wgpuRenderPassEncoderDrawIndexedIndirect(state->current_pass, 
        batch->indirect_buffer, batch->indirect_offset);
}
```

### Memory Pool Management

```c
// Pool allocator for WebGPU resources
typedef struct {
    WGPUDevice device;
    WGPUBuffer pool_buffer;        // Large buffer for sub-allocation
    size_t pool_size;
    size_t next_offset;
    ecs_vec_t free_chunks;         // Available chunks for reuse
} webgpu_buffer_pool_t;

WGPUBuffer webgpu_pool_allocate(webgpu_buffer_pool_t* pool, 
                               size_t size, 
                               size_t alignment) {
    // Align size to device requirements
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    // Try to reuse freed chunk
    for (int i = 0; i < ecs_vec_count(&pool->free_chunks); i++) {
        webgpu_buffer_chunk_t* chunk = ecs_vec_get(&pool->free_chunks, i);
        if (chunk->size >= aligned_size) {
            // Reuse this chunk
            WGPUBuffer buffer = chunk->buffer;
            ecs_vec_remove(&pool->free_chunks, i);
            return buffer;
        }
    }
    
    // Allocate new chunk from pool
    if (pool->next_offset + aligned_size <= pool->pool_size) {
        // Create buffer view into pool
        // Implementation depends on WebGPU buffer aliasing support
        pool->next_offset += aligned_size;
        return webgpu_create_buffer_view(pool->pool_buffer, 
                                        pool->next_offset - aligned_size, 
                                        aligned_size);
    }
    
    // Pool exhausted - allocate individual buffer
    return webgpu_create_individual_buffer(pool->device, aligned_size);
}
```

## 🔍 Debugging and Profiling

### Browser Integration

```c
// Performance monitoring using browser APIs
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
void webgpu_profile_frame_start(const char* frame_name) {
    EM_ASM({
        if (window.performance && window.performance.mark) {
            window.performance.mark(UTF8ToString($0) + '-start');
        }
    }, frame_name);
}

void webgpu_profile_frame_end(const char* frame_name) {
    EM_ASM({
        if (window.performance && window.performance.mark && window.performance.measure) {
            var name = UTF8ToString($0);
            window.performance.mark(name + '-end');
            window.performance.measure(name, name + '-start', name + '-end');
        }
    }, frame_name);
}
#endif
```

### Error Handling

```c
// Comprehensive error callback system
void webgpu_device_error_callback(WGPUErrorType type, char const* message, void* userdata) {
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
    // Log to browser console for debugging
    EM_ASM({
        console.error('WebGPU Error:', UTF8ToString($0), UTF8ToString($1));
    }, type_str, message);
#endif
}
```

This WebGPU integration provides a robust foundation for high-performance web graphics while maintaining compatibility across different WebGPU implementations and deployment targets.