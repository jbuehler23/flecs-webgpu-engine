/**
 * @file resources/resource_manager.c
 * @brief WebGPU resource management implementation.
 */

#include "../private_api.h"
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
    
    WGPUShaderModuleWGSLDescriptor wgsl_source_desc = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_ShaderModuleWGSLDescriptor,
        },
        .code = wgsl_source,
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