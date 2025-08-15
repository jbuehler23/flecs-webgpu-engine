# Build System Guide

This document covers the build configuration for the Flecs WebGPU rendering system, supporting both web (Emscripten) and native (Dawn/wgpu-native) targets.

## 🎯 Build Targets

### Supported Platforms

| Platform | Native Backend | Web Backend | Status |
|----------|---------------|-------------|---------|
| **Linux** | Dawn / wgpu-native | Emscripten + WebGPU | ✅ Supported |
| **macOS** | Dawn / wgpu-native | Emscripten + WebGPU | ✅ Supported |
| **Windows** | Dawn / wgpu-native | Emscripten + WebGPU | ✅ Planned |
| **Web** | N/A | Emscripten + WebGPU | 🚧 Primary Target |

### Build System Matrix

```
┌─────────────────┬──────────────┬─────────────────┐
│ Build System    │ Native       │ Web             │
├─────────────────┼──────────────┼─────────────────┤
│ Bake (Primary)  │ ✅ Supported  │ ✅ Supported     │
│ CMake (Alt)     │ ✅ Supported  │ ✅ Supported     │
│ Emscripten      │ N/A          │ ✅ Required      │
└─────────────────┴──────────────┴─────────────────┘
```

## 🔧 Bake Configuration

### Primary Build Configuration (project.json)

```json
{
    "id": "flecs.systems.webgpu",
    "type": "package",
    "value": {
        "author": "Flecs Community",
        "description": "WebGPU-based rendering module for Flecs (web-native)",
        "use": [
            "flecs",
            "flecs.components.gui",
            "flecs.components.input", 
            "flecs.components.graphics",
            "flecs.components.transform",
            "flecs.components.geometry",
            "flecs.systems.transform"
        ],
        "amalgamate": true
    },
    "lang.c": {
        "${os linux}": {
            "lib": ["m", "dawn_native", "dawn_proc"],
            "ldflags": ["-Wl,-rpath,$ORIGIN"]
        },
        "${os darwin}": {
            "lib": ["dawn_native", "dawn_proc"],
            "ldflags": [
                "-framework Foundation",
                "-framework CoreFoundation", 
                "-framework IOKit",
                "-framework Metal",
                "-framework QuartzCore"
            ]
        },
        "${os windows}": {
            "lib": ["dawn_native", "dawn_proc", "d3d12", "dxgi"]
        },
        "${target em}": {
            "ldflags": [
                "-sUSE_WEBGPU=1",           // Enable WebGPU support
                "-sASYNCIFY=1",             // Support async operations
                "-sALLOW_MEMORY_GROWTH=1",  // Dynamic memory allocation
                "-sINITIAL_MEMORY=64MB",    // Reasonable starting memory
                "-sSTACK_SIZE=5MB",         // Adequate stack for complex scenes
                "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
            ],
            "cflags": [
                "-DWEBGPU_BACKEND_EMSCRIPTEN",
                "-O3",                      // Aggressive optimization for web
                "-flto"                     // Link-time optimization
            ],
            "embed": ["shaders"],          // Embed WGSL shaders in binary
            "${cfg debug}": {
                "ldflags": ["-sASSERTIONS=1", "-sGL_DEBUG=1"]
            },
            "${cfg release}": {
                "ldflags": ["-sASSERTIONS=0", "--closure=1"]
            }
        }
    }
}
```

### Example Application Configuration

```json
{
    "id": "webgpu_demo",
    "type": "application", 
    "value": {
        "use": [
            "flecs",
            "flecs.systems.webgpu"
        ],
        "language": "c++",
        "standalone": true
    },
    "lang.c": {
        "${target em}": {
            "ldflags": ["-sSTACK_SIZE=1000000"],
            "embed": ["assets"]
        }
    }
}
```

### Bake Commands

