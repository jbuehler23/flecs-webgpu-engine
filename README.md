# Flecs WebGPU Systems

A web-native rendering engine for [Flecs ECS](https://github.com/SanderMertens/flecs) built on WebGPU, designed for high-performance web applications and games.

## 🎯 Project Vision

Create a **web-first** engine that provides the shortest path from Flecs ECS code to visual output in browsers, while maintaining the simplicity and performance characteristics of the existing Flecs ecosystem.

## ✨ Key Features

- **Web-Native**: Built specifically for optimal browser performance using WebGPU
- **ECS Integration**: Deep integration with Flecs component system and queries
- **Modern Graphics**: PBR materials, post-processing effects, and efficient instancing
- **Cross-Platform**: Runs on desktop (via Dawn) and web (via Emscripten) with same API
- **Component Compatible**: Works seamlessly with existing Flecs Hub modules

## 🚀 Quick Start

```cpp
#include <flecs.h>
#include <flecs_systems_webgpu.h>

int main() {
    flecs::world world;
    
    // Import WebGPU rendering system
    world.import<flecs::systems::webgpu>();
    
    // Create a simple colored box
    auto entity = world.entity()
        .set<flecs::components::transform::Position3>({0, 0, 0})
        .set<flecs::components::geometry::Box>({1, 1, 1})
        .set<flecs::components::graphics::Rgb>({1, 0, 0});
    
    // Automatic rendering via query systems
    while (world.progress()) { }
    
    return 0;
}
```

## 📚 Documentation

- [Architecture Overview](docs/ARCHITECTURE.md) - System design and component integration
- [Research & Analysis](docs/RESEARCH.md) - Findings from existing Sokol implementation
- [WebGPU Integration](docs/WEBGPU.md) - Technical details of WebGPU backend selection
- [Build System](docs/BUILD.md) - Bake/CMake configuration for web and native targets
- [Development Plan](docs/ROADMAP.md) - Implementation phases and milestones
- [API Reference](docs/API.md) - Component schemas and system interfaces

## 🏗️ Current Status

This project is in **active development**. See [ROADMAP.md](docs/ROADMAP.md) for current progress and upcoming milestones.

- ✅ Architecture design complete
- ✅ WebGPU backend evaluation complete  
- 🚧 Basic renderer prototype in progress
- ⏳ Flecs query integration pending
- ⏳ WebAssembly build configuration pending

## 🤝 Contributing

This project is part of the Flecs community initiative to build a web-native engine. Contributions welcome!

## 📄 License

MIT License - See [LICENSE](LICENSE) for details.

## 🙏 Acknowledgments

- Built on the foundation of [flecs-systems-sokol](https://github.com/flecs-hub/flecs-systems-sokol)
- Inspired by the Flecs ECS framework by Sander Mertens
- WebGPU implementation powered by Google's Dawn and Mozilla's wgpu-native