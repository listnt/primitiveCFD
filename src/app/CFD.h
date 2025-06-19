//
// Created by aidar on 24.05.25.
//

#ifndef CFD_H
#define CFD_H
#include "../base/grid.h"
#include "../base/instance.h"
#include "../base/helper.h"
#include <webgpu/webgpu.h>
#include <cstring>

inline std::pair<float, float> upwind(float c) {
    return std::pair<float, float>(std::max(c / (abs(c) + 1e-6), 0.0), std::max(-c / (abs(c) + 1e-6), 0.0));
}

inline uint32_t getMaxMipLevelCount(const WGPUExtent3D &textureSize) {
    return std::bit_width(std::max(textureSize.width, textureSize.height));
}

struct sphere {
    int x;
    int y;
    int r;
};

struct CFDUniforms {
    float step;
    float dt;
    float rho;
    float kinematicViscosity;
    float velocityIntake;
};

class CFD : public grid {
    WGPUBuffer EBO = nullptr;

    float dt;
    float lx = 20.0;
    float ly = 10.0;
    int Nx = 10;
    int Ny = 10;
    float rho = 1.0;
    float KinematicViscosity = 0.01;
    float velocityIntake = 2.0;
    float F = 0.0;

    // params for box in the middle
    std::vector<sphere> spheres;

    std::vector<int> triangles;
    std::vector<std::vector<Vector2f> > velocity;
    std::vector<std::vector<float> > pressure;
    std::vector<std::vector<int> > stencilMask;

    std::vector<std::vector<Vector2f> > newVelocity;
    std::vector<std::vector<float> > newPressure;

    WGPUBindGroup wgpuBindGroupRender;

    WGPUBindGroup wgpuBindGroupCompute1;
    WGPUBindGroup wgpuBindGroupCompute2;
    WGPUBindGroup wgpuBindGroupCompute3;
    WGPUBindGroup wgpuBindGroupCompute4;
    WGPUBindGroup wgpuBindGroupCompute5;
    WGPUBindGroup wgpuBindGroupCompute6;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute1;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute2;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute3;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute4;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute5;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute6;

    WGPUComputePipeline wgpuComputePipelineCompute1;
    WGPUComputePipeline wgpuComputePipelineCompute2;
    WGPUComputePipeline wgpuComputePipelineCompute3;
    WGPUComputePipeline wgpuComputePipelineCompute4;
    WGPUComputePipeline wgpuComputePipelineCompute5;
    WGPUComputePipeline wgpuComputePipelineCompute6;

    WGPUTexture stencilTexture;
    WGPUTexture textureInputVelocities;
    WGPUTexture textureOutputVelocities;
    WGPUTexture textureInputPressure;
    WGPUTexture textureOutputPressure;

    WGPUTexture textureCurlP;
    std::vector<WGPUExtent3D> textureCurlPMipSizes;

    WGPUTexture textureFancyColors;

    WGPUTextureView stencilTextureView;
    WGPUTextureView textureViewInputVelocities;
    WGPUTextureView textureViewOutputVelocities;
    WGPUTextureView textureViewInputPressure;
    WGPUTextureView textureViewOutputPressure;
    std::vector<WGPUTextureView> textureViewCurlPMipViews;
    WGPUTextureView textureViewFancyColor;

    WGPUBuffer PressureColor = nullptr;
    WGPUBuffer PressureModel = nullptr;
    WGPUBuffer uniformsCompute = nullptr; // shared across different compute stages

public:
    CFD(float step, WGPUDevice device, WGPUQueue queue, WGPUBindGroupLayout meshDependentLayout);

    void InitBuffers(WGPUDevice device, WGPUQueue queue);

    void InitBindGroupLayout(WGPUDevice device, WGPUQueue queue);

    void InitComputePipelines(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline1(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline2(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline3(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline4(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline5(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline6(WGPUDevice device, WGPUQueue queue);

    void InitTexture(WGPUDevice device, WGPUQueue queue);

    void InitTextureViews(WGPUDevice device, WGPUQueue queue);

    void Render(WGPUDevice device, WGPUQueue queue, WGPURenderPassEncoder pass, WGPUBuffer uniforms) override;

    void UpdateGrid(WGPUDevice device, WGPUQueue queue, int frames);

    void InitBindGroup1(WGPUDevice device, WGPUQueue queue);

    void InitBindGroup2(WGPUDevice device, WGPUQueue queue, bool swapP);

    void InitBindGroup3(WGPUDevice device, WGPUQueue queue, WGPUTextureView inputPressure);

    void InitBindGroup4(WGPUDevice device, WGPUQueue queue);

    void InitBindGroup5(WGPUDevice device, WGPUQueue queue, int nextMipLevel);

    void InitBindGroup6(WGPUDevice device, WGPUQueue queue);

    void InitRenderBindGroup(WGPUDevice device, WGPUQueue, WGPUBindGroupLayout meshDependentLayout);

    void SetVelocityBorderCondition(std::vector<std::vector<Vector2f> > &velocity);

    void SetPressureBorderCondition(std::vector<std::vector<float> > &pressure);

    void InitGrid();

    float getDt();

    void DrawLine(WGPUQueue queue, float x0, float y0, float x1, float y1);
};

#endif //CFD_H
