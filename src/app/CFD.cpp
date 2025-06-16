//
// Created by aidar on 24.05.25.
//

#include "CFD.h"

#include <webgpu/webgpu.h>

#include "webgpu/wgpu.h"

CFD::CFD(float step, WGPUDevice device, WGPUQueue queue) {
    this->step = step;

    float CFL = 0.9; //the Courant number is always equal to 0.9

    lx = 25.6;
    ly = 12.8;
    dt = CFL * step * step / 2; // compute dt based on Courant number

    Nx = lx / step;
    Ny = ly / step;

    spheres = {
        sphere{
            .x = static_cast<int>((6.0 / lx) * Nx),
            .y = static_cast<int>((5.0 / ly) * Ny),
            .r = static_cast<int>((1.0 / lx) * Nx)
        },
        sphere{
            .x = static_cast<int>((10.0 / lx) * Nx),
            .y = static_cast<int>((7.0 / ly) * Ny),
            .r = static_cast<int>((1.0 / lx) * Nx)
        },
        sphere{
            .x = static_cast<int>((10.0 / lx) * Nx),
            .y = static_cast<int>((2.0 / ly) * Ny),
            .r = static_cast<int>((1.0 / lx) * Nx)
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
            float x = j * step;

            points.push_back(Vector2f(x, y));
            points.push_back(Vector2f(x, y + step));
            points.push_back(Vector2f(x + step, y + step));
            points.push_back(Vector2f(x + step, y));

            triangles.push_back(4 * (i * Nx + j));
            triangles.push_back(4 * (i * Nx + j) + 1);
            triangles.push_back(4 * (i * Nx + j) + 2);
            triangles.push_back(4 * (i * Nx + j));
            triangles.push_back(4 * (i * Nx + j) + 2);
            triangles.push_back(4 * (i * Nx + j) + 3);


            velocity[i].push_back(Vector2f(0, 0));
            pressure[i].push_back(0);

            newVelocity[i].push_back(Vector2f(0, 0));
            newPressure[i].push_back(0);

            stencilMask[i].push_back(255);
        }
    }

    colors = std::vector<Vector4f>(points.size(), {1, 1, 1, 1});
    pickingColor = std::vector<Vector4f>(points.size(), {1, 1, 1, 1});
    zIndex = std::vector<float>(points.size(), -0.5);

    InitGrid();

    LoadWGPUBuffers(device, queue);

    InitBuffers(device, queue);
    InitBindGroupLayout(device, queue);
    InitComputePipelines(device, queue);
    InitTexture(device, queue);
    InitTextureViews(device, queue);
}

void CFD::InitBuffers(WGPUDevice device, WGPUQueue queue) {
    WGPUBufferDescriptor bufferDescriptor = {};
    bufferDescriptor.size = sizeof(float) * triangles.size();
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bufferDescriptor.label = {"EBO",WGPU_STRLEN};
    bufferDescriptor.mappedAtCreation = false;
    EBO = wgpuDeviceCreateBuffer(device, &bufferDescriptor);

    wgpuQueueWriteBuffer(queue, EBO, 0, triangles.data(), sizeof(int) * triangles.size());

    WGPUBufferDescriptor pressureBuffer = {};
    pressureBuffer.size = colors.size() * sizeof(Vector4f);
    pressureBuffer.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    pressureBuffer.label = {"PressureColor",WGPU_STRLEN};
    pressureBuffer.mappedAtCreation = false;
    PressureColor = wgpuDeviceCreateBuffer(device, &pressureBuffer);

    wgpuQueueWriteBuffer(queue, PressureColor, 0, colors.data(), colors.size() * sizeof(Vector4f));

    WGPUBufferDescriptor pressureModel = {};
    pressureModel.size = sizeof(ModelMat);
    pressureModel.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    pressureModel.label = {"PressureModel",WGPU_STRLEN};
    pressureModel.mappedAtCreation = false;
    PressureModel = wgpuDeviceCreateBuffer(device, &pressureModel);

    WGPUBufferDescriptor cfdUniforms = {};
    cfdUniforms.size = sizeof(CFDUniforms);
    cfdUniforms.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    cfdUniforms.mappedAtCreation = false;
    cfdUniforms.label = {"CFD Uniform Buffer",WGPU_STRLEN};
    uniformsCompute = wgpuDeviceCreateBuffer(device, &cfdUniforms);

    CFDUniforms cfdU = {step, dt, rho, KinematicViscosity};
    wgpuQueueWriteBuffer(queue, uniformsCompute, 0, &cfdU, sizeof(CFDUniforms));

    WGPUBufferDescriptor VelocitiesStagingBufferDesc = {};
    VelocitiesStagingBufferDesc.label = {"staging_buffer_velocities",WGPU_STRLEN};
    VelocitiesStagingBufferDesc.size = 8 * Nx * Ny;
    VelocitiesStagingBufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    VelocitiesStagingBufferDesc.mappedAtCreation = false;
    VelocitiesStagingBuffer = wgpuDeviceCreateBuffer(device, &VelocitiesStagingBufferDesc);

    WGPUBufferDescriptor PressureStagingBufferDesc = {};
    PressureStagingBufferDesc.label = {"staging_buffer_pressure",WGPU_STRLEN};
    PressureStagingBufferDesc.size = 4 * Nx * Ny;
    PressureStagingBufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    PressureStagingBufferDesc.mappedAtCreation = false;
    PressureStagingBuffer = wgpuDeviceCreateBuffer(device, &PressureStagingBufferDesc);
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
}

