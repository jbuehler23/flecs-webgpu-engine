# Research & Analysis

This document provides technical analysis of the existing Flecs rendering ecosystem and documents the design decisions for the WebGPU-based implementation.

## Existing Sokol Architecture Analysis

Analysis of the `flecs-systems-sokol` implementation revealed several patterns that inform the WebGPU architecture design:

**Query-Driven Rendering**
The Sokol implementation uses Flecs queries to discover renderable entities each frame rather than maintaining explicit entity lists:

```c
ecs_iter_t qit = ecs_query_iter(world, renderer->solid_query);
while (ecs_query_next(&qit)) {
    EcsTransform3 *transforms = ecs_field(&qit, EcsTransform3, 0);
    EcsRgb *colors = ecs_field(&qit, EcsRgb, 1);
    // Process entities in batches for GPU upload
}
```

This pattern enables dynamic scene composition without manual entity tracking. Entities become renderable simply by adding appropriate components, and stop being rendered when components are removed.

**Automatic Instancing**
Entities with identical geometry types are automatically batched for efficient GPU rendering:

```c
sokol_populate_buffers(geometry, buffers, query);
// Uploads transform matrices, colors, materials as instance data
```

This approach significantly reduces draw calls for scenes with many similar objects, which is common in game environments.

**Component-Based Resource Management**
GPU resources are managed through ECS components with proper lifecycle integration:

```c
typedef struct {
    ecs_allocator_t *allocator;
    sokol_geometry_buffers_t solid;  // GPU buffers for solid geometry
    void (*populate)(mat4*, void*, int32_t, bool);  // Geometry-specific logic
} SokolGeometry;
```

This ensures that GPU resources are properly allocated and deallocated in response to entity lifecycle events.

### Build System Integration

#### WebAssembly Configuration
```json
"${target em}": {
    "ldflags": ["-s USE_WEBGL2=1"],
    "embed": ["etc/sokol/shaders"]
}
```

**Key Finding**: Sokol uses WebGL2 with embedded shaders. Our WebGPU system will use similar patterns but with modern graphics APIs.

#### Module Dependencies
```json
"use": [
    "flecs",
    "flecs.components.gui",
    "flecs.components.graphics", 
    "flecs.components.transform",
    "flecs.components.geometry",
    "flecs.systems.transform"
]
```

**Key Finding**: Heavy reliance on Flecs Hub component modules - our system must maintain this compatibility.

## 🌐 WebGPU Backend Evaluation

### Dawn vs wgpu-native Comparison

| Factor | Dawn | wgpu-native |
|--------|------|-------------|
| **Maturity** | More advanced implementation | Catching up, rapid development |
| **Language** | C++ implementation | Rust implementation with C bindings |
| **Browser Support** | Chrome/Chromium native | Firefox/Deno native |
| **Error Messages** | Superior debugging experience | Good but less detailed |
| **Build Integration** | Mature CMake support | Growing ecosystem |
| **Emscripten Support** | Official `-sUSE_WEBGPU` integration | Custom bindings available |

### Decision: Dawn Primary, wgpu-native Secondary

**Rationale**:
1. **Emscripten Integration**: Dawn has official Emscripten support via `-sUSE_WEBGPU`
2. **Current Maturity**: Dawn is ahead in implementation completeness
3. **Shared Headers**: Both implementations are moving toward shared `webgpu.h` headers
4. **Flexibility**: Architecture allows switching backends via build configuration

### Emscripten WebGPU Integration

#### Official Approach (Recommended)
```cmake
target_link_libraries(App PRIVATE webgpu)
target_compile_definitions(App PRIVATE WEBGPU_BACKEND_EMSCRIPTEN)
# Uses emdawnwebgpu - Emscripten Dawn WebGPU bindings
```

#### Alternative: Custom Bindings
- `juj/wasm_webgpu`: Optimized for minimal code size
- Direct JavaScript API mapping
- Better for size-constrained scenarios

**Decision**: Use official Emscripten WebGPU support for broader compatibility and future-proofing.

## 🏗️ Flecs Integration Patterns

### System Registration Pattern
```c
// Pattern from Sokol: Systems registered in specific pipeline phases
ECS_SYSTEM(world, SokolPopulateGeometry, EcsPreStore, Geometry, [in] GeometryQuery);
ECS_SYSTEM(world, SokolRender, EcsOnStore, Renderer, Query);
```

**Insight**: Rendering happens in `EcsOnStore` phase, after transform calculations in `EcsPreStore`.

### Query Construction Pattern
```c
// Dynamic query creation based on geometry components
ecs_query_desc_t desc = {
    .terms = {{
        .id = ecs_id(EcsTransform3), .inout = EcsIn
    }, {
        .id = ecs_id(EcsRgb), .inout = EcsIn  
    }, {
        .id = geometry_component_id, .inout = EcsIn
    }}
};
```

**Insight**: Queries are constructed programmatically to handle different geometry types while maintaining consistent data access patterns.

### Component Lifecycle Management
```c
ECS_CTOR(SokolGeometry, ptr, {
    ptr->allocator = ecs_os_malloc_t(ecs_allocator_t);
    flecs_allocator_init(ptr->allocator);
    sokol_geometry_buffers_init(ptr->allocator, &ptr->solid);
})
```

**Insight**: Heavy use of Flecs lifecycle hooks for proper resource management, essential for GPU resources.

## 💡 Key Architectural Insights

### 1. **Separation of Concerns**
- **Geometry Module**: Manages buffers and instance data
- **Renderer Module**: Handles GPU state and draw calls  
- **Resource Module**: Manages textures, shaders, static geometry

### 2. **Event-Driven Updates**
- Resources only updated when components change
- Change detection prevents unnecessary GPU uploads
- Frame-based gathering of all renderable entities

### 3. **Web-Specific Considerations**
- Async resource loading patterns needed
- Canvas resize handling for responsive design
- Browser performance API integration for profiling

## 🎯 Technical Requirements Derived

Based on our analysis, the WebGPU system must provide:

1. **API Compatibility**: Same component interfaces as Sokol system
2. **Performance Parity**: Automatic instancing and efficient batching
3. **Web Optimization**: Async loading, minimal WASM size, browser integration
4. **Extensibility**: Plugin system for custom geometry and effects
5. **Debug Experience**: Browser dev tools integration and performance monitoring

## 📋 Validation Criteria

Our implementation will be considered successful when:

- [ ] Existing Flecs Hub components work without modification
- [ ] Performance matches or exceeds Sokol implementation in browsers
- [ ] API complexity remains equivalent to current system
- [ ] WebAssembly binary size stays under 1MB for basic scenes
- [ ] 60fps rendering of 10k+ entities in modern browsers
- [ ] <100ms cold start time in web browsers

This research forms the foundation for our architectural decisions and implementation approach.