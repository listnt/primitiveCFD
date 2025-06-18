struct Uniforms {
    step: f32,
    dt: f32,
    rho: f32,
    kinematicViscosity: f32,
    velocityIntake: f32
}

@group(0) @binding(0) var inputVelocity: texture_2d<f32>;
@group(0) @binding(1) var stencilMask: texture_2d<u32>;
@group(0) @binding(2) var outputVelocity: texture_storage_2d<rg32float,write>;
@group(0) @binding(3) var<uniform> cfd:Uniforms;

@compute @workgroup_size(16,16)
fn computeNewVelocities(@builtin(global_invocation_id) id: vec3<u32>){
    let mask = textureLoad(stencilMask,vec2<u32>(id.x,id.y), 0);

    let e = textureLoad(inputVelocity,vec2<u32>(id.x + 1,id.y), 0);
    let w = textureLoad(inputVelocity,vec2<u32>(id.x - 1,id.y), 0);
    let n = textureLoad(inputVelocity,vec2<u32>(id.x,id.y + 1), 0);
    let s = textureLoad(inputVelocity,vec2<u32>(id.x,id.y - 1), 0);

    let v = textureLoad(inputVelocity,vec2<u32>(id.x,id.y),0);

    if (mask.r == 1){
        textureStore(outputVelocity,vec2<u32>(id.x,id.y), vec4<f32>(cfd.velocityIntake,0,0,0));
    }else if (mask.r ==0){ // if wall
        textureStore(outputVelocity,vec2<u32>(id.x,id.y), vec4<f32>(0,0,0,0));
    }else if (mask.r == 2){ // if outlet
        let r = textureLoad(stencilMask,vec2<u32>(id.x + 1,id.y), 0);
        let l = textureLoad(stencilMask,vec2<u32>(id.x - 1,id.y), 0);
        let t = textureLoad(stencilMask,vec2<u32>(id.x,id.y + 1), 0);
        let b = textureLoad(stencilMask,vec2<u32>(id.x,id.y - 1), 0);

        if (l.x==255){
            textureStore(outputVelocity,vec2<u32>(id.x,id.y), w);
        }else if(r.x==255){
            textureStore(outputVelocity,vec2<u32>(id.x,id.y), e);
        }else if(t.x==255){
            textureStore(outputVelocity,vec2<u32>(id.x,id.y), n);
        }else if(b.x==255){
            textureStore(outputVelocity,vec2<u32>(id.x,id.y), s);
        }else{
            textureStore(outputVelocity,vec2<u32>(id.x,id.y), vec4<f32>(0,0,0,0));
        }
    }else{
        let fx = upwind(v.x);
        let fy = upwind(v.y);

        let ve1 = v.x * fx.x + e.x * fx.y;
        let vw1 = w.x * fx.x + v.x * fx.y;
        let vn1 = v.x * fy.x + n.x * fy.y;
        let vs1 = s.x * fy.x + v.x * fy.y;

       let a1 = v.x * cfd.dt/cfd.step * (ve1 - vw1);
       let a2 = v.y * cfd.dt/cfd.step * (vn1 - vs1);
       let a3 = cfd.kinematicViscosity * (
                                         cfd.dt / (cfd.step * cfd.step) * (e.x - 2 * v.x + w.x) +
                                         cfd.dt / (cfd.step * cfd.step) * (n.x - 2 * v.x + s.x)
                                     );
       let a4 = v.x - a1 - a2 + a3;

          let ve2 = v.y * fx.x + e.y * fx.y;
          let vw2 = w.y * fx.x + v.y * fx.y;
          let vn2 = v.y * fy.x + n.y * fy.y;
          let vs2 = s.y * fy.x + v.y * fy.y;

       let b1 = v.x * cfd.dt/cfd.step * (ve2 - vw2);
       let b2 = v.y * cfd.dt/cfd.step * (vn2 - vs2);
       let b3 = cfd.kinematicViscosity * (
                                         cfd.dt / (cfd.step * cfd.step) * (e.y - 2 * v.y + w.y) +
                                         cfd.dt / (cfd.step * cfd.step) * (n.y - 2 * v.y + s.y)
                                     );
       let b4 = v.y - b1 - b2 + b3;
       textureStore(outputVelocity,vec2<u32>(id.x,id.y),vec4<f32>(a4,b4,0,0));
    }
}

fn upwind(c: f32) -> vec2<f32>{
    return vec2<f32>(max(c / (abs(c) + 1e-6), 0.0), max(-c / (abs(c) + 1e-6), 0.0));
}