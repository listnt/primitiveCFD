struct Uniforms {
    step: f32,
    dt: f32,
    rho: f32,
    kinematicViscosity: f32
}

@group(0) @binding(0) var inputPressure: texture_2d<f32>;
@group(0) @binding(1) var inputVelocity: texture_2d<f32>;
@group(0) @binding(2) var stencilMask: texture_2d<u32>;
@group(0) @binding(3) var outputPressure: texture_storage_2d<r32float,write>;
@group(0) @binding(4) var<uniform> cfd:Uniforms;

@compute @workgroup_size(16,16)
fn computeNewPressure(@builtin(global_invocation_id) id: vec3<u32>){
    let mask = textureLoad(stencilMask,vec2<u32>(id.x,id.y), 0);

    let e = textureLoad(inputPressure,vec2<u32>(id.x + 1,id.y), 0);
    let w = textureLoad(inputPressure,vec2<u32>(id.x - 1,id.y), 0);
    let n = textureLoad(inputPressure,vec2<u32>(id.x,id.y + 1), 0);
    let s = textureLoad(inputPressure,vec2<u32>(id.x,id.y - 1), 0);

    let p = textureLoad(inputPressure,vec2<u32>(id.x,id.y),0);

    // Dirichet condition, only considers const pressure
    if (mask.r==0){
        textureStore(outputPressure,vec2<u32>(id.x,id.y), p);
    }else if (mask.r == 1){ // Neuman condition
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
            textureStore(outputPressure,vec2<u32>(id.x,id.y), p);
        }
    } else {
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