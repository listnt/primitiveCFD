//
// Created by aidar on 24.05.25.
//

#include "CFD.h"

CFD::CFD(float step, WGPUDevice device, WGPUQueue queue, WGPUBindGroupLayout meshDependentLayout) {
    this->step = step;

    float CFL = 0.9; //the Courant number is always equal to 0.9

    lx = 64;
    ly = 48;
    dt = CFL * step * step / 2; // compute dt based on Courant number

    Nx = lx / step;
    Ny = ly / step;

    spheres = {
        sphere{
            .x = static_cast<int>((12.0 / lx) * Nx),
            .y = static_cast<int>((24.0 / ly) * Ny),
            .r = static_cast<int>((2.0 / lx) * Nx)
        },
        sphere{
            .x = static_cast<int>((25.0 / lx) * Nx),
            .y = static_cast<int>((34.0 / ly) * Ny),
            .r = static_cast<int>((2.0 / lx) * Nx)
        },
        sphere{
            .x = static_cast<int>((25.0 / lx) * Nx),
            .y = static_cast<int>((14.0 / ly) * Ny),
            .r = static_cast<int>((2.0 / lx) * Nx)
        },
    };


    int i = 0;
    int j = 0;

    for (int i = 0; i < Ny; i++) {
        float y = i * step;
        velocity.push_back(std::vector<Vector2f>());
        pressure.push_back(std::vector<float>());

        newVelocity.push_back(std::vector<Vector2f>());
        newPressure.push_back(std::vector<float>());

        stencilMask.push_back(std::vector<int>());
        for (int j = 0; j < Nx; j++) {
            velocity[i].push_back(Vector2f(0, 0));
            pressure[i].push_back(0);

            newVelocity[i].push_back(Vector2f(0, 0));
            newPressure[i].push_back(0);

            stencilMask[i].push_back(255);
        }
    }

    points = {
        Vector2f(0.0, 0.0),
        Vector2f(0.0, ly / 2.0),
        Vector2f(lx / 2.0, ly / 2.0),
        Vector2f(lx / 2.0, 0.0),

        Vector2f(lx / 2.0, 0.0),
        Vector2f(lx / 2.0, ly / 2.0),
        Vector2f(lx, ly / 2.0),
        Vector2f(lx, 0.0),

        Vector2f(0.0, ly / 2.0),
        Vector2f(0.0, ly),
        Vector2f(lx / 2.0, ly),
        Vector2f(lx / 2.0, ly / 2.0),

        Vector2f(lx / 2.0, ly / 2.0),
        Vector2f(lx / 2.0, ly),
        Vector2f(lx, ly),
        Vector2f(lx, ly / 2.0),
    };

    triangles = {
        0, 1, 2,
        0, 2, 3,

        4, 5, 6,
        4, 6, 7,

        8, 9, 10,
        8, 10, 11,

        12, 13, 14,
        12, 14, 15
    };

    colors = {
        Vector4f(1, 1, 1, 1),
        Vector4f(1, 1, 1, 1),
        Vector4f(1, 1, 1, 1),
        Vector4f(1, 1, 1, 1),

        Vector4f(1, 0, 1, 1),
        Vector4f(1, 0, 1, 1),
        Vector4f(1, 0, 1, 1),
        Vector4f(1, 0, 1, 1),

        Vector4f(1, 0, 1, 1),
        Vector4f(1, 0, 1, 1),
        Vector4f(1, 0, 1, 1),
        Vector4f(1, 0, 1, 1),

        Vector4f(1, 1, 1, 1),
        Vector4f(1, 1, 1, 1),
        Vector4f(1, 1, 1, 1),
        Vector4f(1, 1, 1, 1),
    };

    uv = {
        Vector2f(0, 0),
        Vector2f(0, 0.5),
        Vector2f(0.5, 0.5),
        Vector2f(0.5, 0),

        Vector2f(0.5, 0),
        Vector2f(0.5, 0.5),
        Vector2f(1.0, 0.5),
        Vector2f(1.0, 0),

        Vector2f(0, 0.5),
        Vector2f(0, 1.0),
        Vector2f(0.5, 1.0),
        Vector2f(0.5, 0.5),

        Vector2f(0.5, 0.5),
        Vector2f(0.5, 1.0),
        Vector2f(1.0, 1.0),
        Vector2f(1.0, 0.5),
    };

    pickingColor = std::vector<Vector4f>(points.size(), {1, 1, 1, 1});
    zIndex = std::vector<float>(points.size(), -0.5);

    InitGrid();

    LoadWGPUBuffers(device, queue);

    InitBuffers(device, queue);
    InitBindGroupLayout(device, queue);
    InitComputePipelines(device, queue);
    InitTexture(device, queue);
    InitTextureViews(device, queue);

    InitRenderBindGroup(device, queue, meshDependentLayout);
}

