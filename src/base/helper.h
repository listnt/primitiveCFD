//
// Created by aidar on 28.05.25.
//

#ifndef HELPER_H
#define HELPER_H

#include <cstdio>
#include <webgpu/webgpu.h>

WGPUShaderModule createShader(WGPUDevice device,const char *label, const char *filepath);
WGPUBuffer createBuffer(WGPUDevice device, WGPUQueue queue,const void *data, size_t size, WGPUBufferUsage usage);

#endif //HELPER_H
