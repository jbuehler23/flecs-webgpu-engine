# Research Notes

This document captures research and tradeoffs considered during backend selection and initial design.

## WebGPU backend
- Dawn provides a stable native path across platforms and can be used for testing and native deployments.
- Emscripten provides the web deployment path; the project uses emcc for WebAssembly output.

## Design tradeoffs
- Keep ECS-driven rendering: component queries drive rendering submissions.
- Favor predictable performance: minimize allocations on the render-critical path.
- Use instancing when possible to reduce draw calls.

## Implementation insights
- Shader management: keep WGSL shaders as separate source files for easier iteration.
- Buffer updates: use mapped staging uploads for large or frequent updates.
- Testing: validate basic rendering pipelines first (vertex/fragment correctness) before adding advanced features.