```bash
# Build for native development (Linux/macOS)
bake build webgpu_demo

# Build for web deployment
bake build webgpu_demo --cfg release --target em

# Run native version
bake run webgpu_demo

# Run web version (starts local server)
bake run webgpu_demo --target em

# Clean build artifacts
bake clean webgpu_demo

# Debug build with assertions
bake build webgpu_demo --cfg debug --target em
```

## 🌐 CMake Configuration

### Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(flecs-systems-webgpu)

# Detect Emscripten build
if(EMSCRIPTEN)
    set(WEBGPU_BACKEND "EMSCRIPTEN" CACHE STRING "WebGPU backend")
else()
    set(WEBGPU_BACKEND "DAWN" CACHE STRING "WebGPU backend (DAWN or WGPU)")
endif()

# WebGPU backend configuration
if(WEBGPU_BACKEND STREQUAL "EMSCRIPTEN")
    # Emscripten WebGPU setup
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    target_compile_definitions(flecs_systems_webgpu PRIVATE 
        WEBGPU_BACKEND_EMSCRIPTEN)
    
    target_link_options(flecs_systems_webgpu PRIVATE
        -sUSE_WEBGPU=1
        -sASYNCIFY=1
        -sALLOW_MEMORY_GROWTH=1
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        --embed-file ${CMAKE_SOURCE_DIR}/shaders@/shaders)
        
elseif(WEBGPU_BACKEND STREQUAL "DAWN")
    # Dawn WebGPU setup
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DAWN REQUIRED dawn)
    
    target_include_directories(flecs_systems_webgpu PRIVATE ${DAWN_INCLUDE_DIRS})
    target_link_libraries(flecs_systems_webgpu PRIVATE ${DAWN_LIBRARIES})
    target_compile_definitions(flecs_systems_webgpu PRIVATE 
        WEBGPU_BACKEND_DAWN)
        
elseif(WEBGPU_BACKEND STREQUAL "WGPU")
    # wgpu-native setup
    find_package(wgpu REQUIRED)
    target_link_libraries(flecs_systems_webgpu PRIVATE wgpu::wgpu)
    target_compile_definitions(flecs_systems_webgpu PRIVATE 
        WEBGPU_BACKEND_WGPU)
        
endif()

# Platform-specific libraries
if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    # Linux
    target_link_libraries(flecs_systems_webgpu PRIVATE m pthread)
elseif(APPLE AND NOT EMSCRIPTEN)
    # macOS
    find_library(FOUNDATION Foundation REQUIRED)
    find_library(METAL Metal REQUIRED)
    find_library(QUARTZCORE QuartzCore REQUIRED)
    target_link_libraries(flecs_systems_webgpu PRIVATE 
        ${FOUNDATION} ${METAL} ${QUARTZCORE})
elseif(WIN32 AND NOT EMSCRIPTEN)
    # Windows
    target_link_libraries(flecs_systems_webgpu PRIVATE d3d12 dxgi)
endif()
```

### Build Commands

```bash
# Native development build
mkdir build-native && cd build-native
cmake .. -DWEBGPU_BACKEND=DAWN
make -j$(nproc)

# Emscripten web build
mkdir build-web && cd build-web
emcmake cmake .. -DWEBGPU_BACKEND=EMSCRIPTEN
emmake make -j$(nproc)

# Alternative backend (wgpu-native)
mkdir build-wgpu && cd build-wgpu
cmake .. -DWEBGPU_BACKEND=WGPU
make -j$(nproc)
```

## 📦 Dependency Management

### Native Dependencies

#### Dawn WebGPU (Recommended)
```bash
# Ubuntu/Debian
sudo apt install libdawn-dev

# macOS with Homebrew
brew install dawn

