#include <flecs.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

int main() {
    // Simplest possible test - just create a Flecs world
    ecs_world_t *world = ecs_init();
    if (!world) {
        return -1;
    }
    
    // Create a simple entity
    ecs_entity_t entity = ecs_new(world);
    
    // Clean up
    ecs_fini(world);
    
    return 0;
}