void CFD::InitBuffers(WGPUDevice device, WGPUQueue queue) {
    WGPUBufferDescriptor bufferDescriptor = {};
    bufferDescriptor.size = sizeof(float) * triangles.size();
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bufferDescriptor.label = "EBO";
    bufferDescriptor.mappedAtCreation = false;
    EBO = wgpuDeviceCreateBuffer(device, &bufferDescriptor);

    wgpuQueueWriteBuffer(queue, EBO, 0, triangles.data(), sizeof(int) * triangles.size());

    WGPUBufferDescriptor pressureBuffer = {};
    pressureBuffer.size = colors.size() * sizeof(Vector4f);
    pressureBuffer.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    pressureBuffer.label = "PressureColor";
    pressureBuffer.mappedAtCreation = false;
    PressureColor = wgpuDeviceCreateBuffer(device, &pressureBuffer);

    wgpuQueueWriteBuffer(queue, PressureColor, 0, colors.data(), colors.size() * sizeof(Vector4f));

    WGPUBufferDescriptor pressureModel = {};
    pressureModel.size = sizeof(ModelMat);
    pressureModel.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    pressureModel.label = "PressureModel";
    pressureModel.mappedAtCreation = false;
    PressureModel = wgpuDeviceCreateBuffer(device, &pressureModel);

    WGPUBufferDescriptor cfdUniforms = {};
    cfdUniforms.size = sizeof(CFDUniforms);
    cfdUniforms.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    cfdUniforms.mappedAtCreation = false;
    cfdUniforms.label = "CFD Uniform Buffer";
    uniformsCompute = wgpuDeviceCreateBuffer(device, &cfdUniforms);

    CFDUniforms cfdU = {step, dt, rho, KinematicViscosity, velocityIntake};
    wgpuQueueWriteBuffer(queue, uniformsCompute, 0, &cfdU, sizeof(CFDUniforms));
}

void CFD::InitBindGroupLayout(WGPUDevice device, WGPUQueue queue) {
    std::vector<WGPUBindGroupLayoutEntry> entries(4);

    entries[0].binding = 0;
    entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[0].visibility = WGPUShaderStage_Compute;


    entries[1].binding = 1;
    entries[1].texture.sampleType = WGPUTextureSampleType_Uint;
    entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[1].visibility = WGPUShaderStage_Compute;


    entries[2].binding = 2;
    entries[2].storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
    entries[2].storageTexture.format = WGPUTextureFormat_RG32Float;
    entries[2].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
    entries[2].visibility = WGPUShaderStage_Compute;

    entries[3].binding = 3;
    entries[3].visibility = WGPUShaderStage_Compute;
    entries[3].buffer.type = WGPUBufferBindingType_Uniform;
    entries[3].buffer.minBindingSize = sizeof(CFDUniforms);

    WGPUBindGroupLayoutDescriptor bgLayoutDescriptor = {};
    bgLayoutDescriptor.entryCount = 4;
    bgLayoutDescriptor.entries = entries.data();
    wgpuBindGroupLayoutCompute1 = wgpuDeviceCreateBindGroupLayout(device, &bgLayoutDescriptor);

    entries = std::vector<WGPUBindGroupLayoutEntry>(5);

    entries[0].binding = 0;
    entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[0].visibility = WGPUShaderStage_Compute;

    entries[1].binding = 1;
    entries[1].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[1].visibility = WGPUShaderStage_Compute;

    entries[2].binding = 2;
    entries[2].texture.sampleType = WGPUTextureSampleType_Uint;
    entries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[2].visibility = WGPUShaderStage_Compute;

    entries[3].binding = 3;
    entries[3].storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
    entries[3].storageTexture.format = WGPUTextureFormat_R32Float;
    entries[3].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
    entries[3].visibility = WGPUShaderStage_Compute;

    entries[4].binding = 4;
    entries[4].visibility = WGPUShaderStage_Compute;
    entries[4].buffer.type = WGPUBufferBindingType_Uniform;
    entries[4].buffer.minBindingSize = sizeof(CFDUniforms);

    WGPUBindGroupLayoutDescriptor bgLayoutDescriptor2 = {};
    bgLayoutDescriptor2.entryCount = 5;
    bgLayoutDescriptor2.entries = entries.data();
    wgpuBindGroupLayoutCompute2 = wgpuDeviceCreateBindGroupLayout(device, &bgLayoutDescriptor2);

    entries = std::vector<WGPUBindGroupLayoutEntry>(5);

    entries[0].binding = 0;
    entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[0].visibility = WGPUShaderStage_Compute;

    entries[1].binding = 1;
    entries[1].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[1].visibility = WGPUShaderStage_Compute;

    entries[2].binding = 2;
    entries[2].texture.sampleType = WGPUTextureSampleType_Uint;
    entries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[2].visibility = WGPUShaderStage_Compute;

    entries[3].binding = 3;
    entries[3].storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
    entries[3].storageTexture.format = WGPUTextureFormat_RG32Float;
    entries[3].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
    entries[3].visibility = WGPUShaderStage_Compute;

    entries[4].binding = 4;
    entries[4].visibility = WGPUShaderStage_Compute;
    entries[4].buffer.type = WGPUBufferBindingType_Uniform;
    entries[4].buffer.minBindingSize = sizeof(CFDUniforms);

    WGPUBindGroupLayoutDescriptor bgLayoutDescriptor3 = {};
    bgLayoutDescriptor3.entryCount = 5;
    bgLayoutDescriptor3.entries = entries.data();
    wgpuBindGroupLayoutCompute3 = wgpuDeviceCreateBindGroupLayout(device, &bgLayoutDescriptor3);

    entries = std::vector<WGPUBindGroupLayoutEntry>(3);

    entries[0].binding = 0;
    entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[0].visibility = WGPUShaderStage_Compute;

    entries[1].binding = 1;
    entries[1].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[1].visibility = WGPUShaderStage_Compute;

    entries[2].binding = 2;
    entries[2].storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
    entries[2].storageTexture.format = WGPUTextureFormat_RGBA32Float;
    entries[2].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
    entries[2].visibility = WGPUShaderStage_Compute;

    WGPUBindGroupLayoutDescriptor bgLayoutDescriptor4 = {};
    bgLayoutDescriptor4.entryCount = 3;
    bgLayoutDescriptor4.entries = entries.data();
    wgpuBindGroupLayoutCompute4 = wgpuDeviceCreateBindGroupLayout(device, &bgLayoutDescriptor4);

    entries = std::vector<WGPUBindGroupLayoutEntry>(2);

    entries[0].binding = 0;
    entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[0].visibility = WGPUShaderStage_Compute;

    entries[1].binding = 1;
    entries[1].storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
    entries[1].storageTexture.format = WGPUTextureFormat_RGBA32Float;
    entries[1].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
    entries[1].visibility = WGPUShaderStage_Compute;

    WGPUBindGroupLayoutDescriptor bgLayoutDescriptor5 = {};
    bgLayoutDescriptor5.entryCount = 2;
    bgLayoutDescriptor5.entries = entries.data();
    wgpuBindGroupLayoutCompute5 = wgpuDeviceCreateBindGroupLayout(device, &bgLayoutDescriptor5);

    entries = std::vector<WGPUBindGroupLayoutEntry>(4);

    entries[0].binding = 0;
    entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[0].visibility = WGPUShaderStage_Compute;

    entries[1].binding = 1;
    entries[1].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[1].visibility = WGPUShaderStage_Compute;

    entries[2].binding = 2;
    entries[2].texture.sampleType = WGPUTextureSampleType_Uint;
    entries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[2].visibility = WGPUShaderStage_Compute;

    entries[3].binding = 3;
    entries[3].storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
    entries[3].storageTexture.format = WGPUTextureFormat_RGBA8Uint;
    entries[3].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
    entries[3].visibility = WGPUShaderStage_Compute;

    WGPUBindGroupLayoutDescriptor bgLayoutDescriptor6 = {};
    bgLayoutDescriptor6.entryCount = 4;
    bgLayoutDescriptor6.entries = entries.data();
    wgpuBindGroupLayoutCompute6 = wgpuDeviceCreateBindGroupLayout(device, &bgLayoutDescriptor6);
}

