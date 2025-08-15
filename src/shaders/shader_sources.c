/**
 * @file shaders/shader_sources.c
 * @brief Embedded WGSL shader sources.
 */

#include "../private_api.h"

const char *basic_vertex_shader_source = R"(
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
}

struct InstanceInput {
    @location(3) model_matrix_0: vec4<f32>,
    @location(4) model_matrix_1: vec4<f32>,
    @location(5) model_matrix_2: vec4<f32>,
    @location(6) model_matrix_3: vec4<f32>,
    @location(7) color: vec3<f32>,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) color: vec3<f32>,
}

struct Camera {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    view_projection: mat4x4<f32>,
}

@group(0) @binding(0)
var<uniform> camera: Camera;

@vertex
fn vs_main(vertex: VertexInput, instance: InstanceInput) -> VertexOutput {
    let model_matrix = mat4x4<f32>(
        instance.model_matrix_0,
        instance.model_matrix_1,
        instance.model_matrix_2,
        instance.model_matrix_3,
    );
    
    let world_position = model_matrix * vec4<f32>(vertex.position, 1.0);
    let world_normal = normalize((model_matrix * vec4<f32>(vertex.normal, 0.0)).xyz);
    
    var out: VertexOutput;
    out.clip_position = camera.view_projection * world_position;
    out.world_position = world_position.xyz;
    out.world_normal = world_normal;
    out.uv = vertex.uv;
    out.color = instance.color;
    
    return out;
}
)";

const char *basic_fragment_shader_source = R"(
struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) color: vec3<f32>,
}

struct Light {
    direction: vec3<f32>,
    color: vec3<f32>,
    ambient: vec3<f32>,
    intensity: f32,
}

@group(1) @binding(0)
var<uniform> light: Light;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let normal = normalize(in.world_normal);
    let light_dir = normalize(-light.direction);
    
    // Lambertian diffuse lighting
    let ndotl = max(dot(normal, light_dir), 0.0);
    let diffuse = light.color * light.intensity * ndotl;
    
    // Combine lighting with material color
    let final_color = in.color * (light.ambient + diffuse);
    
    return vec4<f32>(final_color, 1.0);
}
)";