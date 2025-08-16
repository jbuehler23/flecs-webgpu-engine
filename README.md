# Flecs WebGPU Engine

A simple 3D rendering engine for Flecs that runs in web browsers using WebAssembly and WebGPU.

## What is this?

If you're familiar with Flecs ECS and want to render 3D graphics in a web browser, this project gives you:

- **WebGPU rendering**: Uses the new WebGPU API for fast 3D graphics in browsers
- **Component-based**: Works with Flecs components you already know (EcsTransform3, EcsRgb, EcsBox)
- **WebAssembly**: Your C code compiles to WASM and runs directly in the browser
- **No JavaScript required**: Write everything in C using Flecs patterns

## Quick Start

### 1. Build the demo

```bash
# Clone and build
git clone <this-repo>
cd flecs-webgpu-engine

# Build for web (requires Emscripten)
source /path/to/emsdk/emsdk_env.sh
bake --target em

# Start the demo server
cd web-demo
./run-demo.sh
```

Visit `http://localhost:8080/flecs-native.html` to see 5 colored boxes rendered with WebGPU.

### 2. Basic usage in your code

```c
#include "flecs_systems_webgpu.h"

int main() {
    ecs_world_t *world = ecs_init();
    
    // Import the WebGPU rendering system
    FlecsSystemsWebGPUImport(world);
    
    // Create a simple cube
    ecs_entity_t cube = ecs_new(world);
    ecs_set(world, cube, EcsTransform3, {
        .value = GLM_MAT4_IDENTITY_INIT
    });
    ecs_set(world, cube, EcsRgb, {.r = 1.0f, .g = 0.0f, .b = 0.0f});
    ecs_add(world, cube, EcsBox);
    
    // The WebGPU system automatically renders entities with these components
    while (ecs_progress(world, 0)) {
        // Your game loop here
    }
    
    return 0;
}
```

## How it works

The rendering system looks for entities that have:
- `EcsTransform3` - Position/rotation/scale matrix
- `EcsRgb` - Color 
- Geometry component (`EcsBox`, `EcsRectangle`, etc.)

It automatically batches these entities by geometry type and sends them to the GPU in one draw call.

## Browser Requirements

You need a recent browser with WebGPU support:
- **Chrome 113+**: Works out of the box
- **Edge 113+**: Works out of the box  
- **Firefox**: Enable `dom.webgpu.enabled` in about:config
- **Safari**: Enable "WebGPU" in Developer menu

## Building

### For Web (Emscripten)
```bash
# Setup Emscripten (one time)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest

# Build 
source ./emsdk_env.sh
cd flecs-webgpu-engine
bake --target em
```

### For Native (Coming Soon)
```bash
bake  # Will build with Dawn WebGPU for desktop
```

## Project Structure

```
flecs-webgpu-engine/
├── src/
│   ├── main.c              # Demo application + core systems
│   ├── resources/          # GPU buffer/pipeline management  
│   ├── rendering/          # Entity batching and draw calls
│   ├── shaders/            # WGSL vertex/fragment shaders
│   └── geometry/           # Box, rectangle vertex data
├── web-demo/              # HTML demo page
└── include/               # Public API headers
```

## What's Implemented

**Working:**
- Basic box and rectangle rendering
- Component-based entity system
- Automatic GPU batching by geometry type
- WebAssembly build system
- WGSL shader pipeline

**TODO:**
- Texture loading
- More geometry types (spheres, etc.)
- Lighting system improvements
- Native desktop builds
- Documentation improvements

## Troubleshooting

**"Failed to initialize WebGPU"**
- Check if your browser supports WebGPU
- Try enabling "Unsafe WebGPU" in chrome://flags

**"Module loading failed"**
- Check browser console for detailed errors
- Make sure you're serving files over HTTP (not file://)

**"Nothing renders"**
- Check that entities have EcsTransform3, EcsRgb, and a geometry component
- Look for WebGPU errors in browser console

## Contributing

This is a learning project! If you find bugs or want to add features:

1. Open an issue describing what you want to fix/add
2. Keep changes simple and well-documented
3. Test with the web demo

## License

MIT - See LICENSE file