void CFD::InitComputePipelines(WGPUDevice device, WGPUQueue queue) {
    InitComputePipeline1(device, queue);
    InitComputePipeline2(device, queue);
    InitComputePipeline3(device, queue);
    InitComputePipeline4(device, queue);
    InitComputePipeline5(device, queue);
    InitComputePipeline6(device, queue);
}

void CFD::InitComputePipeline1(WGPUDevice device, WGPUQueue queue) {
    WGPUShaderModule computeShader1Module = createShader(device, "Compute shader velocities",
                                                         std::string("shaders/step1Compute.wgsl").c_str());

    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = {};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &wgpuBindGroupLayoutCompute1;
    auto piplineDesc = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUComputePipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor.compute.constantCount = 0;
    pipelineDescriptor.compute.constants = nullptr;
    pipelineDescriptor.compute.entryPoint = "computeNewVelocities";
    pipelineDescriptor.compute.module = computeShader1Module;
    pipelineDescriptor.label = "Compute pipeline velocities";
    pipelineDescriptor.layout = piplineDesc;
    wgpuComputePipelineCompute1 = wgpuDeviceCreateComputePipeline(device, &pipelineDescriptor);
}

void CFD::InitComputePipeline2(WGPUDevice device, WGPUQueue queue) {
    WGPUShaderModule computeShader2Module = createShader(device, "Compute shader pressure",
                                                         std::string("shaders/step2Compute.wgsl").c_str());
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = {};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &wgpuBindGroupLayoutCompute2;
    auto piplineDesc = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUComputePipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor.compute.constantCount = 0;
    pipelineDescriptor.compute.constants = nullptr;
    pipelineDescriptor.compute.entryPoint = "computeNewPressure";
    pipelineDescriptor.compute.module = computeShader2Module;
    pipelineDescriptor.label = "Compute pipeline pressure";
    pipelineDescriptor.layout = piplineDesc;
    wgpuComputePipelineCompute2 = wgpuDeviceCreateComputePipeline(device, &pipelineDescriptor);
}

void CFD::InitComputePipeline3(WGPUDevice device, WGPUQueue queue) {
    WGPUShaderModule computeShader3Module = createShader(device, "Compute shader final velocities",
                                                         std::string("shaders/step3Compute.wgsl").c_str());
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = {};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &wgpuBindGroupLayoutCompute3;
    auto piplineDesc = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUComputePipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor.compute.constantCount = 0;
    pipelineDescriptor.compute.constants = nullptr;
    pipelineDescriptor.compute.entryPoint = "computeFinalVelocites";
    pipelineDescriptor.compute.module = computeShader3Module;
    pipelineDescriptor.label = "Compute final velocities";
    pipelineDescriptor.layout = piplineDesc;
    wgpuComputePipelineCompute3 = wgpuDeviceCreateComputePipeline(device, &pipelineDescriptor);
}

void CFD::InitComputePipeline4(WGPUDevice device, WGPUQueue queue) {
    WGPUShaderModule computeShader4Module = createShader(device, "Compute prepare curl texture",
                                                         std::string("shaders/step4Compute.wgsl").c_str());
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = {};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &wgpuBindGroupLayoutCompute4;
    auto piplineDesc = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUComputePipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor.compute.constantCount = 0;
    pipelineDescriptor.compute.constants = nullptr;
    pipelineDescriptor.compute.entryPoint = "computeCurlP";
    pipelineDescriptor.compute.module = computeShader4Module;
    pipelineDescriptor.label = "Compute curl P";
    pipelineDescriptor.layout = piplineDesc;
    wgpuComputePipelineCompute4 = wgpuDeviceCreateComputePipeline(device, &pipelineDescriptor);
}