void CFD::InitComputePipelines(WGPUDevice device, WGPUQueue queue) {
    InitComputePipeline1(device, queue);
    InitComputePipeline2(device, queue);
    InitComputePipeline3(device, queue);
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
    pipelineDescriptor.compute.entryPoint = {"computeNewVelocities", WGPU_STRLEN};
    pipelineDescriptor.compute.module = computeShader1Module;
    pipelineDescriptor.label = {"Compute pipeline velocities",WGPU_STRLEN};
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
    pipelineDescriptor.compute.entryPoint = {"computeNewPressure", WGPU_STRLEN};
    pipelineDescriptor.compute.module = computeShader2Module;
    pipelineDescriptor.label = {"Compute pipeline pressure",WGPU_STRLEN};
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
    pipelineDescriptor.compute.entryPoint = {"computeFinalVelocites", WGPU_STRLEN};
    pipelineDescriptor.compute.module = computeShader3Module;
    pipelineDescriptor.label = {"Compute final velocities",WGPU_STRLEN};
    pipelineDescriptor.layout = piplineDesc;
    wgpuComputePipelineCompute3 = wgpuDeviceCreateComputePipeline(device, &pipelineDescriptor);
}


void CFD::InitTexture(WGPUDevice device, WGPUQueue queue) {
    WGPUExtent3D extent;
    extent.width = Nx;
    extent.height = Ny;
    extent.depthOrArrayLayers = 1;

    std::vector<WGPUTextureDescriptor> textureDesc(5);
    textureDesc[0].dimension = WGPUTextureDimension_2D;
    textureDesc[0].format = WGPUTextureFormat_RG32Float;
    textureDesc[0].size = extent;
    textureDesc[0].sampleCount = 1;
    textureDesc[0].viewFormatCount = 0;
    textureDesc[0].viewFormats = nullptr;
    textureDesc[0].label = {"newVelocities", WGPU_STRLEN};

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
    textureDesc[1].label = {"Stencil mask", WGPU_STRLEN};

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
    textureDesc[2].label = {"Output velocities", WGPU_STRLEN};

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
    textureDesc[3].label = {"Input pressure", WGPU_STRLEN};
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
    textureDesc[4].label = {"Output pressure", WGPU_STRLEN};
    textureDesc[4].usage = (
        WGPUTextureUsage_TextureBinding | // to bind the texture in a shader
        WGPUTextureUsage_StorageBinding | // to write the texture in a shader
        WGPUTextureUsage_CopySrc // to save the output data
    );
    textureDesc[4].mipLevelCount = 1;

    textureInputVelocities = wgpuDeviceCreateTexture(device, &textureDesc[0]);
    stencilTexture = wgpuDeviceCreateTexture(device, &textureDesc[1]);
    textureOutputVelocities = wgpuDeviceCreateTexture(device, &textureDesc[2]);
    textureInputPressure = wgpuDeviceCreateTexture(device, &textureDesc[3]);
    textureOutputPressure = wgpuDeviceCreateTexture(device, &textureDesc[4]);

    WGPUTexelCopyTextureInfo destination = {};
    destination.texture = textureInputVelocities;
    destination.origin = {0, 0, 0};
    destination.aspect = WGPUTextureAspect_All;
    destination.mipLevel = 0;

    WGPUTexelCopyBufferLayout source = {};
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
    textureViewDesc.label = {"newVelocities",WGPU_STRLEN};
    textureViewDesc.baseMipLevel = 0;

    textureViewInputVelocities = wgpuTextureCreateView(this->textureInputVelocities, &textureViewDesc);

    textureViewDesc.label = {"newVelocitiesOutput",WGPU_STRLEN};

    textureViewOutputVelocities = wgpuTextureCreateView(textureOutputVelocities, &textureViewDesc);

    textureViewDesc.label = {"stencilMask",WGPU_STRLEN};
    textureViewDesc.format = WGPUTextureFormat_R32Uint;

    stencilTextureView = wgpuTextureCreateView(stencilTexture, &textureViewDesc);

    textureViewDesc.label = {"pressureInput",WGPU_STRLEN};
    textureViewDesc.format = WGPUTextureFormat_R32Float;

    textureViewInputPressure = wgpuTextureCreateView(textureInputPressure, &textureViewDesc);

    textureViewDesc.label = {"pressureOutput",WGPU_STRLEN};

    textureViewOutputPressure = wgpuTextureCreateView(textureOutputPressure, &textureViewDesc);
}

