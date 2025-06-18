@group(1) @binding(0) var objTexture : texture_2d<u32>;

@fragment
fn fs_main(@location(0) vCol : vec4<f32>,@location(1) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let texelCoords = vec2i(uv * vec2f(textureDimensions(objTexture)));
    // And we fetch a texel from the texture
    let color:vec4<f32> = vec4<f32>(textureLoad(objTexture, texelCoords, 0).rgba)/255.0;

    return color;
}