void CFD::InitComputePipeline5(WGPUDevice device, WGPUQueue queue) {
    WGPUShaderModule computeShader5Module = createShader(device, "Compute min max mip levels",
                                                         std::string("shaders/step5Compute.wgsl").c_str());
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = {};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &wgpuBindGroupLayoutCompute5;
    auto piplineDesc = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUComputePipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor.compute.constantCount = 0;
    pipelineDescriptor.compute.constants = nullptr;
    pipelineDescriptor.compute.entryPoint = "computeMinMax";
    pipelineDescriptor.compute.module = computeShader5Module;
    pipelineDescriptor.label = "Compute min max curl P";
    pipelineDescriptor.layout = piplineDesc;
    wgpuComputePipelineCompute5 = wgpuDeviceCreateComputePipeline(device, &pipelineDescriptor);
}

void CFD::InitComputePipeline6(WGPUDevice device, WGPUQueue queue) {
    WGPUShaderModule computeShader6Module = createShader(device, "Compute final color",
                                                         std::string("shaders/step6Compute.wgsl").c_str());
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = {};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &wgpuBindGroupLayoutCompute6;
    auto piplineDesc = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUComputePipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor.compute.constantCount = 0;
    pipelineDescriptor.compute.constants = nullptr;
    pipelineDescriptor.compute.entryPoint = "computeFinalColor";
    pipelineDescriptor.compute.module = computeShader6Module;
    pipelineDescriptor.label = "Compute final color";
    pipelineDescriptor.layout = piplineDesc;
    wgpuComputePipelineCompute6 = wgpuDeviceCreateComputePipeline(device, &pipelineDescriptor);
}

void CFD::InitTexture(WGPUDevice device, WGPUQueue queue) {
    WGPUExtent3D extent;
    extent.width = Nx;
    extent.height = Ny;
    extent.depthOrArrayLayers = 1;

    std::vector<WGPUTextureDescriptor> textureDesc(7);
    textureDesc[0].dimension = WGPUTextureDimension_2D;
    textureDesc[0].format = WGPUTextureFormat_RG32Float;
    textureDesc[0].size = extent;
    textureDesc[0].sampleCount = 1;
    textureDesc[0].viewFormatCount = 0;
    textureDesc[0].viewFormats = nullptr;
    textureDesc[0].label = "newVelocities";

    textureDesc[0].usage = (
        WGPUTextureUsage_TextureBinding |
        WGPUTextureUsage_StorageBinding |
        WGPUTextureUsage_CopyDst |
        WGPUTextureUsage_CopySrc
    );

    textureDesc[0].mipLevelCount = 1;

    textureDesc[1].dimension = WGPUTextureDimension_2D;
    textureDesc[1].format = WGPUTextureFormat_R32Uint;
    textureDesc[1].size = extent;
    textureDesc[1].sampleCount = 1;
    textureDesc[1].viewFormatCount = 0;
    textureDesc[1].viewFormats = nullptr;
    textureDesc[1].label = "Stencil mask";

    textureDesc[1].usage = (
        WGPUTextureUsage_TextureBinding |
        WGPUTextureUsage_CopyDst
    );

    textureDesc[1].mipLevelCount = 1;

    textureDesc[2].dimension = WGPUTextureDimension_2D;
    textureDesc[2].format = WGPUTextureFormat_RG32Float;
    textureDesc[2].size = extent;
    textureDesc[2].sampleCount = 1;
    textureDesc[2].viewFormatCount = 0;
    textureDesc[2].viewFormats = nullptr;
    textureDesc[2].label = "Output velocities";

    textureDesc[2].usage = (
        WGPUTextureUsage_TextureBinding | // to bind the texture in a shader
        WGPUTextureUsage_StorageBinding | // to write the texture in a shader
        WGPUTextureUsage_CopySrc // to save the output data
    );

    textureDesc[2].mipLevelCount = 1;

    textureDesc[3].dimension = WGPUTextureDimension_2D;
    textureDesc[3].format = WGPUTextureFormat_R32Float;
    textureDesc[3].size = extent;
    textureDesc[3].sampleCount = 1;
    textureDesc[3].viewFormatCount = 0;
    textureDesc[3].viewFormats = nullptr;
    textureDesc[3].label = "Input pressure";
    textureDesc[3].usage = (
        WGPUTextureUsage_TextureBinding |
        WGPUTextureUsage_StorageBinding |
        WGPUTextureUsage_CopyDst |
        WGPUTextureUsage_CopySrc
    );
    textureDesc[3].mipLevelCount = 1;

    textureDesc[4].dimension = WGPUTextureDimension_2D;
    textureDesc[4].format = WGPUTextureFormat_R32Float;
    textureDesc[4].size = extent;
    textureDesc[4].sampleCount = 1;
    textureDesc[4].viewFormatCount = 0;
    textureDesc[4].viewFormats = nullptr;
    textureDesc[4].label = "Output pressure";
    textureDesc[4].usage = (
        WGPUTextureUsage_TextureBinding | // to bind the texture in a shader
        WGPUTextureUsage_StorageBinding | // to write the texture in a shader
        WGPUTextureUsage_CopySrc // to save the output data
    );
    textureDesc[4].mipLevelCount = 1;

    textureDesc[5].dimension = WGPUTextureDimension_2D;
    textureDesc[5].format = WGPUTextureFormat_RGBA32Float;
    textureDesc[5].size = extent;
    textureDesc[5].sampleCount = 1;
    textureDesc[5].viewFormatCount = 0;
    textureDesc[5].viewFormats = nullptr;
    textureDesc[5].label = "texture curl P";
    textureDesc[5].usage = (
        WGPUTextureUsage_TextureBinding |
        WGPUTextureUsage_StorageBinding |
        WGPUTextureUsage_CopySrc |
        WGPUTextureUsage_CopyDst
    );
    textureDesc[5].mipLevelCount = getMaxMipLevelCount(extent);

    textureCurlPMipSizes.resize(textureDesc[5].mipLevelCount);
    textureCurlPMipSizes[0] = extent;

    textureDesc[6].dimension = WGPUTextureDimension_2D;
    textureDesc[6].format = WGPUTextureFormat_RGBA8Uint;
    textureDesc[6].size = extent;
    textureDesc[6].sampleCount = 1;
    textureDesc[6].viewFormatCount = 0;
    textureDesc[6].viewFormats = nullptr;
    textureDesc[6].label = "final color";
    textureDesc[6].usage = (
        WGPUTextureUsage_TextureBinding |
        WGPUTextureUsage_StorageBinding |
        WGPUTextureUsage_CopyDst
    );
    textureDesc[6].mipLevelCount = 1;

    textureInputVelocities = wgpuDeviceCreateTexture(device, &textureDesc[0]);
    stencilTexture = wgpuDeviceCreateTexture(device, &textureDesc[1]);
    textureOutputVelocities = wgpuDeviceCreateTexture(device, &textureDesc[2]);
    textureInputPressure = wgpuDeviceCreateTexture(device, &textureDesc[3]);
    textureOutputPressure = wgpuDeviceCreateTexture(device, &textureDesc[4]);
    textureCurlP = wgpuDeviceCreateTexture(device, &textureDesc[5]);
    textureFancyColors = wgpuDeviceCreateTexture(device, &textureDesc[6]);

    WGPUImageCopyTexture destination = {};
    destination.texture = textureInputVelocities;
    destination.origin = {0, 0, 0};
    destination.aspect = WGPUTextureAspect_All;
    destination.mipLevel = 0;

    WGPUTextureDataLayout source = {};
    source.offset = 0;
    source.bytesPerRow = 8 * Nx;
    source.rowsPerImage = Ny;

    wgpuQueueWriteTexture(queue, &destination, flatten(newVelocity).data(), 8 * Nx * Ny, &source, &extent);

    destination.texture = stencilTexture;
    source.bytesPerRow = 4 * Nx;

    wgpuQueueWriteTexture(queue, &destination, flatten(stencilMask).data(), 4 * Nx * Ny, &source, &extent);

    destination.texture = textureInputPressure;
    source.bytesPerRow = 4 * Nx;

    wgpuQueueWriteTexture(queue, &destination, flatten(newPressure).data(), 4 * Nx * Ny, &source, &extent);
}

