@group(0) @binding(0) var velocity: texture_2d<f32>;
@group(0) @binding(1) var pressure: texture_2d<f32>;
@group(0) @binding(2) var minMaxCurlP:texture_storage_2d<rgba32float,write>;

@compute @workgroup_size(16,16)
fn computeCurlP(@builtin(global_invocation_id) id: vec3<u32>){
    let offset = vec2<u32>(0, 1);

    let vn = textureLoad(velocity,id.xy + offset.xy,0);
    let vs = textureLoad(velocity,id.xy - offset.xy,0);
    let ve = textureLoad(velocity,id.xy + offset.yx,0);
    let vw = textureLoad(velocity,id.xy - offset.yx,0);

    let p = textureLoad(pressure,id.xy,0);
    let curl = (vn.x - vs.x) - (ve.y - vw.y);

    textureStore(minMaxCurlP,id.xy,vec4<f32>(curl,curl,p.x,p.x));
}