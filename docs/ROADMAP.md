# Development Roadmap

This document outlines the implementation phases and milestones for the Flecs WebGPU rendering system.

## Project Objectives

Primary goal: deliver a production-grade web-native rendering engine that integrates with the Flecs ECS ecosystem.

Success criteria:
- API complexity similar to the Sokol-based system
- Competitive runtime performance in browsers
- Small WebAssembly binary for basic scenes
- Smooth rendering on modern hardware at typical scene sizes
- Compatibility with existing Flecs Hub components

## Implementation Phases

### Phase 1: Foundation
Status: In progress

Completed:
- Architecture design and research
- WebGPU backend evaluation and decisions
- Project structure and core component definitions

Current work:
- WebGPU device initialization and error handling
- Canvas surface creation and management
- Minimal renderer and shader proof-of-concept

Upcoming:
- Build system integration for native and web targets
- Basic error handling and developer diagnostics

Deliverables:
- Minimal WebGPU renderer displaying simple geometry
- Cross-platform build configuration
- Component interfaces compatible with Flecs

### Phase 2: Core Rendering
Status: planned

Goals:
- Vertex/index buffer management and uploads
- Primitive generation (Box, Rectangle, Sphere)
- Automatic instancing for identical geometry
- Transform matrix extraction from Flecs components
- Basic material and uniform handling
- Texture loading and sampling
- Query-driven entity discovery and efficient updates

Deliverables:
- Primitive rendering with instancing
- Integration with Flecs component lifecycle

### Phase 3: Advanced Features
Status: planned

Goals:
- PBR material workflow and lighting support
- Shadow mapping and multi-pass rendering
- HDR and tone mapping
- Depth buffer and anti-aliasing options
- Performance-oriented GPU resource management and streaming

Deliverables:
- Realistic lighting and material support
- Efficient resource management for web deployment

### Phase 4: Web Optimization
Status: planned

Goals:
- Binary size reduction and startup time improvements
- Responsive canvas support and high-DPI handling
- Progressive loading for assets and shaders
- Developer tooling: hot reload for shaders, profiling hooks

Deliverables:
- Optimized WebAssembly artifacts and deployment guidance

### Phase 5: Polish & Documentation
Status: planned

Goals:
- Performance tuning and profiling
- API review and simplification
- Comprehensive documentation and examples
- CI and testing for cross-platform stability

Deliverables:
- Production-ready code, docs, and examples

## Milestones

- Milestone 1: Basic WebGPU integration and "hello triangle"
- Milestone 2: Basic primitives, instancing, and Flecs integration
- Milestone 3: Lighting, shadows, and texture support
- Milestone 4: Web optimizations and developer tooling
- Milestone 5: Final polish and community release

## Iteration Strategy

Weekly cadence recommended:
- Monday: plan and prioritize
- Wednesday: mid-week review
- Friday: demo and review

Feedback channels:
- Community input through Flecs Discord and issue tracker
- Regular cross-platform testing on Chrome, Firefox, and Safari

## Progress (short)

Current focus: foundation and basic renderer. Next: integrate with Flecs components and stabilize the build for web and native targets.

---

This roadmap provides a clear path from basic WebGPU integration to a production-ready web-native rendering engine that maintains the elegance and performance of the Flecs ecosystem.