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
@group(0) @binding(3) var outputVelocity: texture_storage_2d<rg32float,write>;
@group(0) @binding(4) var<uniform> cfd:Uniforms;

@compute @workgroup_size(16,16)
fn computeFinalVelocites(@builtin(global_invocation_id) id: vec3<u32>){
    let mask = textureLoad(stencilMask,vec2<u32>(id.x,id.y), 0);

    let e = textureLoad(inputPressure,vec2<u32>(id.x + 1,id.y), 0);
    let w = textureLoad(inputPressure,vec2<u32>(id.x - 1,id.y), 0);
    let n = textureLoad(inputPressure,vec2<u32>(id.x,id.y + 1), 0);
    let s = textureLoad(inputPressure,vec2<u32>(id.x,id.y - 1), 0);

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
    }else {
        let res =  vec2<f32>(
            v.x - (cfd.dt / cfd.rho * (e.x - w.x) / (2 * cfd.step)),
            v.y - (cfd.dt / cfd.rho * (n.x - s.x) / (2 * cfd.step))
        );

        textureStore(outputVelocity,vec2<u32>(id.x,id.y),vec4<f32>(res.x,res.y,0,0));
    }
}