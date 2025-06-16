//
// Created by aidar on 30.04.25.
//

#include "grid.h"

grid::grid(float step, WGPUDevice device, WGPUQueue queue) : step(step) {
    for (float x = -10; x < 10; x += step) {
        for (float y = -10; y < 10; y += step) {
            points.push_back(Vector2f(x, y));
            points.push_back(Vector2f(x, y - step));
        }
    }

    for (float y = -10; y < 10; y += step) {
        for (float x = -10; x < 10; x += step) {
            points.push_back(Vector2f(x, y));
            points.push_back(Vector2f(x - step, y));
        }
    }


    colors = std::vector<Vector4f>(points.size(), {1.0,0.0,0.0, 1.0});
    pickingColor = std::vector<Vector4f>(points.size(), {1.0, 0.0, 0.0, 1.0});
    zIndex = std::vector<float>(points.size(), 0);

    LoadWGPUBuffers(device, queue);
}

void grid::UpdateGrid(float left, float right, float top, float bottom) {
    Vector2f currentCenter = {(left + right) / 2.0f, (top + bottom) / 2.0f};
    if (abs(currentCenter.x - currentPivot.x) > step || abs(currentCenter.y - currentPivot.y) > step) {
        int xCell = static_cast<int>(currentCenter.x / step);
        int yCell = static_cast<int>(currentCenter.y / step);
        currentPivot = {xCell * step, yCell * step};

        T = Vector3f(currentPivot.x, currentPivot.y, 0);
    }

    if ((top - bottom) / step < 4 && step > 1) {
        step = step / (1.0 * currentScaleStep);
        S = S / (float) currentScaleStep;

        switch (currentScaleStep) {
            case 5:
                currentScaleStep = 4;
                break;
            case 4:
                currentScaleStep = 5;
                break;
        }
    } else if ((top - bottom) / step > 20) {
        switch (currentScaleStep) {
            case 5:
                currentScaleStep = 4;
                break;
            case 4:
                currentScaleStep = 5;
                break;
        }

        step = currentScaleStep * step;
        S = S * currentScaleStep;


        Vector2f currentCenter = {(left + right) / 2.0f, (top + bottom) / 2.0f};
        if (abs(currentCenter.x - currentPivot.x) > step || abs(currentCenter.y - currentPivot.y) > step) {
            int xCell = static_cast<int>(currentCenter.x / step);
            int yCell = static_cast<int>(currentCenter.y / step);
            currentPivot = {xCell * step, yCell * step};

            T = Vector3f(currentPivot.x, currentPivot.y, 0);
        }
    }
}
