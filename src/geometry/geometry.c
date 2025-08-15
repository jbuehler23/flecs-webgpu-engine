/**
 * @file geometry/geometry.c
 * @brief Geometry buffer management and primitive generation.
 */

#include "../private_api.h"

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
                
                /* Copy the entire transform matrix from EcsTransform3 */
                glm_mat4_copy(transforms[i].value, *dst_transform);
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
                vec3 scale = {boxes[i].width, boxes[i].height, boxes[i].depth};
                glm_scale(*transform, scale);
            }
        } else if (geometry->component_id == ecs_id(EcsRectangle)) {
            EcsRectangle *rectangles = (EcsRectangle*)geometry_data;
            for (int i = 0; i < it.count; i++) {
                mat4 *transform = ecs_vec_get_t(&geometry->transform_data, mat4, current_count + i);
                
                /* Scale by rectangle dimensions */
                vec3 scale = {rectangles[i].width, rectangles[i].height, 1.0f};
                glm_scale(*transform, scale);
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