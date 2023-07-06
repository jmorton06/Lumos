#ifndef TEST_PATTERN_RENDERER_H
#define TEST_PATTERN_RENDERER_H

#include <stdint.h>
#include "glfm.h"

typedef struct Renderer Renderer;
typedef uintptr_t Texture;
#define NULL_TEXTURE (0)

typedef struct {
    float position[2];
    float texCoord[2];
} Vertex;

struct Renderer {
    Texture (*textureUpload)(Renderer *renderer, uint32_t width, uint32_t height, uint8_t *data);
    void (*textureDestroy)(Renderer *renderer, Texture texture);
    void (*drawFrameStart)(Renderer *renderer, int screenWidth, int screenHeight);
    void (*drawFrameEnd)(Renderer *renderer);
    void (*drawQuad)(Renderer *renderer, Texture texture, const Vertex (*vertices)[4]);
    void (*destroy)(Renderer *renderer);
};

Renderer *createRendererGLES2(GLFMDisplay *display);

#if defined(__APPLE__)
Renderer *createRendererMetal(GLFMDisplay *display);
#endif

#endif
