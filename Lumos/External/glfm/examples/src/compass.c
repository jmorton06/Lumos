// Draws a compass in 3D. Red is north. Requires a device with a compass/gyroscope (iOS/Android only).

#include <stdlib.h>
#include <math.h>
#include "glfm.h"
#include "file_compat.h"

#define FILE_COMPAT_ANDROID_ACTIVITY glfmGetAndroidActivity(display)

static const char *VERT_SHADER =
"#version 100\n"
"attribute highp vec3 a_position;"
"attribute lowp vec3 a_color;"
"varying lowp vec3 v_color;"
"void main() {"
"    gl_Position = vec4(a_position, 1);"
"    v_color = a_color;"
"}";

static const char *FRAG_SHADER =
"#version 100\n"
"varying lowp vec3 v_color;"
"void main() {"
"    gl_FragColor = vec4(v_color, 1);"
"}";

typedef struct {
    GLuint program;
    GLuint vertexBuffer;
    GLuint vertexArray;
    bool sensorDataReceived;
    struct {
        double m00, m01, m02;
        double m10, m11, m12;
        double m20, m21, m22;
    } rotation;
} CompassApp;

static void applyRotation(const CompassApp *app, GLfloat *x, GLfloat *y, GLfloat *z) {
    GLfloat x0 = *x;
    GLfloat y0 = *y;
    GLfloat z0 = *z;
    *x = app->rotation.m00 * x0 + app->rotation.m01 * y0 + app->rotation.m02 * z0;
    *y = app->rotation.m10 * x0 + app->rotation.m11 * y0 + app->rotation.m12 * z0;
    *z = app->rotation.m20 * x0 + app->rotation.m21 * y0 + app->rotation.m22 * z0;
}

