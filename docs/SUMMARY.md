# Project Summary

This repository contains the Flecs WebGPU rendering integration. It provides a component-driven renderer that targets both native and web (WebAssembly) platforms.

Repository layout (high level):
- project.json — Bake configuration
- include/ — Public API headers (`flecs_systems_webgpu.h`)
- src/ — Core implementation (`main.c`, `private_api.h`, geometry, rendering)
- shaders/ — WGSL shader sources
- docs/ — Documentation and design notes

Key features implemented:
- EcsTransform3: transform matrices converted to instance data
- EcsBox / EcsRectangle: geometry components for primitives
- EcsRgb: per-instance color/material data
- EcsCanvas: canvas resize detection and surface management
- Cross-platform: build targets for web and native via Dawn/emscripten

Next steps:
- Continue core rendering work (buffers, instancing)
- Add material and texture systems
- Improve WebAssembly startup and bundle size
- Add examples and integration tests