void CFD::InitTextureViews(WGPUDevice device, WGPUQueue queue) {
    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.aspect = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.format = WGPUTextureFormat_RG32Float;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.label = "newVelocities";
    textureViewDesc.baseMipLevel = 0;

    textureViewInputVelocities = wgpuTextureCreateView(this->textureInputVelocities, &textureViewDesc);

    textureViewDesc.label = "newVelocitiesOutput";

    textureViewOutputVelocities = wgpuTextureCreateView(textureOutputVelocities, &textureViewDesc);

    textureViewDesc.label = "stencilMask";
    textureViewDesc.format = WGPUTextureFormat_R32Uint;

    stencilTextureView = wgpuTextureCreateView(stencilTexture, &textureViewDesc);

    textureViewDesc.label = "pressureInput";
    textureViewDesc.format = WGPUTextureFormat_R32Float;

    textureViewInputPressure = wgpuTextureCreateView(textureInputPressure, &textureViewDesc);

    textureViewDesc.label = "pressureOutput";

    textureViewOutputPressure = wgpuTextureCreateView(textureOutputPressure, &textureViewDesc);

    textureViewDesc.format = WGPUTextureFormat_RGBA32Float;

    for (int level = 0; level < textureCurlPMipSizes.size(); ++level) {
        textureViewDesc.label = "curlP Mip level" + level;
        textureViewDesc.baseMipLevel = level;

        textureViewCurlPMipViews.push_back(wgpuTextureCreateView(textureCurlP, &textureViewDesc));
        if (level > 0) {
            WGPUExtent3D previousSuze = textureCurlPMipSizes[level - 1];
            textureCurlPMipSizes[level] = {
                previousSuze.width / 2,
                previousSuze.height / 2,
                previousSuze.depthOrArrayLayers / 2
            };
        }
    }

    textureViewDesc.format = WGPUTextureFormat_RGBA8Uint;
    textureViewDesc.label = "final color";
    textureViewDesc.baseMipLevel = 0;

    textureViewFancyColor = wgpuTextureCreateView(textureFancyColors, &textureViewDesc);
}

