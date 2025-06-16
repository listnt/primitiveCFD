//
// Created by aidar on 20.04.25.
//

#include "base.h"

#include <cstdio>

Matrix4x4 scale(double x, double y, double z) {
    Matrix4x4 S = Matrix4x4();
    S.m[0][0] = x;
    S.m[1][1] = y;
    S.m[2][2] = z;
    S.m[3][3] = 1;

    return S;
}

Matrix4x4 roll(double angle) {
    Matrix4x4 R = Matrix4x4();
    R.m[0][0] = 1;
    R.m[1][1] = cos(angle);
    R.m[1][2] = -sin(angle);
    R.m[2][1] = sin(angle);
    R.m[2][2] = cos(angle);
    R.m[3][3] = 1;

    return R;
}

Matrix4x4 pitch(double angle) {
    Matrix4x4 R = Matrix4x4();
    R.m[0][0] = cos(angle);
    R.m[0][2] = sin(angle);
    R.m[1][1] = 1;
    R.m[2][0] = -sin(angle);
    R.m[2][2] = cos(angle);
    R.m[3][3] = 1;

    return R;
}

Matrix4x4 yaw(double angle) {
    Matrix4x4 R = Matrix4x4();
    R.m[0][0] = cos(angle);
    R.m[0][1] = -sin(angle);
    R.m[1][0] = sin(angle);
    R.m[1][1] = cos(angle);
    R.m[2][2] = 1;
    R.m[3][3] = 1;

    return R;
}

Matrix4x4 translate(double x, double y, double z) {
    Matrix4x4 T = Matrix4x4();
    T.m[3][0] = x;
    T.m[3][1] = y;
    T.m[3][2] = z;
    T.m[0][0] = 1;
    T.m[1][1] = 1;
    T.m[2][2] = 1;
    T.m[3][3] = 1;

    return T;
}

std::vector<float> flatten(Matrix4x4 m) {
    std::vector<float> v;
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            v.push_back(m.m[i][j]);
        }
    }
    return v;
}

ModelMat toModelMat(Matrix4x4 m) {
    ModelMat modelMat = ModelMat();

    modelMat.m[0].x = m.m[0][0];
    modelMat.m[0].y = m.m[1][0];
    modelMat.m[0].z = m.m[2][0];
    modelMat.m[0].w = m.m[3][0];

    modelMat.m[1].x = m.m[0][1];
    modelMat.m[1].y = m.m[1][1];
    modelMat.m[1].z = m.m[2][1];
    modelMat.m[1].w = m.m[3][1];

    modelMat.m[2].x = m.m[0][2];
    modelMat.m[2].y = m.m[1][2];
    modelMat.m[2].z = m.m[2][2];
    modelMat.m[2].w = m.m[3][2];

    modelMat.m[3].x = m.m[0][3];
    modelMat.m[3].y = m.m[1][3];
    modelMat.m[3].z = m.m[2][3];
    modelMat.m[3].w = m.m[3][3];

    return modelMat;
}

Matrix4x4 computePerspectiveMatrix(float fovInRads, float aspectRatio, float near, float far) {
    Matrix4x4 perspective = Matrix4x4();

    auto f = 1.0 / tan(fovInRads / 2);
    auto rangeinv = 1.0 / (near - far);

    perspective.m[0][0] = f / aspectRatio;
    perspective.m[1][1] = f;
    perspective.m[2][2] = (near + far) * rangeinv;\
    perspective.m[2][3] = -1;
    perspective.m[3][2] = 2 * near * far * rangeinv;

    return perspective;
}

Matrix4x4 lookAt(Vector3f eye, Vector3f center, Vector3f u) {
    Matrix4x4 lookAt = Matrix4x4();
    Vector3f forward = (eye - center).normalize();
    Vector3f right = forward.cross(u).normalize();
    Vector3f up = right.cross(forward).normalize();

    lookAt.m[0][0] = right.x;
    lookAt.m[1][0] = up.x;
    lookAt.m[2][0] = forward.x;
    lookAt.m[0][1] = right.y;
    lookAt.m[1][1] = up.y;
    lookAt.m[2][1] = forward.y;
    lookAt.m[0][2] = right.z;
    lookAt.m[1][2] = up.z;
    lookAt.m[2][2] = forward.z;
    lookAt.m[3][0] = 0;
    lookAt.m[3][1] = 0;
    lookAt.m[3][2] = 0;
    lookAt.m[3][3] = 1;

    return lookAt * translate(-eye.x, -eye.y, -eye.z);
}

