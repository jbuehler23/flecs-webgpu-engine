/**
 * @file rendering/render_system.c
 * @brief Core rendering system implementation for WebGPU.
 */

#include "../private_api.h"

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
            /* Copy the existing transform matrix */
            glm_mat4_copy(transforms[i].value, transform_matrix);
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
 * Update camera uniform buffer with view/projection matrices
 */
static void update_camera_uniforms(WebGPURenderer *renderer) {
    if (!renderer->camera_uniform_buffer || !renderer->device || !renderer->queue) {
        return;
    }
    
    /* Create camera matrices */
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    
    /* Simple camera setup - looking at origin from distance */
    glm_mat4_identity(view_matrix);
    glm_translate(view_matrix, (vec3){0.0f, 0.0f, -5.0f}); /* Move back 5 units */
    
    /* Perspective projection */
    float aspect = (float)renderer->width / (float)renderer->height;
    glm_perspective(glm_rad(45.0f), aspect, 0.1f, 100.0f, projection_matrix);
    
    /* Combine view and projection */
    glm_mat4_mul(projection_matrix, view_matrix, view_projection_matrix);
    
    /* Pack matrices into buffer layout: view, projection, view_projection */
    float camera_data[48]; /* 3 mat4 matrices = 3 * 16 floats */
    memcpy(&camera_data[0], view_matrix, 16 * sizeof(float));
    memcpy(&camera_data[16], projection_matrix, 16 * sizeof(float));
    memcpy(&camera_data[32], view_projection_matrix, 16 * sizeof(float));
    
    /* Upload to GPU */
    webgpu_update_buffer(renderer->device, renderer->queue, renderer->camera_uniform_buffer, 
                        camera_data, sizeof(camera_data), 0);
}

/**
 * Update light uniform buffer with directional lighting data
 */
static void update_light_uniforms(WebGPURenderer *renderer) {
    if (!renderer->light_uniform_buffer || !renderer->device || !renderer->queue) {
        return;
    }
    
    /* Light data matching our 40-byte shader struct */
    struct {
        float direction_x;      // 4 bytes
        float direction_y;      // 4 bytes  
        float direction_z;      // 4 bytes
        float intensity;        // 4 bytes
        float color_x;          // 4 bytes
        float color_y;          // 4 bytes
        float color_z;          // 4 bytes
        float ambient_strength; // 4 bytes
        float ambient_x;        // 4 bytes (ambient_xy.x)
        float ambient_y;        // 4 bytes (ambient_xy.y)
    } light_data = {
        .direction_x = -0.5f,
        .direction_y = -1.0f,
        .direction_z = -0.3f,
        .intensity = 1.0f,
        .color_x = 1.0f,
        .color_y = 1.0f,
        .color_z = 1.0f,
        .ambient_strength = 0.3f,
        .ambient_x = 0.1f,
        .ambient_y = 0.1f,
    };
    
    /* Upload to GPU */
    webgpu_update_buffer(renderer->device, renderer->queue, renderer->light_uniform_buffer, 
                        &light_data, sizeof(light_data), 0);
}

/**
 * Execute all gathered render batches
 */
void webgpu_execute_render_batches(WebGPURenderer *renderer, 
                                  WGPURenderPassEncoder render_pass) {
    if (!renderer || !render_pass) {
        return;
    }
    
    /* Update uniform buffers with current frame data */
    update_camera_uniforms(renderer);
    update_light_uniforms(renderer);
    
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
        
        /* Bind uniform buffer bind groups */
        if (renderer->camera_bind_group) {
            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, renderer->camera_bind_group, 0, NULL);
        }
        if (renderer->light_bind_group) {
            wgpuRenderPassEncoderSetBindGroup(render_pass, 1, renderer->light_bind_group, 0, NULL);
        }
        
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
 * Import material subsystem (simplified)
 */
void webgpu_material_import(ecs_world_t *world) {
    /* Simplified implementation - just register default material components */
    (void)world;
    ecs_trace("WebGPU: Material subsystem imported (simplified)");
}