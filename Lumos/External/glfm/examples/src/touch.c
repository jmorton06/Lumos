// Example app that draws a triangle. The triangle can be moved via touch or keyboard arrow keys.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glfm.h"
#include "file_compat.h"

#define FILE_COMPAT_ANDROID_ACTIVITY glfmGetAndroidActivity(display)

typedef struct {
    GLuint program;
    GLuint vertexBuffer;
    GLuint vertexArray;

    double lastTouchX;
    double lastTouchY;

    double offsetX;
    double offsetY;

    bool needsRedraw;
} ExampleApp;

static void onDraw(GLFMDisplay *display);
static void onSurfaceCreated(GLFMDisplay *display, int width, int height);
static void onSurfaceRefresh(GLFMDisplay *display);
static void onSurfaceDestroyed(GLFMDisplay *display);
static bool onTouch(GLFMDisplay *display, int touch, GLFMTouchPhase phase, double x, double y);
static bool onKey(GLFMDisplay *display, GLFMKeyCode keyCode, GLFMKeyAction action, int modifiers);
static bool onScroll(GLFMDisplay *display, double x, double y, GLFMMouseWheelDeltaType deltaType,
                     double deltaX, double deltaY, double deltaZ);

// Main entry point
void glfmMain(GLFMDisplay *display) {
    ExampleApp *app = calloc(1, sizeof(ExampleApp));

    glfmSetDisplayConfig(display,
                         GLFMRenderingAPIOpenGLES2,
                         GLFMColorFormatRGBA8888,
                         GLFMDepthFormatNone,
                         GLFMStencilFormatNone,
                         GLFMMultisampleNone);
    glfmSetUserData(display, app);
    glfmSetSurfaceCreatedFunc(display, onSurfaceCreated);
    glfmSetSurfaceRefreshFunc(display, onSurfaceRefresh);
    glfmSetSurfaceDestroyedFunc(display, onSurfaceDestroyed);
    glfmSetRenderFunc(display, onDraw);
    glfmSetTouchFunc(display, onTouch);
    glfmSetKeyFunc(display, onKey);
    glfmSetMouseWheelFunc(display, onScroll);
}

static bool onTouch(GLFMDisplay *display, int touch, GLFMTouchPhase phase, double x, double y) {
    if (phase == GLFMTouchPhaseHover) {
        return false;
    }
    ExampleApp *app = glfmGetUserData(display);
    app->needsRedraw = true;
    if (phase != GLFMTouchPhaseBegan) {
        int width, height;
        glfmGetDisplaySize(display, &width, &height);
        app->offsetX += 2 * (x - app->lastTouchX) / width;
        app->offsetY -= 2 * (y - app->lastTouchY) / height;
    }
    app->lastTouchX = x;
    app->lastTouchY = y;
    return true;
}

static bool onKey(GLFMDisplay *display, GLFMKeyCode keyCode, GLFMKeyAction action, int modifiers) {
    bool handled = false;
    if (action == GLFMKeyActionPressed) {
        ExampleApp *app = glfmGetUserData(display);
        switch (keyCode) {
            case GLFMKeyCodeArrowLeft:
                app->offsetX -= 0.1f;
                handled = true;
                break;
            case GLFMKeyCodeArrowRight:
                app->offsetX += 0.1f;
                handled = true;
                break;
            case GLFMKeyCodeArrowUp:
                app->offsetY += 0.1f;
                handled = true;
                break;
            case GLFMKeyCodeArrowDown:
                app->offsetY -= 0.1f;
                handled = true;
                break;
            case GLFMKeyCodeSpace:
            case GLFMKeyCodeEnter:
            case GLFMKeyCodeBackspace:
                app->offsetX = 0.0f;
                app->offsetY = 0.0f;
                handled = true;
            default:
                break;
        }
        app->needsRedraw |= handled;
    }
    return handled;
}

static bool onScroll(GLFMDisplay *display, double x, double y, GLFMMouseWheelDeltaType deltaType,
                     double deltaX, double deltaY, double deltaZ) {
    ExampleApp *app = glfmGetUserData(display);
    int width, height;
    glfmGetDisplaySize(display, &width, &height);
    if (deltaType != GLFMMouseWheelDeltaPixel) {
        deltaX *= 20;
        deltaY *= 20;
    }
    app->offsetX -= deltaX / width;
    app->offsetY += deltaY / height;
    app->needsRedraw = true;
    return true;
}