void CFD::Render(WGPUDevice device, WGPUQueue queue, WGPURenderPassEncoder pass, WGPUBuffer uniforms) {
    // UpdateColors(device, queue);

    wgpuRenderPassEncoderSetBindGroup(pass, 1, wgpuBindGroupRender, 0, 0);

    Matrix4x4 Velocity = translate(T.x, T.y, T.z) *
                         roll(R.z) * pitch(R.y) * yaw(R.z) *
                         scale(S.x, S.y, S.z);

    auto modelMat = toModelMat(Velocity);

    wgpuQueueWriteBuffer(queue, this->Model, 0, &modelMat, sizeof(ModelMat));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, Positions, 0, points.size() * sizeof(Vector2f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 1, Colors, 0, colors.size() * sizeof(Vector4f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 2, zIndexBuffer, 0, zIndex.size() * sizeof(float));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 3, UV, 0, uv.size() * sizeof(Vector2f));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 4, this->Model, 0, sizeof(ModelMat));

    wgpuRenderPassEncoderSetIndexBuffer(pass, EBO, WGPUIndexFormat_Uint32, 0, triangles.size() * sizeof(int));

    wgpuRenderPassEncoderDrawIndexed(pass, triangles.size(), 1, 0, 0, 0);

    Matrix4x4 Pressure = translate(T.x, T.y - 1.05 * ly, T.z) *
                         roll(R.z) * pitch(R.y) * yaw(R.x) *
                         scale(S.x, S.y, S.z);

    modelMat = toModelMat(Pressure);

    wgpuQueueWriteBuffer(queue, this->PressureModel, 0, &modelMat, sizeof(ModelMat));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, Positions, 0, points.size() * sizeof(Vector2f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 1, PressureColor, 0, colors.size() * sizeof(Vector4f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 2, zIndexBuffer, 0, zIndex.size() * sizeof(float));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 3, UV, 0, uv.size() * sizeof(Vector2f));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 4, this->PressureModel, 0, sizeof(ModelMat));

    wgpuRenderPassEncoderSetIndexBuffer(pass, EBO, WGPUIndexFormat_Uint32, 0, triangles.size() * sizeof(int));

    wgpuRenderPassEncoderDrawIndexed(pass, triangles.size(), 1, 0, 0, 0);
}

void CFD::InitGrid() {
    // for border conditions, set stencil mask to zero
    for (int i = 0; i < Nx; i++) {
        // Bottom wall
        stencilMask[0][i] = 0;
        stencilMask[1][i] = 0;
        stencilMask[2][i] = 0;

        // Top wall
        stencilMask[Ny - 1][i] = 0;
        stencilMask[Ny - 2][i] = 0;
        stencilMask[Ny - 3][i] = 0;
    }

    for (int i = 0; i < Ny; i++) {
        // Left wall, inlet
        stencilMask[i][0] = 1;

        // Right wall, outlet
        stencilMask[i][Nx - 1] = 2;
    }

    // initialize velocity and pressure
    for (int i = 0; i < Ny; i++) {
        for (int j = 0; j < Nx; j++) {
            velocity[i][j] = Vector2f(0, 0);
            newVelocity[i][j] = Vector2f(0, 0);

            pressure[i][j] = 0;
            newPressure[i][j] = 0;

            //sphere in the middlle
            for (auto &sp: spheres) {
                if ((i - sp.y) * (i - sp.y) + (j - sp.x) * (j - sp.x) < sp.r * sp.r) {
                    stencilMask[i][j] = 0;
                }
            }

            // wall in the middle
            if (j > Nx / 4 * 3 && j < Nx / 4 * 3 + 15 && i < Ny - 20 && i > 20) {
                stencilMask[i][j] = 0;
            }
        }
    }

    SetVelocityBorderCondition(velocity);
    SetVelocityBorderCondition(newVelocity);

    SetPressureBorderCondition(pressure);
    SetPressureBorderCondition(newPressure);
}

float CFD::getDt() {
    return dt;
}

void CFD::DrawLine(WGPUQueue queue, float x0, float y0, float x1, float y1) {
    int x0i = x0 / lx * Nx;
    int y0i = y0 / ly * Ny;

    int x1i = x1 / lx * Nx;
    int y1i = y1 / ly * Ny;

    int dx = abs(x1i - x0i), sx = x0i < x1i ? 1 : -1;
    int dy = -abs(y1i - y0i), sy = y0i < y1i ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

    for (;;) {
        /* loop */
        int j = x0i;
        int i = y0i;

        if (i < Ny && i > 0 && j < Nx && j > 0) {
            stencilMask[i][j] = 0;
        }

        if (i - 1 < Ny && i - 1 > 0 && j < Nx && j > 0) {
            stencilMask[i - 1][j] = 0;
        }

        if (i < Ny && i > 0 && j + 1 < Nx && j + 1 > 0) {
            stencilMask[i][j + 1] = 0;
        }

        if (i + 1 < Ny && i + 1 > 0 && j < Nx && j > 0) {
            stencilMask[i + 1][j] = 0;
        }

        if (i < Ny && i > 0 && j - 1 < Nx && j - 1 > 0) {
            stencilMask[i][j - 1] = 0;
        }

        if (x0i == x1i && y0i == y1i) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0i += sx;
        } /* e_xy+e_x > 0 */
        if (e2 <= dx) {
            err += dx;
            y0i += sy;
        } /* e_xy+e_y < 0 */
    }

    WGPUExtent3D extent;
    extent.width = Nx;
    extent.height = Ny;
    extent.depthOrArrayLayers = 1;

    WGPUImageCopyTexture destination = {};
    destination.texture = stencilTexture;
    destination.origin = {0, 0, 0};
    destination.aspect = WGPUTextureAspect_All;
    destination.mipLevel = 0;

    WGPUTextureDataLayout source = {};
    source.offset = 0;
    source.bytesPerRow = 4 * Nx;
    source.rowsPerImage = Ny;

    wgpuQueueWriteTexture(queue, &destination, flatten(stencilMask).data(), 4 * Nx * Ny, &source, &extent);

    wgpuTextureViewRelease(stencilTextureView);

    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.aspect = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.label = "stencilMask";
    textureViewDesc.format = WGPUTextureFormat_R32Uint;

    stencilTextureView = wgpuTextureCreateView(stencilTexture, &textureViewDesc);
}

