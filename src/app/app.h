//
// Created by aidar on 08.05.25.
//

#ifndef APP_H
#define APP_H
#include <memory>
#include <SDL2/SDL.h>
#include <webgpu/webgpu.h>


#include "CFD.h"
#include "../base/helper.h"
#include <sdl2webgpu/sdl2webgpu.h>
#include "../base/grid.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

class App {
protected:
    WGPUInstance wgpuInstance;
    WGPUAdapter wgpuAdapter;
    WGPUDevice wgpuDevice;
    WGPUQueue wgpuQueue;
    // WGPUSwapChain wgpuSwapchain;

    WGPUSurface wgpuSurface;

    WGPURenderPipeline wgpuRenderPipelineTriangles;
    WGPURenderPipeline wgpuRenderPipelineLines;

    WGPUBindGroup wgpuBindGroup;

    std::vector<WGPUBindGroupLayout> wgpuBindGroups;

    WGPUTextureFormat surfaceFormat = WGPUTextureFormat_Undefined;

    WGPUBuffer wgpuUbuf;

    bool requestEnded = false;

    // std::shared_ptr<PickingTexture> pickingTexture = std::make_shared<PickingTexture>();
    //
    // std::unordered_map<int, std::shared_ptr<Line> > lines;
    std::shared_ptr<grid> gridV;
    Matrix4x4 projection;
    Matrix4x4 camera;
    // int focusedObj = -1;
    // bool isCaptured = false;
    bool isMouseDown = false;
    // bool isContextOpen = false;
    // bool disablePanning = false;
    // int mouseClickedX = 0, mouseClickedY = 0;
    float left = -25, right = 25, bottom = -25, top = 25, stepx = 1, stepy = 1;;
    int kWidth, kHeight;
    // int selectedInstrument = 0;
    // const std::vector<char *> instruments = {
    //     "pointer",
    //     "line",
    //     "bizier",
    // };
    //
    // Vector4f currentColor = {1, 1, 1, 1};

    std::shared_ptr<CFD> cfd;

public:
    // void PickingPhase();

    void RenderPhase() const;

    // bool keyboarControl(int k, const EmscriptenKeyboardEvent *e, void *u);

    bool mouseControlDown(int eventType, const EmscriptenMouseEvent *e, void *);

    // void pickObject(const EmscriptenMouseEvent *e);
    //
    // void addLine(const EmscriptenMouseEvent *e);
    //
    // void addBizierCurve(const EmscriptenMouseEvent *e);
    //
    // void addPlotter(const EmscriptenMouseEvent *e);

    bool mouseControlUp(int eventType, const EmscriptenMouseEvent *e, void *);

    bool mouseControlMove(int eventType, const EmscriptenMouseEvent *e, void *);

    // bool mouseControlWheel(int eventType, const EmscriptenWheelEvent *e, void *);
    //
    // void AddRandomBizierCurve();
    //
    // void buildGui();
    //
    // void initGui(SDL_Window *window, SDL_GLContext gl_context);

    void Init(int kWidth, int kHeight);

    void initScene(int WindowWidth, int WindowHeight);

    void InitWGPU(int kWidth, int kHeight);
#ifdef __EMSCRIPTEN__
    void InitWGPUWebGPU(int kWidth, int kHeight);
#else
    void InitWGPUNative(int kWidth, int kHeight);
#endif

    void initRenderPipeline();

    void initUniforms();

    void initBindGroup();
};


#endif //APP_H
