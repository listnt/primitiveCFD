struct Uniforms {
    step: f32,
    dt: f32,
    rho: f32,
    kinematicViscosity: f32,
    velocityIntake: f32
}

@group(0) @binding(0) var inputPressure: texture_2d<f32>;
@group(0) @binding(1) var inputVelocity: texture_2d<f32>;
@group(0) @binding(2) var stencilMask: texture_2d<u32>;
@group(0) @binding(3) var outputPressure: texture_storage_2d<r32float,write>;
@group(0) @binding(4) var<uniform> cfd:Uniforms;

@compute @workgroup_size(16,16)
fn computeNewPressure(@builtin(global_invocation_id) id: vec3<u32>){
    let mask = textureLoad(stencilMask,vec2<u32>(id.x,id.y), 0);

    var e = textureLoad(inputPressure,vec2<u32>(id.x + 1,id.y), 0);
    var w = textureLoad(inputPressure,vec2<u32>(id.x - 1,id.y), 0);
    var n = textureLoad(inputPressure,vec2<u32>(id.x,id.y + 1), 0);
    var s = textureLoad(inputPressure,vec2<u32>(id.x,id.y - 1), 0);

    let minV = vec4<f32>(-10.0,-10.0,-10.0,-10.0);
    let maxV = vec4<f32>(10.0,10.0,10.0,10.0);

    e = clamp(e,minV, maxV);
    w = clamp(w,minV, maxV);
    n = clamp(n,minV, maxV);
    s = clamp(s,minV, maxV);

    let p = textureLoad(inputPressure,vec2<u32>(id.x,id.y),0);

    if (mask.r==0 || mask.r == 1){ // Neuman condition if inlet or wall
        let r = textureLoad(stencilMask,vec2<u32>(id.x + 1,id.y), 0);
        let l = textureLoad(stencilMask,vec2<u32>(id.x - 1,id.y), 0);
        let t = textureLoad(stencilMask,vec2<u32>(id.x,id.y + 1), 0);
        let b = textureLoad(stencilMask,vec2<u32>(id.x,id.y - 1), 0);

        if (l.x==255){
            textureStore(outputPressure,vec2<u32>(id.x,id.y), w);
        }else if(r.x==255){
            textureStore(outputPressure,vec2<u32>(id.x,id.y), e);
        }else if(t.x==255){
            textureStore(outputPressure,vec2<u32>(id.x,id.y), n);
        }else if(b.x==255){
            textureStore(outputPressure,vec2<u32>(id.x,id.y), s);
        }else{
            textureStore(outputPressure,vec2<u32>(id.x,id.y), vec4<f32>(0,0,0,0));
        }
    } else if (mask.r == 2){ // if outlet
        textureStore(outputPressure,vec2<u32>(id.x,id.y), vec4<f32>(0,0,0,0));
    }else {
        let ve = textureLoad(inputVelocity,vec2<u32>(id.x + 1,id.y), 0);
        let vw = textureLoad(inputVelocity,vec2<u32>(id.x - 1,id.y), 0);
        let vn = textureLoad(inputVelocity,vec2<u32>(id.x,id.y + 1), 0);
        let vs = textureLoad(inputVelocity,vec2<u32>(id.x,id.y - 1), 0);
        let res = ((n.x + s.x) * cfd.step * cfd.step +
                            (e.x + w.x) * cfd.step * cfd.step) / (4 * cfd.step * cfd.step) -
                           (cfd.step * cfd.step * cfd.step * cfd.step) / (4 * cfd.step * cfd.step) * cfd.rho * 1.0 / cfd.dt *
                           (
                               (ve.x - vw.x) / (2.0 * cfd.step) +
                               (vn.y - vs.y) / (2.0 * cfd.step)
                           );

          textureStore(outputPressure,vec2<u32>(id.x,id.y),vec4<f32>(res,0,0,0));
    }
}