//
// Created by aidar on 08.05.25.
//

#include "app.h"

void App::RenderPhase() const {
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(wgpuSurface, &surfaceTexture);

    WGPUTextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = {"Surface texture view",WGPU_STRLEN};
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    WGPUTextureView backBufView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    // #ifndef WEBGPU_BACKEND_WGPU
    //     // We no longer need the texture, only its view
    //     // (NB: with wgpu-native, surface textures must be release after the call to wgpuSurfacePresent)
    //     wgpuTextureRelease(surfaceTexture.texture);
    // #endif // WEBGPU_BACKEND_WGPU

    WGPURenderPassColorAttachment colorDesc = {};
    colorDesc.view = backBufView;
    colorDesc.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorDesc.resolveTarget = nullptr;
    colorDesc.loadOp = WGPULoadOp_Clear;
    colorDesc.storeOp = WGPUStoreOp_Store;
    colorDesc.clearValue.r = 0.0f;
    colorDesc.clearValue.g = 0.0f;
    colorDesc.clearValue.b = 0.0f;
    colorDesc.clearValue.a = 1.0f;

    WGPURenderPassDescriptor renderPass = {};
    renderPass.colorAttachmentCount = 1;
    renderPass.colorAttachments = &colorDesc;

    int frames = static_cast<int>(0.1 / cfd->getDt());
    if (frames > 100) {
        frames = 100;
    } else if (frames <= 0) {
        frames = 1;
    }

    for (int i = 0; i < frames; i++) {
        cfd->UpdateGrid(wgpuDevice, wgpuQueue);
    }

    WGPUCommandEncoderDescriptor commandEncoderDesc = {};
    commandEncoderDesc.label = {"Command encoder",WGPU_STRLEN};

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpuDevice, &commandEncoderDesc); // create encoder



    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPass); // create pass

    wgpuRenderPassEncoderSetBindGroup(pass, 0, wgpuBindGroup, 0, 0);

    // draw the lines
    // wgpuRenderPassEncoderSetPipeline(pass, wgpuRenderPipelineLines);
    //
    // gridV->Render(wgpuQueue, pass, wgpuUbuf);

    // draw the triangles
    wgpuRenderPassEncoderSetPipeline(pass, wgpuRenderPipelineTriangles);
    cfd->Render(wgpuDevice,wgpuQueue, pass, wgpuUbuf);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass); // release pass

    WGPUCommandBufferDescriptor commandDesc = {};
    commandDesc.nextInChain = nullptr;
    commandDesc.label = {"Command buffer",WGPU_STRLEN};

    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, &commandDesc); // create commands
    wgpuCommandEncoderRelease(encoder); // release encoder

    wgpuQueueSubmit(wgpuQueue, 1, &commands);
    wgpuCommandBufferRelease(commands); // release commands

    wgpuTextureViewRelease(backBufView); // release textureView
#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(wgpuSurface);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device, false, nullptr);
#endif
}

void App::initScene(int WindowWidth, int WindowHeight) {
    left = -11.0f / (1.0 * WindowHeight / WindowWidth);
    right = 11.0f / (1.0 * WindowHeight / WindowWidth);
    bottom = -11.0;
    top = 11.0;

    stepy = 1.0f;
    stepx = 1.0f / (1.0 * WindowHeight / WindowWidth);

    gridV = std::make_shared<grid>(1, wgpuDevice, wgpuQueue);

    projection = computeOrthoMatrix(left, right, bottom, top, -100, 100);
    // projection = Matrix4x4();
    camera = Matrix4x4();

    wgpuQueueWriteBuffer(wgpuQueue, wgpuUbuf, offsetof(Uniforms, projection), &projection,
                         sizeof(Uniforms::projection));
    wgpuQueueWriteBuffer(wgpuQueue, wgpuUbuf,offsetof(Uniforms, view), &camera, sizeof(Uniforms::view));

    cfd = std::make_shared<CFD>(0.1, wgpuDevice, wgpuQueue);
}

void App::Init(int kWidth, int kHeight) {
    InitWGPU(kWidth, kHeight);
    initRenderPipeline();
    initUniforms();
    initBindGroup();
    initScene(kWidth, kHeight);
}

