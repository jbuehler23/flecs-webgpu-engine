/**
 * Simple compilation test for flecs-systems-webgpu
 */

#include <stdio.h>

// For standalone build, we'll mock the required types and functions
#ifdef FLECS_SYSTEMS_WEBGPU_STANDALONE_BUILD

// Mock Flecs types for compilation test
typedef struct ecs_world_t ecs_world_t;
typedef struct ecs_iter_t ecs_iter_t;
typedef struct ecs_query_t ecs_query_t;
typedef struct ecs_allocator_t ecs_allocator_t;
typedef struct ecs_vec_t ecs_vec_t;
typedef unsigned long long ecs_id_t;
typedef unsigned long long ecs_entity_t;

// Mock WebGPU types for compilation test
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

typedef unsigned int WGPUBufferUsageFlags;
typedef unsigned int WGPUTextureFormat;

// Mock Flecs component types
typedef struct { float value[4][4]; } EcsTransform3;
typedef struct { float r, g, b; } EcsRgb;
typedef struct { float width, height, depth; } EcsBox;
typedef struct { float width, height; } EcsRectangle;
typedef struct { unsigned int width, height; } EcsCanvas;

// Mock functions
#define ecs_err(...) printf(__VA_ARGS__)
#define ecs_warn(...) printf(__VA_ARGS__)
#define ecs_trace(...) printf(__VA_ARGS__)
#define ecs_os_malloc_t(T) ((T*)malloc(sizeof(T)))
#define ecs_os_malloc(size) malloc(size)
#define ecs_os_free(ptr) free(ptr)
#define ecs_os_memset_t(ptr, value, T) memset(ptr, value, sizeof(T))
#define ecs_id(T) 0
#define ecs_get_name(world, id) "test"
#define ecs_vec_count(vec) 0
#define ecs_vec_first(vec) NULL
#define ecs_vec_clear(vec) do {} while(0)
#define ecs_vec_append_t(alloc, vec, T) ((T*)NULL)
#define flecs_allocator_init(alloc) do {} while(0)
#define flecs_allocator_fini(alloc) do {} while(0)
#define ecs_vec_init_t(alloc, vec, T, size) do {} while(0)
#define ecs_vec_fini_t(alloc, vec, T) do {} while(0)

void* malloc(size_t size);
void free(void* ptr);
void* memset(void* ptr, int value, size_t size);
float tanf(float x);
void* memcpy(void* dest, const void* src, size_t n);

#include "../src/private_api.h"

#endif

int main() {
    printf("Compilation test passed!\n");
    return 0;
}