Matrix4x4 inverse(Matrix4x4 m) {
    /* Structure of matrix

         0   1   2   3
        ______________
     0 | 0   4   8  12
     1 | 1   5   9  13
     2 | 2   6  10  14
     3 | 3   7  11  15
    */

    // Factored out common terms
    auto t9_14_13_10 = m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2];
    auto t13_6_5_14 = m.m[1][3] * m.m[2][1] - m.m[1][1] * m.m[2][3];
    auto t5_10_9_6 = m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1];
    auto t12_10_8_14 = m.m[0][3] * m.m[2][2] - m.m[0][2] * m.m[2][3];
    auto t4_14_12_6 = m.m[0][1] * m.m[2][3] - m.m[0][3] * m.m[2][1];
    auto t8_6_4_10 = m.m[0][2] * m.m[2][1] - m.m[0][1] * m.m[2][2];
    auto t8_13_12_9 = m.m[0][2] * m.m[1][3] - m.m[0][3] * m.m[1][2];
    auto t12_5_4_13 = m.m[0][3] * m.m[1][1] - m.m[0][1] * m.m[1][3];
    auto t4_9_8_5 = m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1];
    auto t1_14_13_2 = m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0];
    auto t9_2_1_10 = m.m[1][2] * m.m[2][0] - m.m[1][0] * m.m[2][2];
    auto t12_2_0_14 = m.m[0][3] * m.m[2][0] - m.m[0][0] * m.m[2][3];
    auto t0_10_8_2 = m.m[0][0] * m.m[2][2] - m.m[0][2] * m.m[2][0];
    auto t0_13_12_1 = m.m[0][0] * m.m[1][3] - m.m[0][3] * m.m[1][0];
    auto t8_1_0_9 = m.m[0][2] * m.m[1][0] - m.m[0][0] * m.m[1][2];
    auto t1_6_5_2 = m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0];
    auto t4_2_0_6 = m.m[0][1] * m.m[2][0] - m.m[0][0] * m.m[2][1];
    auto t0_5_4_1 = m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];

    Matrix4x4 Inv;
    Inv.m[0][0] = m.m[3][1] * t9_14_13_10 + m.m[3][2] * t13_6_5_14 + m.m[3][3] * t5_10_9_6;
    Inv.m[0][1] = m.m[3][1] * t12_10_8_14 + m.m[3][2] * t4_14_12_6 + m.m[3][3] * t8_6_4_10;
    Inv.m[0][2] = m.m[3][1] * t8_13_12_9 + m.m[3][2] * t12_5_4_13 + m.m[3][3] * t4_9_8_5;
    Inv.m[0][3] = m.m[2][1] * (-t8_13_12_9) + m.m[2][2] * (-t12_5_4_13) + m.m[2][3] * (-t4_9_8_5);
    Inv.m[1][0] = m.m[3][0] * (-t9_14_13_10) + m.m[3][2] * t1_14_13_2 + m.m[3][3] * t9_2_1_10;
    Inv.m[1][1] = m.m[3][0] * (-t12_10_8_14) + m.m[3][2] * t12_2_0_14 + m.m[3][3] * t0_10_8_2;
    Inv.m[1][2] = m.m[3][0] * (-t8_13_12_9) + m.m[3][2] * t0_13_12_1 + m.m[3][3] * t8_1_0_9;
    Inv.m[1][3] = m.m[2][0] * t8_13_12_9 + m.m[2][2] * (-t0_13_12_1) + m.m[2][3] * (-t8_1_0_9);
    Inv.m[2][0] = m.m[3][0] * (-t13_6_5_14) + m.m[3][1] * (-t1_14_13_2) + m.m[3][3] * t1_6_5_2;
    Inv.m[2][1] = m.m[3][0] * (-t4_14_12_6) + m.m[3][1] * (-t12_2_0_14) + m.m[3][3] * t4_2_0_6;
    Inv.m[2][2] = m.m[3][0] * (-t12_5_4_13) + m.m[3][1] * (-t0_13_12_1) + m.m[3][3] * t0_5_4_1;
    Inv.m[2][3] = m.m[2][0] * t12_5_4_13 + m.m[2][1] * t0_13_12_1 + m.m[2][3] * (-t0_5_4_1);
    Inv.m[3][0] = m.m[3][0] * (-t5_10_9_6) + m.m[3][1] * (-t9_2_1_10) + m.m[3][2] * (-t1_6_5_2);
    Inv.m[3][1] = m.m[3][0] * (-t8_6_4_10) + m.m[3][1] * (-t0_10_8_2) + m.m[3][2] * (-t4_2_0_6);
    Inv.m[3][2] = m.m[3][0] * (-t4_9_8_5) + m.m[3][1] * (-t8_1_0_9) + m.m[3][2] * (-t0_5_4_1);
    Inv.m[3][3] = m.m[2][0] * t4_9_8_5 + m.m[2][1] * t8_1_0_9 + m.m[2][2] * t0_5_4_1;

    auto det =
            m.m[3][0] * (m.m[2][1] * (-t8_13_12_9) + m.m[2][2] * (-t12_5_4_13) + m.m[2][3] * (-t4_9_8_5)) +
            m.m[3][1] * (m.m[2][0] * t8_13_12_9 + m.m[2][2] * (-t0_13_12_1) + m.m[2][3] * (-t8_1_0_9)) +
            m.m[3][2] * (m.m[2][0] * t12_5_4_13 + m.m[2][1] * t0_13_12_1 + m.m[2][3] * (-t0_5_4_1)) +
            m.m[3][3] * (m.m[2][0] * t4_9_8_5 + m.m[2][1] * t8_1_0_9 + m.m[2][2] * t0_5_4_1);

    if (det != 0) {
        auto scale = 1 / det;
        for (int j = 0; j < 16; j += 1) {
            Inv.m[j / 4][j % 4] = Inv.m[j / 4][j % 4] * scale;
        }
    }

    return Inv;
}

