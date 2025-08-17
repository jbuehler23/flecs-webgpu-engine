// Basic WebGPU shaders for Flecs geometric primitives
// This implements a simple forward rendering pipeline with per-vertex colors

// Vertex shader uniforms
struct CameraUniforms {
    view_projection_matrix: mat4x4<f32>,
    view_matrix: mat4x4<f32>,
    projection_matrix: mat4x4<f32>,
};

@group(0) @binding(0) var<uniform> camera: CameraUniforms;

// Vertex attributes
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
};

// Instance attributes
struct InstanceInput {
    @location(3) model_matrix_0: vec4<f32>,
    @location(4) model_matrix_1: vec4<f32>,
    @location(5) model_matrix_2: vec4<f32>,
    @location(6) model_matrix_3: vec4<f32>,
    @location(7) color: vec3<f32>,
};

// Vertex shader output / Fragment shader input
struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
    @location(2) color: vec3<f32>,
    @location(3) uv: vec2<f32>,
};

// Fragment shader uniforms (exactly 40 bytes to match buffer layout)
// Using only f32 and vec2 types to avoid WebGPU vec3 padding
struct LightUniforms {
    direction_x: f32,        // 4 bytes
    direction_y: f32,        // 4 bytes
    direction_z: f32,        // 4 bytes
    intensity: f32,          // 4 bytes
    color_x: f32,            // 4 bytes
    color_y: f32,            // 4 bytes
    color_z: f32,            // 4 bytes
    ambient_strength: f32,   // 4 bytes
    ambient_xy: vec2<f32>,   // 8 bytes
                            // Total: 40 bytes exactly
};

@group(1) @binding(0) var<uniform> light: LightUniforms;

// Vertex shader
@vertex
fn vs_main(vertex: VertexInput, instance: InstanceInput) -> VertexOutput {
    // Reconstruct model matrix from instance data
    let model_matrix = mat4x4<f32>(
        instance.model_matrix_0,
        instance.model_matrix_1,
        instance.model_matrix_2,
        instance.model_matrix_3,
    );
    
    // Transform vertex position to world space
    let world_position = model_matrix * vec4<f32>(vertex.position, 1.0);
    
    // Transform normal to world space (assuming uniform scaling)
    let world_normal = normalize((model_matrix * vec4<f32>(vertex.normal, 0.0)).xyz);
    
    var out: VertexOutput;
    out.clip_position = camera.view_projection_matrix * world_position;
    out.world_position = world_position.xyz;
    out.world_normal = world_normal;
    out.color = instance.color;
    out.uv = vertex.uv;
    
    return out;
}

// Fragment shader
@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    // Simple Lambertian diffuse lighting
    let normal = normalize(in.world_normal);
    let light_direction = vec3<f32>(light.direction_x, light.direction_y, light.direction_z);
    let light_dir = normalize(-light_direction);
    let diffuse = max(dot(normal, light_dir), 0.0);
    
    // Combine lighting with instance color
    let light_color = vec3<f32>(light.color_x, light.color_y, light.color_z);
    let ambient_color = vec3<f32>(light.ambient_xy.x, light.ambient_xy.y, light.ambient_strength);
    let ambient_contribution = ambient_color * 0.2;
    let diffuse_contribution = light_color * diffuse * light.intensity;
    let final_color = in.color * (ambient_contribution + diffuse_contribution);
    
    return vec4<f32>(final_color, 1.0);
}

// Alternative simple fragment shader for unlit rendering
@fragment
fn fs_unlit(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(in.color, 1.0);
}