void App::InitWGPU(int kWidth, int kHeight) {
    // Init SDL
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = NULL;

    wgpuInstance = wgpuCreateInstance(&desc);

    SDL_Window *window;
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("title",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              kWidth, kHeight, 0);

    // Here we create our WebGPU surface from the window!
    wgpuSurface = SDL_GetWGPUSurface(wgpuInstance, window);

#ifdef __EMSCRIPTEN__
    this->InitWGPUWebGPU(int kWidth, int kHeight);
#else
    this->InitWGPUNative(kWidth, kHeight);
#endif
}

#ifdef __EMSCRIPTEN__
    void App::InitWGPUWebGPU(int width, int height) {
    wgpuInstanceRequestAdapter(
wgpuInstance,
nullptr,
[](WGPURequestAdapterStatus status, WGPUAdapter a,
   char const *message, void *classPointer) {
    if (status != WGPURequestAdapterStatus_Success) {
        std::cout << "RequestAdapter: " << message << "\n";
        exit(0);
    }

    static_cast<App *>(classPointer)->wgpuAdapter = a;
    static_cast<App *>(classPointer)->requestEnded = true;
}, this);
    // ReSharper disable once CppDFALoopConditionNotUpdated
    while (!requestEnded) {
        emscripten_sleep(100);
    }

    requestEnded = false;
    wgpuInstanceRelease(wgpuInstance);

    wgpuAdapterRequestDevice(
    wgpuAdapter,
    nullptr,
    [](WGPURequestDeviceStatus status, WGPUDevice d,
       const char *message, void *classPointer) {
        if (status != WGPURequestDeviceStatus_Success) {
            std::cout << "RequestDevice: " << message << "\n";
            exit(0);
        }
        static_cast<App *>(classPointer)->wgpuDevice = d;
        static_cast<App *>(classPointer)->requestEnded = true;
    }, this);

    while (!requestEnded) {
        emscripten_sleep(100);
    }
    requestEnded = false;

    wgpuDeviceSetUncapturedErrorCallback(wgpuDevice,
                                         [](WGPUErrorType errorType,
                                            const char *message, void *userData) {
                                             std::cout << "Error: " << errorType << " - message: " << message << "\n";
                                         }, nullptr);

    wgpuQueue = wgpuDeviceGetQueue(wgpuDevice);
}
#else
void App::InitWGPUNative(int kWidth, int kHeight) {
    WGPURequestAdapterCallbackInfo callbackInfo = {};

    callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter a,
                               WGPUStringView message, void *classPointer, void *userData) {
        if (status != WGPURequestAdapterStatus_Success) {
            std::cout << "RequestAdapter: " << std::string(message.data, message.length) << "\n";
            exit(0);
        }

        static_cast<App *>(classPointer)->wgpuAdapter = a;
        static_cast<App *>(classPointer)->requestEnded = true;
    };
    callbackInfo.userdata1 = this;
    callbackInfo.nextInChain = nullptr;

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = wgpuSurface;

    wgpuInstanceRequestAdapter(
        wgpuInstance,
        &adapterOpts,
        callbackInfo);

    while (!requestEnded) {
    }

    // We no longer need to access the instance
    wgpuInstanceRelease(wgpuInstance);
    requestEnded = false;

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};

    deviceCallbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice d,
                                     WGPUStringView message, void *classPointer, void *userData) {
        if (status != WGPURequestDeviceStatus_Success) {
            std::cout << "RequestDevice: " << std::string(message.data, message.length) << "\n";
            exit(0);
        }

        static_cast<App *>(classPointer)->wgpuDevice = d;
        static_cast<App *>(classPointer)->requestEnded = true;
    };

    deviceCallbackInfo.userdata1 = this;
    deviceCallbackInfo.nextInChain = nullptr;
    deviceCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;

    WGPUUncapturedErrorCallbackInfo errorCallbackInfo = {};
    errorCallbackInfo.callback = [](WGPUDevice const *device, WGPUErrorType errorType,
                                    WGPUStringView message, void *userData, void *userdata1) {
        std::cout << "Error: " << errorType << " - message: " << std::string(message.data, message.length) << "\n";
    };
    errorCallbackInfo.nextInChain = nullptr;
    errorCallbackInfo.userdata1 = nullptr;

    WGPUDeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.nextInChain = nullptr;
    deviceDescriptor.label = {"device",WGPU_STRLEN};
    deviceDescriptor.requiredLimits = nullptr;
    deviceDescriptor.uncapturedErrorCallbackInfo = errorCallbackInfo;
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = {"queue",WGPU_STRLEN};


    wgpuAdapterRequestDevice(
        wgpuAdapter,
        &deviceDescriptor,
        deviceCallbackInfo);

    while (!requestEnded) {
    }
    requestEnded = false;

    wgpuQueue = wgpuDeviceGetQueue(wgpuDevice);


    WGPUSurfaceConfiguration config = {};
    config.nextInChain = nullptr;
    // Configuration of the textures created for the underlying swap chain
    config.width = kWidth;
    config.height = kHeight;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.format = WGPUTextureFormat_BGRA8Unorm;

    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.device = wgpuDevice;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(wgpuSurface, &config);

    WGPUSurfaceCapabilities wgpuSurfaceCapabilities = {};
    wgpuSurfaceGetCapabilities(wgpuSurface, wgpuAdapter, &wgpuSurfaceCapabilities);

    WGPUSurfaceTexture surfaceTexture = {};
    wgpuSurfaceGetCurrentTexture(wgpuSurface, &surfaceTexture);

    surfaceFormat = wgpuTextureGetFormat(surfaceTexture.texture);

    wgpuTextureRelease(surfaceTexture.texture);

    // Release the adapter only after it has been fully utilized
    wgpuAdapterRelease(wgpuAdapter);
}