void CFD::UpdateGrid(WGPUDevice device, WGPUQueue queue, int frames) {
    WGPUCommandEncoderDescriptor encoderDesc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    WGPUComputePassDescriptor computePassDesc = {};
    computePassDesc.timestampWrites = nullptr;
    computePassDesc.label = "Compute";

    uint32_t invocationCountX = Nx;
    uint32_t invocationCountY = Ny;
    uint32_t workgroupSizePerDim = 8;
    // This ceils invocationCountX / workgroupSizePerDim
    uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
    uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;

    WGPUComputePassEncoder computePass = wgpuCommandEncoderBeginComputePass(encoder, &computePassDesc);

    for (int i = 0; i < frames; i++) {
        InitBindGroup1(device, queue);
        wgpuComputePassEncoderSetPipeline(computePass, wgpuComputePipelineCompute1);
        wgpuComputePassEncoderSetBindGroup(computePass, 0, wgpuBindGroupCompute1, 0, nullptr);

        wgpuComputePassEncoderDispatchWorkgroups(computePass, workgroupCountX, workgroupCountY, 1);
        wgpuBindGroupRelease(this->wgpuBindGroupCompute1);

        auto swapP = false;
        for (int i = 0; i < 20; i++) {
            InitBindGroup2(device, queue, swapP);
            wgpuComputePassEncoderSetPipeline(computePass, wgpuComputePipelineCompute2);
            wgpuComputePassEncoderSetBindGroup(computePass, 0, wgpuBindGroupCompute2, 0, nullptr);

            wgpuComputePassEncoderDispatchWorkgroups(computePass, workgroupCountX, workgroupCountY, 1);

            wgpuBindGroupRelease(this->wgpuBindGroupCompute2);

            swapP = !swapP;
        }

        WGPUTextureView inputPressure = textureViewOutputPressure;
        if (swapP) {
            inputPressure = textureViewInputPressure;
        }

        InitBindGroup3(device, queue, inputPressure);
        wgpuComputePassEncoderSetPipeline(computePass, wgpuComputePipelineCompute3);
        wgpuComputePassEncoderSetBindGroup(computePass, 0, wgpuBindGroupCompute3, 0, nullptr);

        wgpuComputePassEncoderDispatchWorkgroups(computePass, workgroupCountX, workgroupCountY, 1);
        wgpuBindGroupRelease(this->wgpuBindGroupCompute3);
    }

    InitBindGroup4(device, queue);
    wgpuComputePassEncoderSetPipeline(computePass, wgpuComputePipelineCompute4);
    wgpuComputePassEncoderSetBindGroup(computePass, 0, wgpuBindGroupCompute4, 0, nullptr);

    wgpuComputePassEncoderDispatchWorkgroups(computePass, workgroupCountX, workgroupCountY, 1);
    wgpuBindGroupRelease(this->wgpuBindGroupCompute4);

    wgpuComputePassEncoderSetPipeline(computePass, wgpuComputePipelineCompute5);
    for (int level = 1; level < textureCurlPMipSizes.size(); level++) {
        InitBindGroup5(device, queue, level);
        wgpuComputePassEncoderSetBindGroup(computePass, 0, wgpuBindGroupCompute5, 0, nullptr);

        wgpuComputePassEncoderDispatchWorkgroups(computePass, workgroupCountX, workgroupCountY, 1);
        wgpuBindGroupRelease(this->wgpuBindGroupCompute5);
    }

    InitBindGroup6(device, queue);
    wgpuComputePassEncoderSetPipeline(computePass, wgpuComputePipelineCompute6);
    wgpuComputePassEncoderSetBindGroup(computePass, 0, wgpuBindGroupCompute6, 0, nullptr);

    wgpuComputePassEncoderDispatchWorkgroups(computePass, workgroupCountX, workgroupCountY, 1);
    wgpuBindGroupRelease(this->wgpuBindGroupCompute6);

    wgpuComputePassEncoderEnd(computePass);
    wgpuComputePassEncoderRelease(computePass); // release pass

    WGPUCommandBufferDescriptor commandDesc = {};
    commandDesc.nextInChain = nullptr;
    commandDesc.label = "Command buffer";

    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, &commandDesc); // create commands
    wgpuCommandEncoderRelease(encoder); // release encoder

    wgpuQueueSubmit(queue, 1, &commands);

    wgpuCommandBufferRelease(commands); // release commands
}