static void drawCompass(CompassApp *app, int width, int height) {
    if (app->program == 0) {
        // Create shader
        app->program = glCreateProgram();

        GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        
        glShaderSource(vertShader, 1, &VERT_SHADER, NULL);
        glShaderSource(fragShader, 1, &FRAG_SHADER, NULL);
        
        glCompileShader(vertShader);
        glCompileShader(fragShader);

        glAttachShader(app->program, vertShader);
        glAttachShader(app->program, fragShader);

        glBindAttribLocation(app->program, 0, "a_position");
        glBindAttribLocation(app->program, 1, "a_color");

        glLinkProgram(app->program);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
    }
    
#if defined(GL_VERSION_3_0) && GL_VERSION_3_0
    if (app->vertexArray == 0) {
        glGenVertexArrays(1, &app->vertexArray);
    }
    glBindVertexArray(app->vertexArray);
#endif

    // Set up triangle attributes
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
    
    // Define triangle vertices
    // (X axis points North, Z axis is vertical)
    float sideLength = 0.25f;
    float x = sqrt(pow(sideLength, 2) - pow(sideLength / 2, 2)) / 2;
    float y = sideLength / 2;
    GLfloat vertices[] = {
        // x,y,z      r,g,b
        // North (red)
        0.5f + x,  0, 0,    1.0, 0.0, 0.0,
        0.5f - x,  y, 0,    1.0, 0.0, 0.0,
        0.5f - x, -y, 0,    1.0, 0.0, 0.0,
        
        // South
        -0.5f - x,  0, 0,   1.0, 1.0, 1.0,
        -0.5f + x, -y, 0,   1.0, 1.0, 1.0,
        -0.5f + x,  y, 0,   1.0, 1.0, 1.0,

        // West
         0, 0.5f + x, 0,    1.0, 1.0, 1.0,
        -y, 0.5f - x, 0,    1.0, 1.0, 1.0,
         y, 0.5f - x, 0,    1.0, 1.0, 1.0,
        
        // East
         0, -0.5f - x, 0,   1.0, 1.0, 1.0,
         y, -0.5f + x, 0,   1.0, 1.0, 1.0,
        -y, -0.5f + x, 0,   1.0, 1.0, 1.0,
    };
    
    // Transform triangle vertices
    int vertexCount = sizeof(vertices) / stride;
    for (int i = 0; i < vertexCount; i++) {
        GLfloat *t = vertices + i * stride / sizeof(GLfloat);
        applyRotation(app, t + 0,  t + 1,  t + 2);
        if (height < width) {
            float scaleX = (float)height / width;
            t[0] *= scaleX;
        } else {
            float scaleY = (float)width / height;
            t[1] *= scaleY;
        }
    }

    // Draw triangles
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

static void onDraw(GLFMDisplay *display) {
    CompassApp *app = glfmGetUserData(display);
    int width, height;
    glfmGetDisplaySize(display, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (app->sensorDataReceived) {
        drawCompass(app, width, height);
    }
    glfmSwapBuffers(display);
}

static void onSurfaceDestroyed(GLFMDisplay *display) {
    CompassApp *app = glfmGetUserData(display);
    app->program = 0;
    app->vertexBuffer = 0;
    app->vertexArray = 0;
}

static void onSensor(GLFMDisplay *display, GLFMSensorEvent event) {
    if (event.sensor == GLFMSensorRotationMatrix) {
        CompassApp *app = glfmGetUserData(display);
        app->sensorDataReceived = true;

        GLFMInterfaceOrientation orientation = glfmGetInterfaceOrientation(display);
        switch (orientation) {
            case GLFMInterfaceOrientationPortrait: default:
                app->rotation.m00 = event.matrix.m00;
                app->rotation.m01 = event.matrix.m01;
                app->rotation.m02 = event.matrix.m02;
                app->rotation.m10 = event.matrix.m10;
                app->rotation.m11 = event.matrix.m11;
                app->rotation.m12 = event.matrix.m12;
                app->rotation.m20 = event.matrix.m20;
                app->rotation.m21 = event.matrix.m21;
                app->rotation.m22 = event.matrix.m22;
                break;
            case GLFMInterfaceOrientationLandscapeLeft: // Rotate Z 90 degrees
                app->rotation.m00 = event.matrix.m10;
                app->rotation.m01 = event.matrix.m11;
                app->rotation.m02 = event.matrix.m12;
                app->rotation.m10 = -event.matrix.m00;
                app->rotation.m11 = -event.matrix.m01;
                app->rotation.m12 = -event.matrix.m02;
                app->rotation.m20 = event.matrix.m20;
                app->rotation.m21 = event.matrix.m21;
                app->rotation.m22 = event.matrix.m22;
                break;
            case GLFMInterfaceOrientationPortraitUpsideDown: // Rotate Z 180 degrees
                app->rotation.m00 = -event.matrix.m00;
                app->rotation.m01 = -event.matrix.m01;
                app->rotation.m02 = -event.matrix.m02;
                app->rotation.m10 = -event.matrix.m10;
                app->rotation.m11 = -event.matrix.m11;
                app->rotation.m12 = -event.matrix.m12;
                app->rotation.m20 = event.matrix.m20;
                app->rotation.m21 = event.matrix.m21;
                app->rotation.m22 = event.matrix.m22;
                break;
            case GLFMInterfaceOrientationLandscapeRight: // Rotate Z -90 degrees
                app->rotation.m00 = -event.matrix.m10;
                app->rotation.m01 = -event.matrix.m11;
                app->rotation.m02 = -event.matrix.m12;
                app->rotation.m10 = event.matrix.m00;
                app->rotation.m11 = event.matrix.m01;
                app->rotation.m12 = event.matrix.m02;
                app->rotation.m20 = event.matrix.m20;
                app->rotation.m21 = event.matrix.m21;
                app->rotation.m22 = event.matrix.m22;
                break;
        }
    }
}

void glfmMain(GLFMDisplay *display) {
    CompassApp *app = calloc(1, sizeof(CompassApp));

    glfmSetDisplayConfig(display,
                         GLFMRenderingAPIOpenGLES2,
                         GLFMColorFormatRGBA8888,
                         GLFMDepthFormatNone,
                         GLFMStencilFormatNone,
                         GLFMMultisampleNone);
    glfmSetUserData(display, app);
    glfmSetSurfaceDestroyedFunc(display, onSurfaceDestroyed);
    glfmSetRenderFunc(display, onDraw);
    
    if (glfmIsSensorAvailable(display, GLFMSensorRotationMatrix)) {
        // Enable sensor for the device's rotation matrix. To disable, set the callback function to NULL.
        glfmSetSensorFunc(display, GLFMSensorRotationMatrix, onSensor);
    } else {
        printf("Warning: Rotation sensor not available on this device.\n");
        // North points up
        app->sensorDataReceived = true;
        app->rotation.m00 = 0.0f;
        app->rotation.m01 = -1.0f;
        app->rotation.m02 = 0.0f;
        app->rotation.m10 = 1.0f;
        app->rotation.m11 = 0.0f;
        app->rotation.m12 = 0.0f;
        app->rotation.m20 = 0.0f;
        app->rotation.m21 = 0.0f;
        app->rotation.m22 = 1.0f;
    }
}
