#!/bin/bash

echo "🔨 Building Flecs WebGPU Engine for Web Deployment"
echo "=================================================="

# Activate Emscripten environment
echo "🔄 Activating Emscripten environment..."
source /Users/Joe/Dev/flecs-dev/emsdk/emsdk_env.sh

# Clean previous builds
echo "🧹 Cleaning previous builds..."
rm -rf .bake_cache
rm -rf bin/Em-debug/
rm -rf web-demo/wasm/

# Build the WebAssembly application with proper exports
echo "⚙️  Building WebAssembly application..."

# Create output directory
mkdir -p web-demo/wasm

# Build directly with emcc for proper web output
emcc \
    -std=gnu99 \
    -DWEBGPU_BACKEND_EMSCRIPTEN \
    -DFLECS_STATIC \
    -I./include \
    -I/Users/Joe/bake/include \
    src/web-main.c \
    src/resources/resource_manager.c \
    src/math/math_utils.c \
    src/rendering/render_system.c \
    src/shaders/shader_sources.c \
    src/geometry/geometry.c \
    /Users/Joe/bake/src/tower_defense/deps/flecs.c \
    /Users/Joe/bake/src/tower_defense/deps/cglm.c \
    /Users/Joe/bake/src/tower_defense/deps/flecs_components_gui.c \
    /Users/Joe/bake/src/tower_defense/deps/flecs_components_input.c \
    /Users/Joe/bake/src/tower_defense/deps/flecs_components_graphics.c \
    /Users/Joe/bake/src/tower_defense/deps/flecs_components_transform.c \
    /Users/Joe/bake/src/tower_defense/deps/flecs_components_geometry.c \
    /Users/Joe/bake/src/tower_defense/deps/flecs_systems_transform.c \
    -sUSE_WEBGPU=1 \
    -sASYNCIFY=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sEXPORTED_FUNCTIONS='["_main","_flecs_webgpu_init","_flecs_webgpu_create_box","_flecs_webgpu_create_rectangle","_flecs_webgpu_delete_entity","_flecs_webgpu_clear_entities","_flecs_webgpu_step","_flecs_webgpu_get_entity_count","_flecs_webgpu_shutdown","_malloc","_free"]' \
    -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -sMODULARIZE=1 \
    -sEXPORT_NAME='FlecsWebGPU' \
    -sINVOKE_RUN=0 \
    --embed-file shaders \
    -o web-demo/wasm/flecs-webgpu.js

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

# Check if the files were created
WASM_FILE="web-demo/wasm/flecs-webgpu.wasm"
JS_FILE="web-demo/wasm/flecs-webgpu.js"

if [ ! -f "$WASM_FILE" ] || [ ! -f "$JS_FILE" ]; then
    echo "❌ WebAssembly files not found"
    exit 1
fi

echo "✅ WebAssembly build completed successfully!"
echo "📦 WASM Output: $WASM_FILE ($(du -h "$WASM_FILE" | cut -f1))"
echo "📦 JS Output: $JS_FILE ($(du -h "$JS_FILE" | cut -f1))"
echo ""
echo "🚀 To run the demo:"
echo "   cd web-demo"
echo "   ./run-demo.sh"
echo ""
echo "🎯 Demo URL: http://localhost:8080"