void CFD::InitBindGroup1(WGPUDevice device, WGPUQueue queue) {
    static std::vector<WGPUBindGroupEntry> entries(4);

    entries[0].binding = 0;
    entries[0].textureView = this->textureViewInputVelocities;

    entries[1].binding = 1;
    entries[1].textureView = this->stencilTextureView;

    entries[2].binding = 2;
    entries[2].textureView = this->textureViewOutputVelocities;

    entries[3].binding = 3;
    entries[3].buffer = this->uniformsCompute;
    entries[3].offset = 0;
    entries[3].size = sizeof(CFDUniforms);

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.label = "BindGroup1";
    bgDesc.layout = wgpuBindGroupLayoutCompute1;
    bgDesc.entryCount = 4;
    bgDesc.entries = entries.data();
    wgpuBindGroupCompute1 = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void CFD::InitBindGroup2(WGPUDevice device, WGPUQueue queue, bool swapP) {
    auto input = textureViewInputPressure;
    auto output = textureViewOutputPressure;

    if (swapP) {
        input = textureViewOutputPressure;
        output = textureViewInputPressure;
    }

    static std::vector<WGPUBindGroupEntry> entries(5);

    entries[0].binding = 0;
    entries[0].textureView = input;

    entries[1].binding = 1;
    entries[1].textureView = this->textureViewOutputVelocities;

    entries[2].binding = 2;
    entries[2].textureView = this->stencilTextureView;

    entries[3].binding = 3;
    entries[3].textureView = output;

    entries[4].binding = 4;
    entries[4].buffer = this->uniformsCompute;
    entries[4].offset = 0;
    entries[4].size = sizeof(CFDUniforms);

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.label = "BindGroup2";
    bgDesc.layout = wgpuBindGroupLayoutCompute2;
    bgDesc.entryCount = 5;
    bgDesc.entries = entries.data();
    wgpuBindGroupCompute2 = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void CFD::InitBindGroup3(WGPUDevice device, WGPUQueue queue, WGPUTextureView inputPressure) {
    static std::vector<WGPUBindGroupEntry> entries(5);

    entries[0].binding = 0;
    entries[0].textureView = inputPressure;

    entries[1].binding = 1;
    entries[1].textureView = this->textureViewOutputVelocities;

    entries[2].binding = 2;
    entries[2].textureView = this->stencilTextureView;

    entries[3].binding = 3;
    entries[3].textureView = textureViewInputVelocities;

    entries[4].binding = 4;
    entries[4].buffer = this->uniformsCompute;
    entries[4].offset = 0;
    entries[4].size = sizeof(CFDUniforms);

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.label = "BindGroup3";
    bgDesc.layout = wgpuBindGroupLayoutCompute3;
    bgDesc.entryCount = 5;
    bgDesc.entries = entries.data();
    wgpuBindGroupCompute3 = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void CFD::InitBindGroup4(WGPUDevice device, WGPUQueue queue) {
    static std::vector<WGPUBindGroupEntry> entries(3);

    entries[0].binding = 0;
    entries[0].textureView = textureViewInputVelocities;

    entries[1].binding = 1;
    entries[1].textureView = textureViewInputPressure;

    entries[2].binding = 2;
    entries[2].textureView = textureViewCurlPMipViews[0];

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.label = "BindGroup4";
    bgDesc.layout = wgpuBindGroupLayoutCompute4;
    bgDesc.entryCount = 3;
    bgDesc.entries = entries.data();
    wgpuBindGroupCompute4 = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void CFD::InitBindGroup5(WGPUDevice device, WGPUQueue queue, int nextMipLevel) {
    static std::vector<WGPUBindGroupEntry> entries(2);

    entries[0].binding = 0;
    entries[0].textureView = textureViewCurlPMipViews[nextMipLevel - 1];

    entries[1].binding = 1;
    entries[1].textureView = textureViewCurlPMipViews[nextMipLevel];

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.label = "BindGroup5";
    bgDesc.layout = wgpuBindGroupLayoutCompute5;
    bgDesc.entryCount = 2;
    bgDesc.entries = entries.data();
    wgpuBindGroupCompute5 = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void CFD::InitBindGroup6(WGPUDevice device, WGPUQueue queue) {
    static std::vector<WGPUBindGroupEntry> entries(4);

    entries[0].binding = 0;
    entries[0].textureView = textureViewCurlPMipViews[0];

    entries[1].binding = 1;
    entries[1].textureView = textureViewCurlPMipViews[textureCurlPMipSizes.size() - 1];

    entries[2].binding = 2;
    entries[2].textureView = stencilTextureView;

    entries[3].binding = 3;
    entries[3].textureView = textureViewFancyColor;

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.label = "BindGroup6";
    bgDesc.layout = wgpuBindGroupLayoutCompute6;
    bgDesc.entryCount = 4;
    bgDesc.entries = entries.data();
    wgpuBindGroupCompute6 = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void CFD::InitRenderBindGroup(WGPUDevice device, WGPUQueue, WGPUBindGroupLayout meshDependentLayout) {
    WGPUBindGroupEntry entry = {};

    entry.binding = 0;
    entry.textureView = textureViewFancyColor;

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.label = "RenderBindGroup";
    bgDesc.layout = meshDependentLayout;
    bgDesc.entryCount = 1;
    bgDesc.entries = &entry;
    wgpuBindGroupRender = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void CFD::SetVelocityBorderCondition(std::vector<std::vector<Vector2f> > &velocity_ptr) {
    // border conditions, velocity
    for (int i = 0; i < Nx; i++) {
        // Bottom wall
        velocity_ptr[0][i] = Vector2f(0, 0);
        velocity_ptr[1][i] = Vector2f(0, 0);
        velocity_ptr[2][i] = Vector2f(0, 0);

        // Top wall
        velocity_ptr[Ny - 1][i] = Vector2f(0, 0);
        velocity_ptr[Ny - 2][i] = Vector2f(0, 0);
        velocity_ptr[Ny - 3][i] = Vector2f(0, 0);
    }

    for (int i = 0; i < Ny; i++) {
        // Left wall
        velocity_ptr[i][0] = Vector2f(velocityIntake, 0);

        // Right wall
        velocity_ptr[i][Nx - 1] = Vector2f(0, 0);
    }
}

void CFD::SetPressureBorderCondition(std::vector<std::vector<float> > &pressure_ptr) {
    for (int i = 0; i < Nx; i++) {
        // Bottom wall, Neumann condition
        pressure_ptr[0][i] = 0;
        pressure_ptr[1][i] = 0;
        pressure_ptr[2][i] = pressure_ptr[3][i];

        // Top wall, Neumann condition
        pressure_ptr[Ny - 1][i] = 0;
        pressure_ptr[Ny - 2][i] = 0;
        pressure_ptr[Ny - 3][i] = pressure_ptr[Ny - 4][i];
    }

    for (int i = 0; i < Ny; i++) {
        // Left wall, velocity inlet
        pressure_ptr[i][0] = pressure_ptr[i][1];

        // Right wall, outlet
        pressure_ptr[i][Nx - 1] = 0;
    }
}
