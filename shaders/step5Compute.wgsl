@group(0) @binding(0) var input: texture_2d<f32>;
@group(0) @binding(1) var output:texture_storage_2d<rgba32float,write>;

@compute @workgroup_size(16,16)
fn computeMinMax(@builtin(global_invocation_id) id: vec3<u32>){
    let offset = vec2<u32>(0, 1);

    let v0 = textureLoad(input,2 * id.xy + offset.xx,0);
    let vn = textureLoad(input,2 * id.xy + offset.xy,0);
    let ve = textureLoad(input,2 * id.xy + offset.yx,0);
    let vne = textureLoad(input,2 * id.xy + offset.yy,0);

    let minCurl = min(min(v0.x,ve.x), min(vn.x,vne.x));
    let maxCurl = max(max(v0.y,ve.y), max(vn.y,vne.y));

    let minP = min(min(v0.z,ve.z), min(vn.z,vne.z));
    let maxP = max(max(v0.a,ve.a), max(vn.a,vne.a));

    textureStore(output,id.xy,vec4<f32>(minCurl,maxCurl,minP,maxP));
}