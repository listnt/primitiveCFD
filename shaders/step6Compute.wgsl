@group(0) @binding(0) var input: texture_2d<f32>;
@group(0) @binding(1) var minMaxCurlP: texture_2d<f32>;
@group(0) @binding(2) var stencilMask: texture_2d<u32>;
@group(0) @binding(3) var output:texture_storage_2d<rgba8uint,write>;

@compute @workgroup_size(16,16)
fn computeFinalColor(@builtin(global_invocation_id) id: vec3<u32>){
    let mask = textureLoad(stencilMask,vec2<u32>(id.x,id.y), 0);
    if (mask.r == 0){
        textureStore(output,id.xy,vec4<u32>(0,0,0,255));
        return;
    }else if (mask.r == 1){
         textureStore(output,id.xy,vec4<u32>(0,255,255,255));
         return;
    }else if (mask.r == 2){
          textureStore(output,id.xy,vec4<u32>(255,0,255,255));
          return;
    }

    let minMax = textureLoad(minMaxCurlP,vec2<u32>(0,0),0);

    let minV = minMax.x;
    let maxV = minMax.y;

    let curl = textureLoad(input,id.xy,0).x;

    var t : f32 = (curl - minV) / (maxV - minV);

    var startR : f32 = 1.0;
    var startG : f32 = 1.0;
    var startB : f32 = 1.0;

    var endR : f32 = 0.0;
    var endG : f32 = 0.0;
    var endB : f32 = 1.0;

    if (t <= (-minV) / (maxV - minV)) {
        t /= (-minV) / (maxV - minV);

        t *= t; // slower at start

        startR = 1.0;
        startG = 0.0;
        startB = 0.0;

        endR = 1.0;
        endG = 1.0;
        endB = 1.0;
    } else {
        t -= (-minV) / (maxV - minV);
        t /= (-minV) / (maxV - minV);

        t = (3.0 * t * t) - (2.0 * t * t * t);
    }


    let cR : u32 = u32((startR + t * (endR - startR)) * 255.0);
    let cG : u32 = u32((startG + t * (endG - startG)) * 255.0);
    let cB : u32 = u32((startB + t * (endB - startB)) * 255.0);

    textureStore(output,id.xy,vec4<u32>(cR,cG,cB,255));
}