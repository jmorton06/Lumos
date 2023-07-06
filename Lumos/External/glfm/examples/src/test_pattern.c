// Draws a test pattern to check if framebuffer is scaled correctly.
// Tap to modify interface chrome (navigation bar, status bar, etc)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glfm.h"
#include "test_pattern_renderer.h"
#include "file_compat.h"

#define FILE_COMPAT_ANDROID_ACTIVITY glfmGetAndroidActivity(display)

typedef struct {
    Renderer *renderer;
    Texture texture;
    bool textureNeedsUpdate;
    bool needsRedraw;
} TestPatternApp;

static Texture createTestPatternTexture(GLFMDisplay *display, uint32_t width, uint32_t height) {
    double top, right, bottom, left;
    glfmGetDisplayChromeInsets(display, &top, &right, &bottom, &left);

    static const uint32_t borderColor = 0xff0000ff;
    static const uint32_t insetColor = 0xffff3322;

    TestPatternApp *app = glfmGetUserData(display);
    Texture texture = 0;
    uint32_t *data = malloc(width * height * sizeof(uint32_t));
    if (data) {
        uint32_t *out = data;
        for (uint32_t y = 0; y < height; y++) {
            *out++ = borderColor;
            if (y == 0 || y == height - 1) {
                for (uint32_t x = 1; x < width - 1; x++) {
                    *out++ = borderColor;
                }
            } else if (y < bottom || y >= height - top) {
                for (uint32_t x = 1; x < width - 1; x++) {
                    *out++ = insetColor;
                }
            } else {
                uint32_t x = 1;
                while (x < left) {
                    *out++ = insetColor;
                    x++;
                }
                while (x < width - right - 1) {
                    *out++ = ((x & 1U) == (y & 1U)) ? 0xff000000 : 0xffffffff;
                    x++;
                }

                while (x < width - 1) {
                    *out++ = insetColor;
                    x++;
                }
            }
            *out++ = borderColor;
        }

        texture = app->renderer->textureUpload(app->renderer, width, height, (uint8_t *)data);

        free(data);
    }
    if (texture != 0) {
        printf("Created test pattern %ix%i with insets %i, %i, %i, %i\n", width, height,
               (int)top, (int)right, (int)bottom, (int)left);
    }
    return texture;
}

static void onSurfaceCreated(GLFMDisplay *display, int width, int height) {
    TestPatternApp *app = glfmGetUserData(display);
#if defined(__APPLE__)
    if (glfmGetRenderingAPI(display) == GLFMRenderingAPIMetal) {
        app->renderer = createRendererMetal(display);
        printf("Hello from Metal!\n");
    }
#endif
    if (!app->renderer) {
        app->renderer = createRendererGLES2(display);
        printf("Hello from GLES2!\n");
    }
    app->textureNeedsUpdate = true;
    app->needsRedraw = true;
}

static void onSurfaceResized(GLFMDisplay *display, int width, int height) {
    TestPatternApp *app = glfmGetUserData(display);
    app->textureNeedsUpdate = true;
}

static void onSurfaceRefresh(GLFMDisplay *display) {
    TestPatternApp *app = glfmGetUserData(display);
    app->needsRedraw = true;
}

static void onOrientationChange(GLFMDisplay *display, GLFMInterfaceOrientation orientation) {
    TestPatternApp *app = glfmGetUserData(display);
    app->textureNeedsUpdate = true;
}

static void onInsetsChange(GLFMDisplay *display, double top, double right, double bottom, double left) {
    TestPatternApp *app = glfmGetUserData(display);
    app->textureNeedsUpdate = true;
}

static void onSurfaceDestroyed(GLFMDisplay *display) {
    // When the surface is destroyed, all existing GL resources are no longer valid.
    TestPatternApp *app = glfmGetUserData(display);
    app->renderer->textureDestroy(app->renderer, app->texture);
    app->renderer->destroy(app->renderer);
    app->renderer = NULL;
}

static bool onTouch(GLFMDisplay *display, int touch, GLFMTouchPhase phase, double x, double y) {
    if (phase == GLFMTouchPhaseBegan) {
        char *chromeString = NULL;
        GLFMUserInterfaceChrome chrome = glfmGetDisplayChrome(display);
        switch (chrome) {
            case GLFMUserInterfaceChromeNavigation:
                chromeString = "Navigation+StatusBar";
                glfmSetDisplayChrome(display, GLFMUserInterfaceChromeNavigationAndStatusBar);
                break;
            case GLFMUserInterfaceChromeNavigationAndStatusBar:
                chromeString = "None";
                glfmSetDisplayChrome(display, GLFMUserInterfaceChromeNone);
                break;
            case GLFMUserInterfaceChromeNone: default:
                chromeString = "Navigation";
                glfmSetDisplayChrome(display, GLFMUserInterfaceChromeNavigation);
                break;
        }
        glfmPerformHapticFeedback(display, GLFMHapticFeedbackLight);
        printf("Chrome set to: %s\n", chromeString);
        return true;
    } else {
        return false;
    }
}

static void onDraw(GLFMDisplay *display) {
    TestPatternApp *app = glfmGetUserData(display);
    if (!app->textureNeedsUpdate && !app->needsRedraw) {
        return;
    }
    
    int width, height;
    glfmGetDisplaySize(display, &width, &height);

    if (app->textureNeedsUpdate && app->texture != NULL_TEXTURE) {
        app->renderer->textureDestroy(app->renderer, app->texture);
        app->texture = NULL_TEXTURE;
    }
    if (app->texture == NULL_TEXTURE) {
        app->texture = createTestPatternTexture(display, width, height);
        app->textureNeedsUpdate = false;
    }

    app->renderer->drawFrameStart(app->renderer, width, height);
    
    const Vertex vertices[4] = {
        { .position = { -1, -1 }, .texCoord = { 0, 0 } },
        { .position = {  1, -1 }, .texCoord = { 1, 0 } },
        { .position = { -1,  1 }, .texCoord = { 0, 1 } },
        { .position = {  1,  1 }, .texCoord = { 1, 1 } },
    };
    
    app->renderer->drawQuad(app->renderer, app->texture, &vertices);
    app->renderer->drawFrameEnd(app->renderer);
    glfmSwapBuffers(display);
    app->needsRedraw = false;
}

void glfmMain(GLFMDisplay *display) {
    TestPatternApp *app = calloc(1, sizeof(TestPatternApp));

    GLFMRenderingAPI renderingAPI = glfmIsMetalSupported(display) ? GLFMRenderingAPIMetal : GLFMRenderingAPIOpenGLES2;
    glfmSetDisplayConfig(display,
                         renderingAPI,
                         GLFMColorFormatRGBA8888,
                         GLFMDepthFormatNone,
                         GLFMStencilFormatNone,
                         GLFMMultisampleNone);

    glfmSetUserData(display, app);
    glfmSetDisplayChrome(display, GLFMUserInterfaceChromeNavigationAndStatusBar);
    glfmSetTouchFunc(display, onTouch);
    glfmSetSurfaceCreatedFunc(display, onSurfaceCreated);
    glfmSetSurfaceResizedFunc(display, onSurfaceResized);
    glfmSetSurfaceRefreshFunc(display, onSurfaceRefresh);
    glfmSetSurfaceDestroyedFunc(display, onSurfaceDestroyed);
    glfmSetOrientationChangedFunc(display, onOrientationChange);
    glfmSetDisplayChromeInsetsChangedFunc(display, onInsetsChange);
    glfmSetRenderFunc(display, onDraw);
}