#endif


void App::initRenderPipeline() {
    // bind group layout (used by both the pipeline layout and uniform bind group
    WGPUBindGroupLayoutEntry bglEntry = {};
    bglEntry.binding = 0;
    bglEntry.visibility = WGPUShaderStage_Vertex;
    bglEntry.buffer.type = WGPUBufferBindingType_Uniform;
    bglEntry.buffer.minBindingSize = sizeof(Uniforms);

    WGPUBindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount = 1;
    bglDesc.entries = &bglEntry;
    wgpuBindGroupLayout = wgpuDeviceCreateBindGroupLayout(wgpuDevice, &bglDesc);

    // pipeline layout (used by the render pipeline, released after its creation)
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = &wgpuBindGroupLayout;
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(wgpuDevice, &layoutDesc);

    std::string pathPrefix = "";
#ifdef __EMSCRIPTEN__
    pathPrefix = "/";
#endif

    WGPUShaderModule vertMod = createShader(wgpuDevice, "vert", (pathPrefix + "shaders/vert.wgsl").c_str());
    WGPUShaderModule fragMod = createShader(wgpuDevice, "frag", (pathPrefix + "shaders/frag.wgsl").c_str());

    // describe buffer layouts
    WGPUVertexAttribute vertAttrs[7] = {};
    vertAttrs[0].format = WGPUVertexFormat_Float32x2;
    vertAttrs[0].offset = 0;
    vertAttrs[0].shaderLocation = 0;
    vertAttrs[1].format = WGPUVertexFormat_Float32x4;
    vertAttrs[1].offset = 0;
    vertAttrs[1].shaderLocation = 1;
    vertAttrs[2].format = WGPUVertexFormat_Float32;
    vertAttrs[2].offset = 0;
    vertAttrs[2].shaderLocation = 2;

    vertAttrs[3].format = WGPUVertexFormat_Float32x4;
    vertAttrs[3].offset = 0;
    vertAttrs[3].shaderLocation = 3;
    vertAttrs[4].format = WGPUVertexFormat_Float32x4;
    vertAttrs[4].offset = sizeof(Vector4f);
    vertAttrs[4].shaderLocation = 4;
    vertAttrs[5].format = WGPUVertexFormat_Float32x4;
    vertAttrs[5].offset = 2 * sizeof(Vector4f);
    vertAttrs[5].shaderLocation = 5;
    vertAttrs[6].format = WGPUVertexFormat_Float32x4;
    vertAttrs[6].offset = 3 * sizeof(Vector4f);
    vertAttrs[6].shaderLocation = 6;

    WGPUVertexBufferLayout vertexBufferLayout[4] = {};
    vertexBufferLayout[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[0].arrayStride = sizeof(Vector2f);
    vertexBufferLayout[0].attributeCount = 1;
    vertexBufferLayout[0].attributes = vertAttrs;
    vertexBufferLayout[1].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[1].arrayStride = sizeof(Vector4f);
    vertexBufferLayout[1].attributeCount = 1;
    vertexBufferLayout[1].attributes = vertAttrs + 1;
    vertexBufferLayout[2].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[2].arrayStride = sizeof(float);
    vertexBufferLayout[2].attributeCount = 1;
    vertexBufferLayout[2].attributes = vertAttrs + 2;

    vertexBufferLayout[3].stepMode = WGPUVertexStepMode_Instance;
    vertexBufferLayout[3].arrayStride = sizeof(ModelMat);
    vertexBufferLayout[3].attributeCount = 4;
    vertexBufferLayout[3].attributes = vertAttrs + 3;


    // vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
    // vertexBufferLayout.arrayStride = 6 * sizeof(float);
    // vertexBufferLayout.attributeCount = 3;
    // vertexBufferLayout.attributes = vertAttrs;

    // Fragment state
    WGPUBlendState blend = {};
    blend.color.operation = WGPUBlendOperation_Add;
    blend.color.srcFactor = WGPUBlendFactor_One;
    blend.color.dstFactor = WGPUBlendFactor_One;
    blend.alpha.operation = WGPUBlendOperation_Add;
    blend.alpha.srcFactor = WGPUBlendFactor_One;
    blend.alpha.dstFactor = WGPUBlendFactor_One;

    WGPUColorTargetState colorTarget = {};
    colorTarget.format = surfaceFormat; //wgpuSurfaceGetPreferredFormat(surface,adapter);
    colorTarget.blend = &blend;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragment = {};
    fragment.module = fragMod;
    fragment.entryPoint = {"fs_main",WGPU_STRLEN};
    fragment.targetCount = 1;
    fragment.targets = &colorTarget;

    // Create the render pipeline
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.layout = pipelineLayout;
    // State for the fragment shader
    pipelineDesc.fragment = &fragment;
    // We do not use stencil/depth testing for now
    pipelineDesc.depthStencil = nullptr;

    pipelineDesc.vertex.module = vertMod;
    pipelineDesc.vertex.entryPoint = {"vs_main",WGPU_STRLEN};
    pipelineDesc.vertex.bufferCount = 4;
    pipelineDesc.vertex.buffers = vertexBufferLayout;

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = 0xFFFFFFFF;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CW;
    pipelineDesc.primitive.cullMode = WGPUCullMode_None;

    pipelineDesc.label = {"Triangle Render Pipeline",WGPU_STRLEN};

    wgpuRenderPipelineTriangles = wgpuDeviceCreateRenderPipeline(wgpuDevice, &pipelineDesc);

    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_LineList;
    pipelineDesc.label = {"Lines Render Pipeline",WGPU_STRLEN};

    wgpuRenderPipelineLines = wgpuDeviceCreateRenderPipeline(wgpuDevice, &pipelineDesc);

    wgpuPipelineLayoutRelease(pipelineLayout);
    // We no longer need to access the shader module
    wgpuShaderModuleRelease(fragMod);
    wgpuShaderModuleRelease(vertMod);
}

void App::initUniforms() {
    WGPUBufferDescriptor bufDesc = {};
    bufDesc.size = sizeof(Uniforms);
    bufDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    bufDesc.mappedAtCreation = false;
    bufDesc.label = {"Uniform Buffer",WGPU_STRLEN};

    wgpuUbuf = wgpuDeviceCreateBuffer(wgpuDevice, &bufDesc);

    Uniforms uniforms = {};
    uniforms.projection = Matrix4x4();
    uniforms.view = Matrix4x4();

    wgpuQueueWriteBuffer(wgpuQueue, wgpuUbuf, 0, &uniforms, sizeof(Uniforms));
}

void App::initBindGroup() {
    WGPUBindGroupEntry bgEntry = {};
    bgEntry.binding = 0;
    bgEntry.buffer = wgpuUbuf;
    bgEntry.offset = 0;
    bgEntry.size = sizeof(Uniforms);

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout = wgpuBindGroupLayout;
    bgDesc.entryCount = 1;
    bgDesc.entries = &bgEntry;


    wgpuBindGroup = wgpuDeviceCreateBindGroup(wgpuDevice, &bgDesc);
}
