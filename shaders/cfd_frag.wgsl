@group(0) @binding(0) var velocities : texture_2d<f32>;
@group(0) @binding(0) var minMaxCurlP: texture_2d<f32>; // should be 1x1 pixel containing minCurl,maxCurl,minP,maxP

@fragment
fn fs_main(@location(0) vCol : vec4<f32>) -> @location(0) vec4<f32> {

    return vCol;
}