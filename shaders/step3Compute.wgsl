struct Uniforms {
    step: f32,
    dt: f32,
    rho: f32,
    kinematicViscosity: f32
}

@group(0) @binding(0) var inputPressure: texture_2d<f32>;
@group(0) @binding(1) var inputVelocity: texture_2d<f32>;
@group(0) @binding(2) var stencilMask: texture_2d<u32>;
@group(0) @binding(3) var outputVelocities: texture_storage_2d<rg32float,write>;
@group(0) @binding(4) var<uniform> cfd:Uniforms;

@compute @workgroup_size(16,16)
fn computeFinalVelocites(@builtin(global_invocation_id) id: vec3<u32>){
    let mask = textureLoad(stencilMask,vec2<u32>(id.x,id.y), 0);

    let e = textureLoad(inputPressure,vec2<u32>(id.x + 1,id.y), 0);
    let w = textureLoad(inputPressure,vec2<u32>(id.x - 1,id.y), 0);
    let n = textureLoad(inputPressure,vec2<u32>(id.x,id.y + 1), 0);
    let s = textureLoad(inputPressure,vec2<u32>(id.x,id.y - 1), 0);

    let v = textureLoad(inputVelocity,vec2<u32>(id.x,id.y),0);

    // Dirichet condition, only considers const pressure
    if (mask.r==0){
        textureStore(outputVelocities,vec2<u32>(id.x,id.y), v);
    } else {
        let res =  vec2<f32>(
            v.x - (cfd.dt / cfd.rho * (e.x - w.x) / (2 * cfd.step)),
            v.y - (cfd.dt / cfd.rho * (n.x - s.x) / (2 * cfd.step))
        );

        textureStore(outputVelocities,vec2<u32>(id.x,id.y),vec4<f32>(res.x,res.y,0,0));
    }
}