void CFD::Render(WGPUDevice device, WGPUQueue queue, WGPURenderPassEncoder pass, WGPUBuffer uniforms) {
    UpdateColors(device, queue);

    Matrix4x4 Velocity = translate(T.x - lx / 2, T.y - 0.2 * ly, T.z) *
                         roll(R.z) * pitch(R.y) * yaw(R.z) *
                         scale(S.x, S.y, S.z);

    auto modelMat = toModelMat(Velocity);

    wgpuQueueWriteBuffer(queue, this->Model, 0, &modelMat, sizeof(ModelMat));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, Positions, 0, points.size() * sizeof(Vector2f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 1, Colors, 0, colors.size() * sizeof(Vector4f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 2, zIndexBuffer, 0, zIndex.size() * sizeof(float));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 3, this->Model, 0, sizeof(ModelMat));

    wgpuRenderPassEncoderSetIndexBuffer(pass, EBO, WGPUIndexFormat_Uint32, 0, triangles.size() * sizeof(int));

    wgpuRenderPassEncoderDrawIndexed(pass, triangles.size(), 1, 0, 0, 0);

    Matrix4x4 Pressure = translate(T.x - lx / 2, T.y - 1.05 * ly - 0.2 * ly, T.z) *
                         roll(R.z) * pitch(R.y) * yaw(R.x) *
                         scale(S.x, S.y, S.z);

    modelMat = toModelMat(Pressure);

    wgpuQueueWriteBuffer(queue, this->PressureModel, 0, &modelMat, sizeof(ModelMat));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, Positions, 0, points.size() * sizeof(Vector2f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 1, PressureColor, 0, colors.size() * sizeof(Vector4f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 2, zIndexBuffer, 0, zIndex.size() * sizeof(float));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 3, this->PressureModel, 0, sizeof(ModelMat));

    wgpuRenderPassEncoderSetIndexBuffer(pass, EBO, WGPUIndexFormat_Uint32, 0, triangles.size() * sizeof(int));

    wgpuRenderPassEncoderDrawIndexed(pass, triangles.size(), 1, 0, 0, 0);
}

