#include <cstdio>
#include <SDL2/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl2webgpu/sdl2webgpu.h>
#include "random"
#include <iostream>

#include "src/app/app.h"


auto app = new App();

void Draw() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            exit(1);
        }
    }

    app->RenderPhase();
}


#ifdef __cplusplus

extern "C" {
#endif
int init(const int width, const int height, bool debug) {
    app->Init(width,height);

#ifdef EMSCRIPTEN
    emscripten_set_mousedown_callback("#canvas", nullptr, 0,
                                      [](int k,const EmscriptenMouseEvent *e,void * u) -> EM_BOOL{
                                      return app->mouseControlDown(k,e,u);
                                      });
    emscripten_set_mouseup_callback("#canvas", nullptr, 0, [](int k,const EmscriptenMouseEvent *e,void * u) -> EM_BOOL{
                                    return app->mouseControlUp(k,e,u);
                                    });
    emscripten_set_mousemove_callback("#canvas", nullptr, 0,
                                      [](int k,const EmscriptenMouseEvent *e,void * u) -> EM_BOOL{
                                      return app->mouseControlMove(k,e,u);
                                      });
    // emscripten_set_wheel_callback("#canvas", nullptr, 0, [](int k,const EmscriptenWheelEvent *e,void * u) -> EM_BOOL{
    //                               return app->mouseControlWheel(k,e,u);
    //                               });
    // emscripten_set_keydown_callback("#canvas", nullptr, 0,
    //                                 [](int k,const EmscriptenKeyboardEvent *e,void * u) -> EM_BOOL{
    //                                 return app->keyboarControl(k,e,u);
    //                                 });

    emscripten_set_main_loop(Draw, 0, true);

#else
    while (true) {
        Draw();
    }
#endif
    return true;
}

#ifdef __cplusplus
}
#endif

int main(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        std::cout<< argv[i] << std::endl;
    }
    // Seed the random number generator with the current time
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    init(640, 480, true);

    return 1;
}
