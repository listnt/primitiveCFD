struct VertexIn {
    @location(0) aPos : vec2<f32>,
    @location(1) aCol : vec4<f32>,
    @location(2) zIndex : f32,
    @location(3) model_1 :vec4<f32>,
    @location(4) model_2 :vec4<f32>,
    @location(5) model_3 :vec4<f32>,
    @location(6) model_4 :vec4<f32>
}

struct VertexOut {
    @location(0) vCol : vec4<f32>,
    @builtin(position) Position : vec4<f32>
}
struct Uniforms {
    projection: mat4x4<f32>,
    view: mat4x4<f32>,
}
@group(0) @binding(0) var<uniform> pvm: Uniforms;

@vertex
fn vs_main(in : VertexIn) -> VertexOut {
    var model : mat4x4<f32>;
    model = transpose( mat4x4<f32>(in.model_1, in.model_2, in.model_3, in.model_4));

    var output : VertexOut;
    output.Position = vec4<f32>(pvm.projection * pvm.view * model * vec4<f32>(in.aPos, in.zIndex, 1.0));
    output.vCol = in.aCol;
    return output;
}