Vector3f Lerp(Vector3f a, Vector3f b, double t) {
    return Vector3f(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}

Vector2f Lerp(Vector2f a, Vector2f b, double t) {
    return Vector2f(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

Vector4f ZIndexToColor4f(int zIndex) {
    int r = (zIndex & 0x000000FF) >> 0;
    int g = (zIndex & 0x0000FF00) >> 8;
    int b = (zIndex & 0x00FF0000) >> 16;
    int a = (zIndex & 0xFF000000) >> 24;

    return {
        static_cast<float>(r / 255.0), static_cast<float>(g / 255.0), static_cast<float>(b / 255.0),
        static_cast<float>(a / 255.0)
    };
}

std::vector<Vector2f> CreateCircle(Vector2f center, float radius, int segments) {
    std::vector<Vector2f> points;
    points.reserve(3 * segments);
    for (int i = 0; i < segments; i++) {
        points.push_back(Vector2f(center.x + radius * cos(2 * M_PI * (i) / segments),
                                  center.y + radius * sin(2 * M_PI * (i) / segments)));
        points.push_back(Vector2f(center.x + radius * cos(2 * M_PI * (i + 1) / segments),
                                  center.y + radius * sin(2 * M_PI * (i + 1) / segments)));
        points.push_back(center);
    }
    return points;
}

Matrix4x4 computeOrthoMatrix(float left, float right, float bottom, float top, float near, float far) {
    Matrix4x4 ortho = Matrix4x4();
    ortho.m[0][0] = 2 / (right - left);
    ortho.m[1][1] = 2 / (top - bottom);
    ortho.m[2][2] = -2 / (far - near);
    ortho.m[0][3] = -(right + left) / (right - left);
    ortho.m[1][3] = -(top + bottom) / (top - bottom);
    ortho.m[2][3] = -(far + near) / (far - near);
    ortho.m[3][3] = 1;
    return ortho;
}
