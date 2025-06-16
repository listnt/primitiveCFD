#version 300 es
precision mediump float;

// Input vertex data, different for all executions of this shader.
layout (location = 0) in vec2 vPosition;
layout (location = 3) in vec4 aColor;
layout (location = 2) in float zIndex;

// Values that stay constant for the whole mesh.
uniform mat4 viewModel;
uniform mat4 projection;

out vec4 color;

void main(){
    color = aColor;
    // Output position of the vertex, in clip space : MVP * position
    gl_Position = projection * viewModel * vec4(vPosition.xy, zIndex, 1.0);
    gl_PointSize=10.0;
}
