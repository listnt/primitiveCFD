//
// Created by aidar on 16.04.25.
//

#ifndef INSTANCE_H
#define INSTANCE_H

#include <iostream>
#include <math.h>
#include <memory>
#include <mutex>
#include <vector>

#include "base.h"


class model {
protected:
    std::vector<Vector2f> points;
    std::vector<float> zIndex;
    std::vector<Vector4f> pickingColor;
    std::vector<Vector4f> colors;

public:
    model(const std::vector<Vector2f> &points, float zIndex, Vector4f color): points(points),
                                                                              zIndex(std::vector<float>(
                                                                                  points.size(), zIndex)),
                                                                              colors(std::vector<Vector4f>(
                                                                                  points.size(), color)),
                                                                              pickingColor(points.size(),
                                                                                  ZIndexToColor4f(
                                                                                      static_cast<int>(std::round(
                                                                                          zIndex)))) {
    }

    model() = default;

    std::vector<Vector2f> getPoints() {
        return points;
    }

    std::vector<Vector4f> getColor() {
        return colors;
    }

    std::vector<float> getZIndeex() {
        return zIndex;
    }

    int getZIndex() {
        return static_cast<int>(std::round(zIndex[0]));
    }

    std::vector<Vector4f> getPickingColor() {
        return pickingColor;
    }
};

class instance : public model {
protected:
    WGPUBuffer Positions = nullptr;
    WGPUBuffer Colors = nullptr;
    WGPUBuffer PickingColor = nullptr;
    WGPUBuffer zIndexBuffer = nullptr;

    ModelMat modelMat = ModelMat();
    WGPUBuffer Model = nullptr;


    void LoadWGPUBuffers(WGPUDevice device, WGPUQueue queue);

public:
    Vector3f T, S, R;

    instance(): T(0, 0, 0), S(1, 1, 1), R(0, 0, 0) {
    };

    void loadModel(WGPUDevice device, WGPUQueue queue, const std::shared_ptr<model> &obj) {
        points = obj->getPoints();
        zIndex = obj->getZIndeex();
        colors = obj->getColor();
        pickingColor = std::vector<Vector4f>(points.size(), ZIndexToColor4f(static_cast<float>(std::round(zIndex[0]))));

        LoadWGPUBuffers(device, queue);
    }


    ~instance() {
        printf("Delete instance\n");
        wgpuBufferDestroy(Positions);
        wgpuBufferDestroy(Colors);
        wgpuBufferDestroy(PickingColor);
        wgpuBufferDestroy(zIndexBuffer);

        wgpuBufferDestroy(Model);
    }

    virtual void Render(WGPUDevice device, WGPUQueue queue, WGPURenderPassEncoder pass, WGPUBuffer uniforms);

    virtual void Picking(WGPUQueue queue, WGPURenderPassEncoder pass, WGPUBuffer uniforms);

    void Transform(Vector3f T);

    void Rotate(Vector3f R);

    void Scale(Vector3f S);

    void virtual Pick() {
    };

    void virtual Unpick() {
    };

    virtual void SetUniformColor(Vector4f newColor, WGPUQueue queue);
};

#endif //INSTANCE_H
