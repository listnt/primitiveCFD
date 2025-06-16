//
// Created by aidar on 16.04.25.
//

#include "instance.h"
#include <numeric>

#include <chrono> // Import the ctime library

void instance::LoadWGPUBuffers(WGPUDevice device, WGPUQueue queue) {
    WGPUBufferDescriptor bufferDescriptor = {};
    bufferDescriptor.size = points.size() * sizeof(Vector2f);
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDescriptor.mappedAtCreation = false;
    Positions = wgpuDeviceCreateBuffer(device, &bufferDescriptor);

    wgpuQueueWriteBuffer(queue, Positions, 0, points.data(), points.size() * sizeof(Vector2f));

    bufferDescriptor.size = colors.size() * sizeof(Vector4f);
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDescriptor.mappedAtCreation = false;
    Colors = wgpuDeviceCreateBuffer(device, &bufferDescriptor);
    wgpuQueueWriteBuffer(queue, Colors, 0, colors.data(), colors.size() * sizeof(Vector4f));

    bufferDescriptor.size = pickingColor.size() * sizeof(Vector4f);
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDescriptor.mappedAtCreation = false;
    PickingColor = wgpuDeviceCreateBuffer(device, &bufferDescriptor);
    wgpuQueueWriteBuffer(queue, PickingColor, 0, pickingColor.data(), pickingColor.size() * sizeof(Vector4f));

    bufferDescriptor.size = zIndex.size() * sizeof(float);
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDescriptor.mappedAtCreation = false;
    zIndexBuffer = wgpuDeviceCreateBuffer(device, &bufferDescriptor);
    wgpuQueueWriteBuffer(queue, zIndexBuffer, 0, zIndex.data(), zIndex.size() * sizeof(float));

    bufferDescriptor.size = sizeof(ModelMat);
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDescriptor.mappedAtCreation = false;
    Model = wgpuDeviceCreateBuffer(device, &bufferDescriptor);
    wgpuQueueWriteBuffer(queue, Model, 0, &modelMat, sizeof(ModelMat));
}

void instance::Render(WGPUDevice device,WGPUQueue queue, WGPURenderPassEncoder pass, WGPUBuffer uniforms) {
    auto Model = translate(T.x, T.y, T.z) *
                 roll(R.z) * pitch(R.y) * yaw(R.x) *
                 scale(S.x, S.y, S.z);

    auto modelMat = toModelMat(Model);

    wgpuQueueWriteBuffer(queue, this->Model, 0, &modelMat, sizeof(ModelMat));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, Positions, 0, points.size() * sizeof(Vector2f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 1, Colors, 0, colors.size() * sizeof(Vector4f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 2, zIndexBuffer, 0, zIndex.size() * sizeof(float));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 3, this->Model, 0, sizeof(ModelMat));

    if (points.size() < 2) {
        return;
    }

    wgpuRenderPassEncoderDraw(pass, points.size(), 1, 0, 0);
}

void instance::Picking(WGPUQueue queue, WGPURenderPassEncoder pass, WGPUBuffer uniforms) {
    auto Model = translate(T.x, T.y, T.z) *
                 roll(R.z) * pitch(R.y) * yaw(R.x)
                 * scale(S.x, S.y, S.z);

    auto modelMat = toModelMat(Model);

    wgpuQueueWriteBuffer(queue, this->Model, 0, &modelMat, sizeof(ModelMat));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, Positions, 0, points.size() * sizeof(Vector2f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 1, PickingColor, 0, pickingColor.size() * sizeof(Vector4f));
    wgpuRenderPassEncoderSetVertexBuffer(pass, 2, zIndexBuffer, 0, zIndex.size() * sizeof(float));

    wgpuRenderPassEncoderSetVertexBuffer(pass, 3, this->Model, 0, sizeof(ModelMat));
}

void instance::Transform(Vector3f T) { this->T = T; }

void instance::Rotate(Vector3f R) { this->R = R; }

void instance::Scale(Vector3f S) { this->S = S; }

void instance::SetUniformColor(Vector4f newColor, WGPUQueue queue) {
    for (auto &color: colors) {
        color = newColor;
    }

    wgpuQueueWriteBuffer(queue, Colors, 0, colors.data(), colors.size() * sizeof(Vector4f));
}
