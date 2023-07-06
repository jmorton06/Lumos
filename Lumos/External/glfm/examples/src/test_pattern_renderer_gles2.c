#include "test_pattern_renderer.h"

#include <stdlib.h>
#include <stddef.h>
#include "file_compat.h"

#define FILE_COMPAT_ANDROID_ACTIVITY glfmGetAndroidActivity(display)

typedef struct {
    Renderer renderer;
    GLuint textureProgram;
    GLuint textureVertexBuffer;
    GLuint textureVertexArray;
} RendererGLES2;

#define impl_of(this_renderer) ((RendererGLES2 *)(void *)((uint8_t *)this_renderer - offsetof(RendererGLES2, renderer)))

static Texture textureUpload(Renderer *renderer, uint32_t width, uint32_t height, uint8_t *data) {
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return textureId;
}

static void textureDestroy(Renderer *renderer, Texture texture) {
    if (texture != NULL_TEXTURE) {
        GLuint textureId = (GLuint)texture;
        glDeleteTextures(1, &textureId);
    }
}

static void drawFrameStart(Renderer *renderer, int screenWidth, int screenHeight) {
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void drawFrameEnd(Renderer *renderer) {
    // Do nothing
}

static void drawQuad(Renderer *renderer, Texture texture, const Vertex (*vertices)[4]) {
    // NOTE: This function draws one quad at a time, which is slow. Don't use in production.
    RendererGLES2 *impl = impl_of(renderer);
    glUseProgram(impl->textureProgram);

#if defined(GL_VERSION_3_0) && GL_VERSION_3_0
    glBindVertexArray(impl->textureVertexArray);
#endif

    glBindBuffer(GL_ARRAY_BUFFER, impl->textureVertexBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texCoord));
    
    GLuint textureId = (GLuint)texture;
    glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices), vertices, GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void destroy(Renderer *renderer) {
    RendererGLES2 *impl = impl_of(renderer);
    free(impl);
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
        size_t length = (size_t)ftell(shaderFile);
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
            GLchar *log = malloc((size_t)logLength);
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

Renderer *createRendererGLES2(GLFMDisplay *display) {
    RendererGLES2 *impl = calloc(1, sizeof(RendererGLES2));
    
    GLuint vertShader = compileShader(display, GL_VERTEX_SHADER, "texture.vert");
    GLuint fragShader = compileShader(display, GL_FRAGMENT_SHADER, "texture.frag");
    if (vertShader != 0 && fragShader != 0) {
        impl->textureProgram = glCreateProgram();
        
        glAttachShader(impl->textureProgram, vertShader);
        glAttachShader(impl->textureProgram, fragShader);
        
        glBindAttribLocation(impl->textureProgram, 0, "position");
        glBindAttribLocation(impl->textureProgram, 1, "texCoord");
        
        glLinkProgram(impl->textureProgram);
        
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
    }
    
    glGenBuffers(1, &impl->textureVertexBuffer);
    
#if defined(GL_VERSION_3_0) && GL_VERSION_3_0
    glGenVertexArrays(1, &impl->textureVertexArray);
#endif
    
    Renderer *renderer = &impl->renderer;
    renderer->textureUpload = textureUpload;
    renderer->textureDestroy = textureDestroy;
    renderer->drawFrameStart = drawFrameStart;
    renderer->drawFrameEnd = drawFrameEnd;
    renderer->drawQuad = drawQuad;
    renderer->destroy = destroy;
    return renderer;
}
