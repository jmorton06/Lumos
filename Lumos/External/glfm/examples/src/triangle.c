// Basic triangle example
#include "glfm.h"

static GLint program = 0;
static GLuint vertexBuffer = 0;
static GLuint vertexArray = 0;

static void onDraw(GLFMDisplay *display);
static void onSurfaceDestroyed(GLFMDisplay *display);

void glfmMain(GLFMDisplay *display) {
    glfmSetDisplayConfig(display,
                         GLFMRenderingAPIOpenGLES2,
                         GLFMColorFormatRGBA8888,
                         GLFMDepthFormatNone,
                         GLFMStencilFormatNone,
                         GLFMMultisampleNone);
    glfmSetRenderFunc(display, onDraw);
    glfmSetSurfaceDestroyedFunc(display, onSurfaceDestroyed);
}

static void onSurfaceDestroyed(GLFMDisplay *display) {
    // When the surface is destroyed, all existing GL resources are no longer valid.
    program = 0;
    vertexBuffer = 0;
    vertexArray = 0;
}

static GLuint compileShader(const GLenum type, const GLchar *shaderString, GLint shaderLength) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderString, &shaderLength);
    glCompileShader(shader);
    return shader;
}

static void onDraw(GLFMDisplay *display) {
    if (program == 0) {
        const GLchar vertexShader[] =
            "#version 100\n"
            "attribute highp vec4 position;\n"
            "void main() {\n"
            "   gl_Position = position;\n"
            "}";

        const GLchar fragmentShader[] =
            "#version 100\n"
            "void main() {\n"
            "  gl_FragColor = vec4(0.85, 0.80, 0.75, 1.0);\n"
            "}";

        program = glCreateProgram();
        GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertexShader, sizeof(vertexShader) - 1);
        GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragmentShader, sizeof(fragmentShader) - 1);

        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);

        glLinkProgram(program);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
    }
    if (vertexBuffer == 0) {
        const GLfloat vertices[] = {
             0.0,  0.5, 0.0,
            -0.5, -0.5, 0.0,
             0.5, -0.5, 0.0,
        };
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }

    int width, height;
    glfmGetDisplaySize(display, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.08f, 0.07f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

#if defined(GL_VERSION_3_0) && GL_VERSION_3_0
    if (vertexArray == 0) {
        glGenVertexArrays(1, &vertexArray);
    }
    glBindVertexArray(vertexArray);
#endif

    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfmSwapBuffers(display);
}