void CFD::InitGrid() {
    // for border conditions, set stencil mask to zero
    for (int i = 0; i < Nx; i++) {
        // Bottom wall
        stencilMask[0][i] = 0;

        // Top wall
        stencilMask[Ny - 1][i] = 0;
    }

    for (int i = 0; i < Ny; i++) {
        // Left wall
        stencilMask[i][0] = 0;

        // Right wall
        stencilMask[i][Nx - 1] = 1;
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

void CFD::UpdateGrid(WGPUDevice device, WGPUQueue queue) {
    WGPUCommandEncoderDescriptor encoderDesc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    WGPUComputePassDescriptor computePassDesc = {};
    computePassDesc.timestampWrites = nullptr;
    computePassDesc.label = {"Compute",WGPU_STRLEN};

    uint32_t invocationCountX = Nx;
    uint32_t invocationCountY = Ny;
    uint32_t workgroupSizePerDim = 8;
    // This ceils invocationCountX / workgroupSizePerDim
    uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
    uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;

    WGPUComputePassEncoder computePass = wgpuCommandEncoderBeginComputePass(encoder, &computePassDesc);

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

    wgpuComputePassEncoderEnd(computePass);
    wgpuComputePassEncoderRelease(computePass); // release pass

    // Encode texture-to-buffer copy
    WGPUTexelCopyTextureInfo srcV = {};
    srcV.texture = textureOutputVelocities;
    srcV.mipLevel = 0;
    srcV.origin = {0, 0, 0};
    srcV.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferInfo dstV = {};
    dstV.buffer = VelocitiesStagingBuffer;
    dstV.layout.offset = 0;
    dstV.layout.bytesPerRow = Nx * 8; // 8 bytes per texel
    dstV.layout.rowsPerImage = Ny;

    WGPUExtent3D copy_size = {static_cast<uint32_t>(Nx), static_cast<uint32_t>(Ny), 1};

    wgpuCommandEncoderCopyTextureToBuffer(encoder, &srcV, &dstV, &copy_size);

    // Encode texture-to-buffer copy
    WGPUTexelCopyTextureInfo srcP = {};
    srcP.texture = textureOutputPressure;
    srcP.mipLevel = 0;
    srcP.origin = {0, 0, 0};
    srcP.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferInfo dstP = {};
    dstP.buffer = PressureStagingBuffer;
    dstP.layout.offset = 0;
    dstP.layout.bytesPerRow = Nx * 4; // 4 bytes per texel
    dstP.layout.rowsPerImage = Ny;

    wgpuCommandEncoderCopyTextureToBuffer(encoder, &srcP, &dstP, &copy_size);

    WGPUCommandBufferDescriptor commandDesc = {};
    commandDesc.nextInChain = nullptr;
    commandDesc.label = {"Command buffer",WGPU_STRLEN};

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
    bgDesc.label = {"BindGroup1",WGPU_STRLEN};
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
    bgDesc.label = {"BindGroup2",WGPU_STRLEN};
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
    bgDesc.label = {"BindGroup3",WGPU_STRLEN};
    bgDesc.layout = wgpuBindGroupLayoutCompute3;
    bgDesc.entryCount = 5;
    bgDesc.entries = entries.data();
    wgpuBindGroupCompute3 = wgpuDeviceCreateBindGroup(device, &bgDesc);
}

void CFD::SetVelocityBorderCondition(std::vector<std::vector<Vector2f> > &velocity_ptr) {
    // border conditions, velocity
    for (int i = 0; i < Nx; i++) {
        // Bottom wall
        velocity_ptr[0][i] = Vector2f(0, 0);

        // Top wall
        velocity_ptr[Ny - 1][i] = Vector2f(0, 0);
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
        pressure_ptr[0][i] = pressure_ptr[1][i];

        // Top wall, Neumann condition
        pressure_ptr[Ny - 1][i] = pressure_ptr[Ny - 2][i];
    }

    for (int i = 0; i < Ny; i++) {
        // Left wall, velocity inlet
        pressure_ptr[i][0] = pressure_ptr[i][1];

        // Right wall, outlet
        pressure_ptr[i][Nx - 1] = 0;
    }
}

void CFD::UpdateColors(WGPUDevice device, WGPUQueue queue) {
    bool isMapped = false;
    wgpuBufferMapAsync(VelocitiesStagingBuffer, WGPUMapMode_Read, 0, 8 * Nx * Ny, {
                           .nextInChain = nullptr,
                           .mode = WGPUCallbackMode_AllowProcessEvents,
                           .callback = [](WGPUMapAsyncStatus status, WGPUStringView message,
                                          WGPU_NULLABLE void *userdata1, WGPU_NULLABLE void *userdata2) {
                               if (status != WGPUMapAsyncStatus_Success) {
                                   std::cout << "failed to map array\n" << message.data << std::endl;
                               }

                               bool *ptr = reinterpret_cast<bool *>(userdata1);
                               (*ptr) = true;
                           },
                           .userdata1 = (void *) (&isMapped),
                       });
    while (!isMapped) {
        wgpuQueueSubmit(queue, 0, nullptr);
    }

    isMapped = false;

    wgpuBufferMapAsync(PressureStagingBuffer, WGPUMapMode_Read, 0, 4 * Nx * Ny, {
                           .nextInChain = nullptr,
                           .mode = WGPUCallbackMode_AllowProcessEvents,
                           .callback = [](WGPUMapAsyncStatus status, WGPUStringView message,
                                          WGPU_NULLABLE void *userdata1, WGPU_NULLABLE void *userdata2) {
                               if (status != WGPUMapAsyncStatus_Success) {
                                   std::cout << "failed to map array\n" << message.data << std::endl;
                               }

                               bool *ptr = reinterpret_cast<bool *>(userdata1);
                               (*ptr) = true;
                           },
                           .userdata1 = (void *) (&isMapped),
                       });
    while (!isMapped) {
        wgpuQueueSubmit(queue, 0, nullptr);
    }

    isMapped = false;

    const void *dataV = wgpuBufferGetMappedRange(VelocitiesStagingBuffer, 0, 8 * Nx * Ny);
    const void *dataP = wgpuBufferGetMappedRange(PressureStagingBuffer, 0, 4 * Nx * Ny);

    for (int i = 0; i < Ny; i++) {
        memcpy(velocity[i].data(), (Vector2f *) dataV + i * Nx, 8 * Nx);
        memcpy(pressure[i].data(), (float *) dataP + i * Nx, 4 * Nx);
    }

    wgpuBufferUnmap(VelocitiesStagingBuffer);
    wgpuBufferUnmap(PressureStagingBuffer);

    float maxV = -1e9;
    float minV = 1e9;
    for (int i = 1; i < Ny - 1; i++) {
        for (int j = 1; j < Nx - 1; j++) {
            // technically I should divide by dxdy, but it's simpler
            float curl = (velocity[i][j + 1].y - velocity[i][j - 1].y) - (
                             velocity[i + 1][j].x - velocity[i - 1][j].x);
            float absV = std::abs(sqrt(velocity[i][j].x * velocity[i][j].x + velocity[i][j].y * velocity[i][j].y));

            maxV = std::max(maxV, curl);
            minV = std::min(minV, curl);
        }
    }


    for (int i = 1; i < Ny - 1; i++) {
        for (int j = 1; j < Nx - 1; j++) {
            float curl = (velocity[i][j + 1].y - velocity[i][j - 1].y) - (
                             velocity[i + 1][j].x - velocity[i - 1][j].x);

            float absV = std::abs(sqrt(velocity[i][j].x * velocity[i][j].x + velocity[i][j].y * velocity[i][j].y));

            auto t = (curl - minV) / (maxV - minV);

            float startR = 1.0;
            float startG = 1.0;
            float startB = 1.0;

            float endR = 0.0;
            float endG = 0.0;
            float endB = 1.0;

            if (t <= -minV / (maxV - minV)) {
                t /= -minV / (maxV - minV);

                t *= t; // slower at start

                startR = 1.0;
                startG = 0.0;
                startB = 0.0;

                endR = 1.0;
                endG = 1.0;
                endB = 1.0;
            } else {
                t -= -minV / (maxV - minV);
                t /= -minV / (maxV - minV);

                t = (3.0 * t * t) - (2.0 * t * t * t);
            }


            float R = startR + t * (endR - startR);
            float G = startG + t * (endG - startG);
            float B = startB + t * (endB - startB);

            int pIndex = 4 * (i * Nx + j);

            colors[pIndex] = Vector4f(R, G, B, 1);
            colors[pIndex + 1] = Vector4f(R, G, B, 1);
            colors[pIndex + 2] = Vector4f(R, G, B, 1);
            colors[pIndex + 3] = Vector4f(R, G, B, 1);
        }
    }

    wgpuQueueWriteBuffer(queue, Colors, 0, colors.data(), colors.size() * sizeof(Vector4f));

    float maxP = 0;
    for (int i = 0; i < Ny; i++) {
        for (int j = 0; j < Nx; j++) {
            maxP = std::max(maxP, abs(pressure[i][j]));
        }
    }

    if (maxP == 0) {
        maxP = 1;
    }

    for (int i = 0; i < Ny; i++) {
        for (int j = 0; j < Nx; j++) {
            int pIndex = 4 * (i * Nx + j);

            colors[pIndex] = Vector4f(abs(pressure[i][j]) / maxP, 0,
                                      1 - abs(pressure[i][j]) / maxP, 1);
            colors[pIndex + 1] = Vector4f(abs(pressure[i][j]) / maxP, 0,
                                          1 - abs(pressure[i][j]) / maxP, 1);
            colors[pIndex + 2] = Vector4f(abs(pressure[i][j]) / maxP, 0,
                                          1 - abs(pressure[i][j]) / maxP, 1);
            colors[pIndex + 3] = Vector4f(abs(pressure[i][j]) / maxP, 0,
                                          1 - abs(pressure[i][j]) / maxP, 1);
        }
    }

    wgpuQueueWriteBuffer(queue, PressureColor, 0, colors.data(), colors.size() * sizeof(Vector4f));
}