static void onSurfaceCreated(GLFMDisplay *display, int width, int height) {
    GLFMRenderingAPI api = glfmGetRenderingAPI(display);
    printf("Hello from GLFM! Using OpenGL %s\n",
           api == GLFMRenderingAPIOpenGLES32 ? "ES 3.2" :
           api == GLFMRenderingAPIOpenGLES31 ? "ES 3.1" :
           api == GLFMRenderingAPIOpenGLES3 ? "ES 3.0" : "ES 2.0");
}

static void onSurfaceRefresh(GLFMDisplay *display) {
    ExampleApp *app = glfmGetUserData(display);
    app->needsRedraw = true;
}

static void onSurfaceDestroyed(GLFMDisplay *display) {
    // When the surface is destroyed, all existing GL resources are no longer valid.
    ExampleApp *app = glfmGetUserData(display);
    app->program = 0;
    app->vertexBuffer = 0;
    app->vertexArray = 0;
    printf("Goodbye\n");
}

static GLuint compileShader(GLFMDisplay *display, GLenum type, const char *shaderName) {
    char fullPath[PATH_MAX];
    fc_resdir(fullPath, sizeof(fullPath));
    strncat(fullPath, shaderName, sizeof(fullPath) - strlen(fullPath) - 1);

    // Get shader string
    char *shaderString = NULL;
    FILE *shaderFile = fopen(fullPath, "rb");
    if (shaderFile) {
        fseek(shaderFile, 0, SEEK_END);
        long length = ftell(shaderFile);
        fseek(shaderFile, 0, SEEK_SET);

        shaderString = malloc(length + 1);
        if (shaderString) {
            fread(shaderString, length, 1, shaderFile);
            shaderString[length] = 0;
        }
        fclose(shaderFile);
    }
    if (!shaderString) {
        printf("Couldn't read file: %s\n", fullPath);
        return 0;
    }

    // Compile
    const char *constShaderString = shaderString;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &constShaderString, NULL);
    glCompileShader(shader);
    free(shaderString);

    // Check compile status
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        printf("Couldn't compile shader: %s\n", shaderName);
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLchar *log = malloc(logLength);
            glGetShaderInfoLog(shader, logLength, &logLength, log);
            if (log[0] != 0) {
                printf("Shader log: %s\n", log);
            }
            free(log);
        }
        glDeleteShader(shader);
        shader = 0;
    }
    return shader;
}

static void draw(GLFMDisplay *display, int width, int height) {
    ExampleApp *app = glfmGetUserData(display);

    // Create shader
    if (app->program == 0) {
        GLuint vertShader = compileShader(display, GL_VERTEX_SHADER, "simple.vert");
        GLuint fragShader = compileShader(display, GL_FRAGMENT_SHADER, "simple.frag");
        if (vertShader == 0 || fragShader == 0) {
            return;
        }
        app->program = glCreateProgram();

        glAttachShader(app->program, vertShader);
        glAttachShader(app->program, fragShader);

        glBindAttribLocation(app->program, 0, "a_position");
        glBindAttribLocation(app->program, 1, "a_color");

        glLinkProgram(app->program);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
    }
    
    // Draw background
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
#if defined(GL_VERSION_3_0) && GL_VERSION_3_0
    if (app->vertexArray == 0) {
        glGenVertexArrays(1, &app->vertexArray);
    }
    glBindVertexArray(app->vertexArray);
#endif
    
    // Draw triangle
    glUseProgram(app->program);
    if (app->vertexBuffer == 0) {
        glGenBuffers(1, &app->vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, app->vertexBuffer);
    const size_t stride = sizeof(GLfloat) * 6;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(GLfloat) * 3));

    const GLfloat vertices[] = {
        // x,y,z, r,g,b
        app->offsetX + 0.0f, app->offsetY + 0.5f, 0.0,  1.0, 0.0, 0.0,
        app->offsetX - 0.5f, app->offsetY - 0.5f, 0.0,  0.0, 1.0, 0.0,
        app->offsetX + 0.5f, app->offsetY - 0.5f, 0.0,  0.0, 0.0, 1.0,
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

static void onDraw(GLFMDisplay *display) {
    ExampleApp *app = glfmGetUserData(display);
    if (app->needsRedraw) {
        app->needsRedraw = false;
        
        int width, height;
        glfmGetDisplaySize(display, &width, &height);
        draw(display, width,  height);
        glfmSwapBuffers(display);
    }
}
