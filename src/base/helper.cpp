//
// Created by aidar on 28.05.25.
//

#include "helper.h"

#include "webgpu/wgpu.h"


WGPUShaderModule createShader(WGPUDevice device, const char *label, const char *filepath) {
    std::FILE *file = fopen(filepath, "rb");
    if (!file) {
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *code = new char[size + 1];
    fread(code, size, 1, file);

    code[size] = '\0';

    WGPUShaderSourceWGSL wgsl = {};
    wgsl.chain = {.sType = WGPUSType_ShaderSourceWGSL};
    wgsl.code = {code,WGPU_STRLEN};

    WGPUShaderModuleDescriptor desc = {};
    desc.nextInChain = (WGPUChainedStruct *) &wgsl;
    desc.label = {label,WGPU_STRLEN};
    auto shaderModule = wgpuDeviceCreateShaderModule(device, &desc);

    if (file) {
        fclose(file);
    }

    if (code) {
        delete[] code;
    }

    return shaderModule;
}

WGPUBuffer createBuffer(WGPUDevice device, WGPUQueue queue, const void *data, size_t size,
                        WGPUBufferUsage usage) {
    WGPUBufferDescriptor desc = {};
    desc.usage = WGPUBufferUsage_CopyDst | usage;
    desc.size = size;
    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);
    wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
    return buffer;
}
