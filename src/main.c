/**
 * @file main.c
 * @brief Flecs WebGPU Engine Demo Application - following tower_defense pattern
 */

#include "flecs_systems_webgpu.h"

/* Include the main WebGPU system implementation */
#include "main_internal.c"

/* Global state for demo */
static ecs_world_t *g_world = NULL;
static ecs_entity_t g_renderer = 0;

/* Forward declarations */
static void demo_create_scene(void);

/**
 * Initialize the demo world
 */
static void demo_init() {
    ecs_log_set_level(1); // Enable info logging
    ecs_trace("Initializing Flecs WebGPU demo");
    
    /* Create Flecs world */
    g_world = ecs_init();
    if (!g_world) {
        ecs_err("Failed to create Flecs world");
        return;
    }
    
    /* Import WebGPU systems */
    FlecsSystemsWebGPUImport(g_world);
    
    /* Create canvas entity for web demo */
    ecs_entity_t canvas = ecs_new(g_world);
    ecs_set(g_world, canvas, EcsCanvas, {
        .width = 1024,
        .height = 600
    });
    
    /* Create renderer entity */
    g_renderer = ecs_new(g_world);
    ecs_add(g_world, g_renderer, WebGPURenderer);
    ecs_add(g_world, g_renderer, WebGPUQuery);
    
    /* Create some demo entities */
    demo_create_scene();
    
    ecs_trace("Flecs WebGPU demo initialized successfully");
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
        
        ecs_trace("Created demo box entity %llu at position (%.2f, 0, -5)", 
                 entity, i * 2.0f - 4.0f);
    }
}

/**
 * Main application entry point - like tower_defense
 */
int main() {
    /* Initialize the demo */
    demo_init();
    
    if (!g_world) {
        ecs_err("Failed to initialize demo");
        return -1;
    }
    
    /* Main loop - let Emscripten handle this with emscripten_set_main_loop */
#ifdef __EMSCRIPTEN__
    /* For Emscripten, the main loop is handled by the browser */
    ecs_trace("Flecs WebGPU demo ready for web");
    
    /* Set target FPS for web */
    ecs_set_target_fps(g_world, 60);
    
    /* Run indefinitely - browser controls the loop */
    ecs_progress(g_world, 0); // Initial frame
    
    /* The browser will continue calling our progress function */
    return 0;
#else
    /* Native application loop */
    ecs_trace("Running native application loop");
    
    ecs_set_target_fps(g_world, 60);
    
    while (ecs_progress(g_world, 0)) {
        /* Continue until quit */
    }
    
    /* Cleanup */
    ecs_fini(g_world);
    return 0;
#endif
}