# Flecs WebGPU Rendering Engine

A high-performance web-native rendering system for [Flecs ECS](https://github.com/SanderMertens/flecs) applications. This project addresses the critical gap in modern game engines by providing first-class web deployment capabilities without sacrificing performance or development ergonomics.

## Overview

The traditional approach to web graphics has relied on WebGL, which presents significant limitations for modern game development workflows. WebGPU represents the next evolution of web graphics APIs, providing near-native performance with a modern, compute-capable programming model. This rendering engine leverages WebGPU to deliver a seamless development experience where games can target web browsers as efficiently as native platforms.

The engine maintains complete compatibility with the existing Flecs component ecosystem while providing automatic GPU resource management, efficient entity batching, and modern graphics features. Developers familiar with Flecs will find the rendering integration transparent and intuitive.

## Technical Approach

The architecture follows established ECS patterns where rendering is driven by component queries rather than explicit render calls. Entities with geometric components are automatically discovered, batched by geometry type, and submitted to the GPU with minimal overhead. This approach ensures optimal performance scaling from simple prototypes to complex scenes with thousands of entities.

Key architectural decisions include:
- Component-driven rendering pipeline that integrates naturally with Flecs queries
- Automatic instancing for entities sharing geometry types
- Modern shader pipeline using WGSL (WebGPU Shading Language)
- Cross-platform compatibility targeting both web (Emscripten) and native (Dawn) environments
- Zero-allocation rendering paths for performance-critical applications

## Getting Started

The rendering system integrates seamlessly with existing Flecs applications. Basic usage requires only importing the WebGPU module and using standard Flecs components:

```cpp
#include <flecs.h>
#include <flecs_systems_webgpu.h>

int main() {
    flecs::world world;
    
    // Import WebGPU rendering capabilities
    world.import<flecs::systems::webgpu>();
    
    // Create renderable entities using standard components
    auto cube = world.entity()
        .set<flecs::components::transform::Position3>({0, 0, 0})
        .set<flecs::components::geometry::Box>({1, 1, 1})
        .set<flecs::components::graphics::Rgb>({1, 0, 0});
    
    // Rendering happens automatically during world progression
    while (world.progress()) { }
    
    return 0;
}
```

## Build Requirements

**Native Development:**
- C++11 compatible compiler
- Dawn WebGPU implementation or wgpu-native
- CMake 3.16+ or Bake build system

**Web Deployment:**
- Emscripten SDK 3.1.25+
- WebGPU-capable browser (Chrome 113+, Firefox with experimental features)

## Implementation Status

This project represents a comprehensive foundation for web-native game development. The current implementation includes:

**Completed:**
- Core WebGPU device and resource management
- Component-driven geometry system with automatic instancing
- Modern WGSL shader pipeline with PBR material support
- Cross-platform build configuration for native and web targets
- Comprehensive architecture documentation and technical analysis

**In Progress:**
- GPU resource allocation and buffer management
- Basic primitive rendering (boxes, rectangles)
- Integration testing with existing Flecs Hub modules

**Planned:**
- Advanced lighting models and shadow mapping
- Texture loading and material systems
- Post-processing effects pipeline
- Performance optimization for large-scale scenes

## Technical Documentation

Detailed technical documentation is available in the `docs/` directory:

- `ARCHITECTURE.md` - System design and component integration patterns
- `RESEARCH.md` - Analysis of existing Flecs rendering systems
- `WEBGPU.md` - WebGPU backend selection and integration details
- `BUILD.md` - Cross-platform build configuration
- `ROADMAP.md` - Development timeline and milestones

## Project Philosophy

This engine prioritizes practical game development needs over academic purity. Design decisions favor developer productivity, maintainable code, and predictable performance characteristics. The goal is to enable game developers to ship high-quality web applications without requiring deep graphics programming expertise.

The project maintains strict compatibility with existing Flecs patterns and components, ensuring that adoption requires minimal changes to existing codebases. Performance optimizations are implemented transparently, allowing developers to focus on game logic rather than rendering details.

## License

MIT License - See [LICENSE](LICENSE) for complete terms.

## Acknowledgments

This work builds upon the excellent foundation provided by the Flecs ECS framework and the existing flecs-systems-sokol implementation. Technical decisions were informed by modern graphics programming practices and the specific requirements of web deployment environments.