//
// Created by aidar on 24.05.25.
//

#ifndef CFD_H
#define CFD_H
#include "../base/grid.h"
#include "../base/instance.h"
#include "../base/helper.h"

inline std::pair<float, float> upwind(float c) {
    return std::pair<float, float>(std::max(c / (abs(c) + 1e-6), 0.0), std::max(-c / (abs(c) + 1e-6), 0.0));
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
    float velocityIntake = 4.0;
    float F = 0.0;

    // params for box in the middle
    std::vector<sphere> spheres;

    std::vector<int> triangles;
    std::vector<std::vector<Vector2f> > velocity;
    std::vector<std::vector<float> > pressure;
    std::vector<std::vector<int> > stencilMask;

    std::vector<std::vector<Vector2f> > newVelocity;
    std::vector<std::vector<float> > newPressure;

    WGPUBindGroup wgpuBindGroupCompute1;
    WGPUBindGroup wgpuBindGroupCompute2;
    WGPUBindGroup wgpuBindGroupCompute3;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute1;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute2;
    WGPUBindGroupLayout wgpuBindGroupLayoutCompute3;

    WGPUComputePipeline wgpuComputePipelineCompute1;
    WGPUComputePipeline wgpuComputePipelineCompute2;
    WGPUComputePipeline wgpuComputePipelineCompute3;

    WGPUTexture stencilTexture;
    WGPUTexture textureInputVelocities;
    WGPUTexture textureOutputVelocities;
    WGPUTexture textureInputPressure;
    WGPUTexture textureOutputPressure;

    WGPUTextureView stencilTextureView;
    WGPUTextureView textureViewInputVelocities;
    WGPUTextureView textureViewOutputVelocities;
    WGPUTextureView textureViewInputPressure;
    WGPUTextureView textureViewOutputPressure;

    WGPUBuffer PressureColor = nullptr;
    WGPUBuffer PressureModel = nullptr;
    WGPUBuffer uniformsCompute = nullptr; // shared across different compute stages
    WGPUBuffer VelocitiesStagingBuffer = nullptr;
    WGPUBuffer PressureStagingBuffer = nullptr;

public:
    CFD(float step, WGPUDevice device, WGPUQueue queue);

    void InitBuffers(WGPUDevice device, WGPUQueue queue);

    void InitBindGroupLayout(WGPUDevice device, WGPUQueue queue);

    void InitComputePipelines(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline1(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline2(WGPUDevice device, WGPUQueue queue);

    void InitComputePipeline3(WGPUDevice device, WGPUQueue queue);

    void InitTexture(WGPUDevice device, WGPUQueue queue);

    void InitTextureViews(WGPUDevice device, WGPUQueue queue);

    void Render(WGPUDevice device, WGPUQueue queue, WGPURenderPassEncoder pass, WGPUBuffer uniforms) override;

    void UpdateGrid(WGPUDevice device, WGPUQueue queue);

    void InitBindGroup1(WGPUDevice device, WGPUQueue queue);

    void InitBindGroup2(WGPUDevice device, WGPUQueue queue, bool swapP);

    void InitBindGroup3(WGPUDevice device, WGPUQueue queue, WGPUTextureView inputPressure);

    void SetVelocityBorderCondition(std::vector<std::vector<Vector2f> > &velocity);

    void SetPressureBorderCondition(std::vector<std::vector<float> > &pressure);

    void UpdateColors(WGPUDevice device, WGPUQueue queue);

    void InitGrid();

    float getDt();
};


#endif //CFD_H