# Build from source (if packages unavailable)
git clone https://dawn.googlesource.com/dawn
cd dawn
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) && sudo make install
```

#### wgpu-native (Alternative)
```bash
# Download prebuilt binaries
wget https://github.com/gfx-rs/wgpu-native/releases/latest/download/wgpu-native-linux-x86_64.tar.gz
tar -xzf wgpu-native-linux-x86_64.tar.gz
sudo cp -r include/* /usr/local/include/
sudo cp lib/* /usr/local/lib/
```

### Emscripten Setup

```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Verify WebGPU support (Emscripten 3.1.25+)
emcc --show-ports | grep webgpu
```

## 🚀 Development Workflow

### Local Development Server

```bash
# For web builds, use a local server (required for WebGPU)
cd build-web
python3 -m http.server 8080

# Or use Emscripten's built-in server
emrun --browser firefox webgpu_demo.html
```

### Asset Pipeline

```bash
# Shader compilation validation
glslangValidator shaders/scene.wgsl

# Texture optimization for web
convert texture.png -quality 85 texture.jpg
optipng -o7 texture.png
```

### Testing Across Browsers

```javascript
// Browser compatibility check
if (!navigator.gpu) {
    console.error('WebGPU not supported');
    // Fallback or error handling
}

// Feature detection
const adapter = await navigator.gpu.requestAdapter();
const features = Array.from(adapter.features);
console.log('Supported features:', features);
```

## 🔧 Configuration Options

### Compile-Time Features

```c
// Build configuration macros
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    #define WEBGPU_WEB_OPTIMIZATIONS 1
    #define WEBGPU_ASYNC_LOADING 1
#endif

#ifdef WEBGPU_BACKEND_DAWN  
    #define WEBGPU_VALIDATION_LAYERS 1
    #define WEBGPU_DEBUG_LABELS 1
#endif

// Performance tuning
#define WEBGPU_MAX_INSTANCES_PER_BATCH 1000
#define WEBGPU_BUFFER_POOL_SIZE (16 * 1024 * 1024)  // 16MB
#define WEBGPU_SHADER_CACHE_SIZE 64
```

### Runtime Configuration

```c
// Runtime feature detection and configuration
typedef struct {
    bool enable_validation;      // Enable API validation layers
    bool enable_profiling;       // Enable GPU profiling
    uint32_t max_texture_size;   // Maximum texture dimension
    uint32_t msaa_samples;       // MSAA sample count (0 = disabled)
    webgpu_power_preference_t power_preference;
} webgpu_config_t;

webgpu_config_t webgpu_detect_optimal_config(void) {
    webgpu_config_t config = {0};
    
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    // Web: Conservative settings for compatibility
    config.max_texture_size = 2048;
    config.msaa_samples = 4;
    config.power_preference = WEBGPU_POWER_PREFERENCE_LOW_POWER;
#else
    // Native: More aggressive settings
    config.enable_validation = true;
    config.enable_profiling = true;
    config.max_texture_size = 8192;
    config.msaa_samples = 8;
    config.power_preference = WEBGPU_POWER_PREFERENCE_HIGH_PERFORMANCE;
#endif
    
    return config;
}
```

## 📊 Build Optimization

### Size Optimization (Web Builds)

```bash
# Emscripten optimization flags
-O3                    # Aggressive optimization
-flto                  # Link-time optimization  
--closure=1            # Google Closure Compiler
-s FILESYSTEM=0        # Disable filesystem if not needed
-s MODULARIZE=1        # Create modular output
-s EXPORT_NAME=FlecsWebGPU

# Asset optimization
--preload-file assets@/assets    # Preload critical assets
--embed-file shaders@/shaders    # Embed shaders in binary
```

### Performance Optimization

```bash
# Native development optimization
-DCMAKE_BUILD_TYPE=RelWithDebInfo  # Optimized + debug symbols
-DWEBGPU_ENABLE_BACKEND_VALIDATION=ON  # Enable validation
-DDAWN_ENABLE_D3D12=ON             # Enable D3D12 backend (Windows)
-DDAWN_ENABLE_METAL=ON             # Enable Metal backend (macOS)
-DDAWN_ENABLE_VULKAN=ON            # Enable Vulkan backend (Linux)
```

This build system provides flexible cross-platform support while optimizing for each target's specific requirements and capabilities.