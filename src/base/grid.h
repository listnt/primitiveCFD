//
// Created by aidar on 30.04.25.
//

#ifndef GRID_H
#define GRID_H
#include "instance.h"


class grid : public instance {
protected:
    float step;
    int currentScaleStep = 5;
    Vector2f currentPivot = {0, 0};

public:
    grid() {
    };

    grid(float step, WGPUDevice device, WGPUQueue queue);

    void UpdateGrid(float left, float right, float top, float bottom);
};


#endif //GRID_H
