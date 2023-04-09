// GLFM
// https://github.com/brackeen/glfm

#if defined(__ANDROID__)

#include "glfm.h"
#include "glfm_internal.h"

#include <EGL/egl.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/sensor.h>
#include <android/window.h>
#include <assert.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#define GLFM_LOG_LIFECYCLE_ENABLE 0

#ifdef NDEBUG
#  define GLFM_LOG(...) do { } while (0)
#  define GLFM_LOG_LIFECYCLE(...) do { } while (0)
#else
#  include <android/log.h>
#  define GLFM_LOG(...) __android_log_print(ANDROID_LOG_DEBUG, "GLFM", __VA_ARGS__)
#  if GLFM_LOG_LIFECYCLE_ENABLE
#    define GLFM_LOG_LIFECYCLE(...) __android_log_print(ANDROID_LOG_VERBOSE, "GLFM", "Lifecycle: " __VA_ARGS__)
#  else
#    define GLFM_LOG_LIFECYCLE(...) do { } while (0)
#  endif
#endif

#define GLFM_MAX_SIMULTANEOUS_TOUCHES 5
// Same update interval as iOS
#define GLFM_SENSOR_UPDATE_INTERVAL_MICROS ((int)(0.01 * 1000000))
#define GLFM_RESIZE_EVENT_MAX_WAIT_FRAMES 5

// If GLFM_HANDLE_BACK_BUTTON is 1, when the user presses the back button, the task is moved to the
// back. Otherwise, when the user presses the back button, the activity is destroyed.
// On newer API levels (31) this may not be needed.
#define GLFM_HANDLE_BACK_BUTTON 1

// MARK: - Platform data (global singleton)

typedef struct {
    ALooper *looper;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int commandPipeRead;
    int commandPipeWrite;
    bool threadRunning;

    ALooper *uiLooper;
    int uiCommandPipeRead;
    int uiCommandPipeWrite;

    ANativeWindow *window;
    AInputQueue *inputQueue;
    ARect contentRectArray[2];
    int contentRectIndex;

    ANativeWindow *pendingWindow;
    AInputQueue *pendingInputQueue;

    ANativeActivity *activity;
    AConfiguration *config;
    bool destroyRequested;

    bool multitouchEnabled;

    ARect keyboardFrame;
    bool keyboardVisible;

    bool animating;
    bool refreshRequested;
    bool swapCalled;
    bool surfaceCreatedNotified;
    double lastSwapTime;

    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    EGLConfig eglConfig;
    EGLContext eglContext;
    bool eglContextCurrent;

    int32_t width;
    int32_t height;
    double scale;
    int resizeEventWaitFrames;

    struct {
        int top, right, bottom, left;
        bool valid;
    } insets;

    GLFMDisplay *display;
    GLFMRenderingAPI renderingAPI;

    ASensorEventQueue *sensorEventQueue;
    GLFMSensorEvent sensorEvent[GLFM_NUM_SENSORS];
    bool sensorEventValid[GLFM_NUM_SENSORS];
    bool deviceSensorEnabled[GLFM_NUM_SENSORS];

    GLFMInterfaceOrientation orientation;

    JNIEnv *jniEnv;
} GLFMPlatformData;

static GLFMPlatformData *platformDataGlobal = NULL;

// MARK: - Private function declarations

static void *glfm__mainLoop(void *param);
static int glfm__looperCallback(int fd, int events, void *userData);
static void glfm__setAllRequestedSensorsEnabled(GLFMDisplay *display, bool enable);
static void glfm__reportOrientationChangeIfNeeded(GLFMDisplay *display);
static void glfm__reportInsetsChangedIfNeeded(GLFMDisplay *display);
static bool glfm__updateSurfaceSizeIfNeeded(GLFMDisplay *display, bool force);
static float glfm__getRefreshRate(const GLFMDisplay *display);
static void glfm__getDisplayChromeInsets(const GLFMDisplay *display, int *top, int *right,
                                         int *bottom, int *left);
static void glfm__resetContentRect(GLFMPlatformData *platformData);
static void glfm__updateKeyboardVisibility(GLFMPlatformData *platformData);
static void glfm__updateUserInterfaceChrome(GLFMPlatformData *platformData);

// MARK: - JNI code

#ifdef NDEBUG
#  define glfm__printException(jni) ((void)0)
#else
#  define glfm__printException(jni) (*(jni))->ExceptionDescribe(jni)
#endif

#define glfm__wasJavaExceptionThrown(jni) \
    ((*(jni))->ExceptionCheck(jni) ? (glfm__printException(jni), (*(jni))->ExceptionClear(jni), true) : false)

#define glfm__clearJavaException(jni) \
    do { \
        if ((*(jni))->ExceptionCheck(jni)) { \
            glfm__printException(jni); \
            (*(jni))->ExceptionClear(jni); \
        } \
    } while (0)

static jmethodID glfm__getJavaMethodID(JNIEnv *jni, jobject object, const char *name, const char *sig) {
    if (object) {
        jclass class = (*jni)->GetObjectClass(jni, object);
        jmethodID methodID = (*jni)->GetMethodID(jni, class, name, sig);
        (*jni)->DeleteLocalRef(jni, class);
        return glfm__wasJavaExceptionThrown(jni) ? NULL : methodID;
    } else {
        return NULL;
    }
}

static jfieldID glfm__getJavaFieldID(JNIEnv *jni, jobject object, const char *name, const char *sig) {
    if (object) {
        jclass class = (*jni)->GetObjectClass(jni, object);
        jfieldID fieldID = (*jni)->GetFieldID(jni, class, name, sig);
        (*jni)->DeleteLocalRef(jni, class);
        return glfm__wasJavaExceptionThrown(jni) ? NULL : fieldID;
    } else {
        return NULL;
    }
}

static jmethodID glfm__getJavaStaticMethodID(JNIEnv *jni, jclass class, const char *name, const char *sig) {
    if (class) {
        jmethodID methodID = (*jni)->GetStaticMethodID(jni, class, name, sig);
        return glfm__wasJavaExceptionThrown(jni) ? NULL : methodID;
    } else {
        return NULL;
    }
}

static jfieldID glfm__getJavaStaticFieldID(JNIEnv *jni, jclass class, const char *name, const char *sig) {
    if (class) {
        jfieldID fieldID = (*jni)->GetStaticFieldID(jni, class, name, sig);
        return glfm__wasJavaExceptionThrown(jni) ? NULL : fieldID;
    } else {
        return NULL;
    }
}

#define glfm__callJavaMethod(jni, object, methodName, methodSig, returnType) \
    (*(jni))->Call##returnType##Method(jni, object, \
        glfm__getJavaMethodID(jni, object, methodName, methodSig))

#define glfm__callJavaMethodWithArgs(jni, object, methodName, methodSig, returnType, ...) \
    (*(jni))->Call##returnType##Method(jni, object, \
        glfm__getJavaMethodID(jni, object, methodName, methodSig), __VA_ARGS__)

#define glfm__callJavaStaticMethod(jni, class, methodName, methodSig, returnType) \
    (*(jni))->CallStatic##returnType##Method(jni, class, \
        glfm__getJavaStaticMethodID(jni, class, methodName, methodSig))

#define glfm__callJavaStaticMethodWithArgs(jni, class, methodName, methodSig, returnType, ...) \
    (*(jni))->CallStatic##returnType##Method(jni, class, \
        glfm__getJavaStaticMethodID(jni, class, methodName, methodSig), __VA_ARGS__)

#define glfm__getJavaField(jni, object, fieldName, fieldSig, fieldType) \
    (*(jni))->Get##fieldType##Field(jni, object, \
        glfm__getJavaFieldID(jni, object, fieldName, fieldSig))

#define glfm__getJavaStaticField(jni, class, fieldName, fieldSig, fieldType) \
    (*(jni))->GetStatic##fieldType##Field(jni, class, \
        glfm__getJavaStaticFieldID(jni, class, fieldName, fieldSig))

// MARK: - EGL

static bool glfm__eglContextInit(GLFMPlatformData *platformData) {

    // Available in eglext.h in API 18
    static const int EGL_CONTEXT_MAJOR_VERSION_KHR = 0x3098;
    static const int EGL_CONTEXT_MINOR_VERSION_KHR = 0x30FB;

    EGLint majorVersion = 0;
    EGLint minorVersion = 0;
    bool created = false;
    if (platformData->eglContext == EGL_NO_CONTEXT) {
        // OpenGL ES 3.2
        if (platformData->display->preferredAPI >= GLFMRenderingAPIOpenGLES32) {
            majorVersion = 3;
            minorVersion = 2;
            const EGLint contextAttribList[] = { EGL_CONTEXT_MAJOR_VERSION_KHR, majorVersion,
                                                 EGL_CONTEXT_MINOR_VERSION_KHR, minorVersion,
                                                 EGL_NONE, EGL_NONE };
            platformData->eglContext = eglCreateContext(platformData->eglDisplay,
                                                        platformData->eglConfig,
                                                        EGL_NO_CONTEXT,
                                                        contextAttribList);
            created = platformData->eglContext != EGL_NO_CONTEXT;
        }
        // OpenGL ES 3.1
        if (!created && platformData->display->preferredAPI >= GLFMRenderingAPIOpenGLES31) {
            majorVersion = 3;
            minorVersion = 1;
            const EGLint contextAttribList[] = { EGL_CONTEXT_MAJOR_VERSION_KHR, majorVersion,
                                                 EGL_CONTEXT_MINOR_VERSION_KHR, minorVersion,
                                                 EGL_NONE, EGL_NONE };
            platformData->eglContext = eglCreateContext(platformData->eglDisplay,
                                                        platformData->eglConfig,
                                                        EGL_NO_CONTEXT,
                                                        contextAttribList);
            created = platformData->eglContext != EGL_NO_CONTEXT;
        }
        // OpenGL ES 3.0
        if (!created && platformData->display->preferredAPI >= GLFMRenderingAPIOpenGLES3) {
            majorVersion = 3;
            minorVersion = 0;
            const EGLint contextAttribList[] = { EGL_CONTEXT_CLIENT_VERSION, majorVersion,
                                                 EGL_NONE, EGL_NONE };
            platformData->eglContext = eglCreateContext(platformData->eglDisplay,
                                                        platformData->eglConfig,
                                                        EGL_NO_CONTEXT,
                                                        contextAttribList);
            created = platformData->eglContext != EGL_NO_CONTEXT;
        }
        // OpenGL ES 2.0
        if (!created) {
            majorVersion = 2;
            minorVersion = 0;
            const EGLint contextAttribList[] = { EGL_CONTEXT_CLIENT_VERSION, majorVersion,
                                                 EGL_NONE, EGL_NONE };
            platformData->eglContext = eglCreateContext(platformData->eglDisplay,
                                                        platformData->eglConfig,
                                                        EGL_NO_CONTEXT,
                                                        contextAttribList);
            created = platformData->eglContext != EGL_NO_CONTEXT;
        }

        if (created) {
            eglQueryContext(platformData->eglDisplay, platformData->eglContext,
                            EGL_CONTEXT_MAJOR_VERSION_KHR, &majorVersion);
            if (majorVersion >= 3) { 
                // This call fails on many devices.
                // When it fails, `minorVersion` is left unchanged.
                eglQueryContext(platformData->eglDisplay, platformData->eglContext,
                                EGL_CONTEXT_MINOR_VERSION_KHR, &minorVersion);
            }
            if (majorVersion == 3 && minorVersion == 2) {
                platformData->renderingAPI = GLFMRenderingAPIOpenGLES32;
            } else if (majorVersion == 3 && minorVersion == 1) {
                platformData->renderingAPI = GLFMRenderingAPIOpenGLES31;
            } else if (majorVersion == 3) {
                platformData->renderingAPI = GLFMRenderingAPIOpenGLES3;
            } else {
                platformData->renderingAPI = GLFMRenderingAPIOpenGLES2;
            }
        }
    }

    if (!eglMakeCurrent(platformData->eglDisplay, platformData->eglSurface,
                        platformData->eglSurface, platformData->eglContext)) {
        GLFM_LOG_LIFECYCLE("eglMakeCurrent() failed");
        platformData->eglContextCurrent = false;
        return false;
    } else {
        platformData->eglContextCurrent = true;
        GLFM_LOG_LIFECYCLE("GL Context made current");
        if (created && !platformData->surfaceCreatedNotified) {
            platformData->surfaceCreatedNotified = true;
            if (platformData->display && platformData->display->surfaceCreatedFunc) {
                platformData->display->surfaceCreatedFunc(platformData->display,
                                                          platformData->width,
                                                          platformData->height);
            }
        }
        return true;
    }
}

static void glfm__eglContextDisable(GLFMPlatformData *platformData) {
    if (platformData->eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(platformData->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    platformData->eglContextCurrent = false;
}

static void glfm__eglSurfaceInit(GLFMPlatformData *platformData) {
    if (platformData->eglSurface == EGL_NO_SURFACE) {
        platformData->eglSurface = eglCreateWindowSurface(platformData->eglDisplay,
                                                          platformData->eglConfig,
                                                          platformData->window, NULL);

        switch (platformData->display->swapBehavior) {
        case GLFMSwapBehaviorPlatformDefault:
            // Platform default, do nothing.
            break;
        case GLFMSwapBehaviorBufferPreserved:
            eglSurfaceAttrib(platformData->eglDisplay, platformData->eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
            break;
        case GLFMSwapBehaviorBufferDestroyed:
            eglSurfaceAttrib(platformData->eglDisplay, platformData->eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED);
        }
    }
}

#ifndef NDEBUG

static void glfm__eglLogConfig(GLFMPlatformData *platformData, EGLConfig config) {
    GLFM_LOG("Config: %p", config);
    EGLint value;
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_RENDERABLE_TYPE, &value);
    GLFM_LOG("  EGL_RENDERABLE_TYPE %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_SURFACE_TYPE, &value);
    GLFM_LOG("  EGL_SURFACE_TYPE    %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_RED_SIZE, &value);
    GLFM_LOG("  EGL_RED_SIZE        %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_GREEN_SIZE, &value);
    GLFM_LOG("  EGL_GREEN_SIZE      %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_BLUE_SIZE, &value);
    GLFM_LOG("  EGL_BLUE_SIZE       %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_ALPHA_SIZE, &value);
    GLFM_LOG("  EGL_ALPHA_SIZE      %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_DEPTH_SIZE, &value);
    GLFM_LOG("  EGL_DEPTH_SIZE      %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_STENCIL_SIZE, &value);
    GLFM_LOG("  EGL_STENCIL_SIZE    %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_SAMPLE_BUFFERS, &value);
    GLFM_LOG("  EGL_SAMPLE_BUFFERS  %i", value);
    eglGetConfigAttrib(platformData->eglDisplay, config, EGL_SAMPLES, &value);
    GLFM_LOG("  EGL_SAMPLES         %i", value);
}

#endif

static bool glfm__eglInit(GLFMPlatformData *platformData) {
    if (platformData->eglDisplay != EGL_NO_DISPLAY) {
        glfm__eglSurfaceInit(platformData);
        return glfm__eglContextInit(platformData);
    }
    int rBits, gBits, bBits, aBits;
    int depthBits, stencilBits, samples;

    switch (platformData->display->colorFormat) {
        case GLFMColorFormatRGB565:
            rBits = 5;
            gBits = 6;
            bBits = 5;
            aBits = 0;
            break;
        case GLFMColorFormatRGBA8888:
        default:
            rBits = 8;
            gBits = 8;
            bBits = 8;
            aBits = 8;
            break;
    }

    switch (platformData->display->depthFormat) {
        case GLFMDepthFormatNone:
        default:
            depthBits = 0;
            break;
        case GLFMDepthFormat16:
            depthBits = 16;
            break;
        case GLFMDepthFormat24:
            depthBits = 24;
            break;
    }

    switch (platformData->display->stencilFormat) {
        case GLFMStencilFormatNone:
        default:
            stencilBits = 0;
            break;
        case GLFMStencilFormat8:
            stencilBits = 8;
            if (depthBits > 0) {
                // Many implementations only allow 24-bit depth with 8-bit stencil.
                depthBits = 24;
            }
            break;
    }

    samples = platformData->display->multisample == GLFMMultisample4X ? 4 : 0;

    EGLint majorVersion;
    EGLint minorVersion;
    EGLint format;
    EGLint numConfigs;

    platformData->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(platformData->eglDisplay, &majorVersion, &minorVersion);

    while (true) {
        const EGLint attribList[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, rBits,
            EGL_GREEN_SIZE, gBits,
            EGL_BLUE_SIZE, bBits,
            EGL_ALPHA_SIZE, aBits,
            EGL_DEPTH_SIZE, depthBits,
            EGL_STENCIL_SIZE, stencilBits,
            EGL_SAMPLE_BUFFERS, samples > 0 ? 1 : 0,
            EGL_SAMPLES, samples > 0 ? samples : 0,
            EGL_NONE, EGL_NONE
        };
        eglChooseConfig(platformData->eglDisplay, attribList,
                        &platformData->eglConfig, 1, &numConfigs);
        if (numConfigs) {
            // Found!
            //glfm__eglLogConfig(platformData, platformData->eglConfig);
            break;
        } else if (samples > 0) {
            // Try 2x multisampling or no multisampling
            samples -= 2;
        } else if (depthBits > 8) {
            // Try 16-bit depth or 8-bit depth
            depthBits -= 8;
        } else {
            // Failure
#ifndef NDEBUG
            static bool printedConfigs = false;
            if (!printedConfigs) {
                printedConfigs = true;
                GLFM_LOG("eglChooseConfig() failed");
                EGLConfig configs[256];
                EGLint numTotalConfigs;
                if (eglGetConfigs(platformData->eglDisplay, configs, 256, &numTotalConfigs)) {
                    GLFM_LOG("Num available configs: %i", numTotalConfigs);
                    int i;
                    for (i = 0; i < numTotalConfigs; i++) {
                        glfm__eglLogConfig(platformData, configs[i]);
                    }
                } else {
                    GLFM_LOG("Couldn't get any EGL configs");
                }
            }
#endif

            GLFM_LOG("eglChooseConfig() failed");
            glfm__reportSurfaceError(platformData->eglDisplay, "eglChooseConfig() failed");
            eglTerminate(platformData->eglDisplay);
            platformData->eglDisplay = EGL_NO_DISPLAY;
            return false;
        }
    }

    glfm__eglSurfaceInit(platformData);

    eglQuerySurface(platformData->eglDisplay, platformData->eglSurface, EGL_WIDTH,
                    &platformData->width);
    eglQuerySurface(platformData->eglDisplay, platformData->eglSurface, EGL_HEIGHT,
                    &platformData->height);
    eglGetConfigAttrib(platformData->eglDisplay, platformData->eglConfig, EGL_NATIVE_VISUAL_ID,
                       &format);

    ANativeWindow_setBuffersGeometry(platformData->window, 0, 0, format);

    return glfm__eglContextInit(platformData);
}

static void glfm__eglSurfaceDestroy(GLFMPlatformData *platformData) {
    if (platformData->eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(platformData->eglDisplay, platformData->eglSurface);
        platformData->eglSurface = EGL_NO_SURFACE;
    }
    glfm__eglContextDisable(platformData);
}

static void glfm__eglDestroy(GLFMPlatformData *platformData) {
    if (platformData->eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(platformData->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (platformData->eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(platformData->eglDisplay, platformData->eglContext);
            GLFM_LOG_LIFECYCLE("GL Context destroyed");
            if (platformData->surfaceCreatedNotified) {
                platformData->surfaceCreatedNotified = false;
                if (platformData->display && platformData->display->surfaceDestroyedFunc) {
                    platformData->display->surfaceDestroyedFunc(platformData->display);
                }
            }
        }
        if (platformData->eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(platformData->eglDisplay, platformData->eglSurface);
        }
        eglTerminate(platformData->eglDisplay);
    }
    platformData->eglDisplay = EGL_NO_DISPLAY;
    platformData->eglContext = EGL_NO_CONTEXT;
    platformData->eglSurface = EGL_NO_SURFACE;
    platformData->eglContextCurrent = false;
}

static void glfm__eglCheckError(GLFMPlatformData *platformData) {
    EGLint err = eglGetError();
    if (err == EGL_BAD_SURFACE) {
        glfm__eglSurfaceDestroy(platformData);
        glfm__eglSurfaceInit(platformData);
    } else if (err == EGL_CONTEXT_LOST || err == EGL_BAD_CONTEXT) {
        if (platformData->eglContext != EGL_NO_CONTEXT) {
            platformData->eglContext = EGL_NO_CONTEXT;
            platformData->eglContextCurrent = false;
            GLFM_LOG_LIFECYCLE("GL Context lost");
            if (platformData->surfaceCreatedNotified) {
                platformData->surfaceCreatedNotified = false;
                if (platformData->display && platformData->display->surfaceDestroyedFunc) {
                    platformData->display->surfaceDestroyedFunc(platformData->display);
                }
            }
        }
        glfm__eglContextInit(platformData);
    } else {
        glfm__eglDestroy(platformData);
        glfm__eglInit(platformData);
    }
}

static void glfm__drawFrame(GLFMPlatformData *platformData) {
    if (!platformData->eglContextCurrent) {
        // Probably a bad config (Happens on Android 2.3 emulator)
        return;
    }

    // Check for resize (or rotate)
    glfm__updateSurfaceSizeIfNeeded(platformData->display, false);

    // Tick and draw
    if (platformData->refreshRequested) {
        platformData->refreshRequested = false;
        if (platformData->display && platformData->display->surfaceRefreshFunc) {
            platformData->display->surfaceRefreshFunc(platformData->display);
        }
    }
    if (platformData->display && platformData->display->renderFunc) {
        platformData->display->renderFunc(platformData->display);
    }
}

// MARK: - ANativeActivity callbacks (UI thread)

enum {
    GLFMLooperIDCommand = 1,
    GLFMLooperIDInput = 2,
    GLFMLooperIDSensor = 3,
};

typedef enum {
    GLFMActivityCommandOnStart,
    GLFMActivityCommandOnPause,
    GLFMActivityCommandOnResume,
    GLFMActivityCommandOnStop,
    GLFMActivityCommandOnDestroy,
    GLFMActivityCommandOnWindowFocusGained,
    GLFMActivityCommandOnWindowFocusLost,
    GLFMActivityCommandOnNativeWindowCreated,
    GLFMActivityCommandOnNativeWindowResized,
    GLFMActivityCommandOnNativeWindowRedrawNeeded,
    GLFMActivityCommandOnNativeWindowDestroyed,
    GLFMActivityCommandOnInputQueueCreated,
    GLFMActivityCommandOnInputQueueDestroyed,
    GLFMActivityCommandOnContentRectChanged,
    GLFMActivityCommandOnConfigurationChanged,
    GLFMActivityCommandOnLowMemory,
} GLFMActivityCommand;

static void glfm__sendCommand(ANativeActivity *activity, GLFMActivityCommand command) {
    GLFMPlatformData *platformData = activity->instance;
    uint8_t data = (uint8_t)command;
    if (write(platformData->commandPipeWrite, &data, sizeof(data)) != sizeof(data)) {
        GLFM_LOG("Couldn't write to pipe");
    }
}

static void glfm__activityOnStart(ANativeActivity *activity) {
    GLFMPlatformData *platformData = activity->instance;
    if (platformData && platformData->display) {
        glfm__updateUserInterfaceChrome(platformData);
    }
    glfm__sendCommand(activity, GLFMActivityCommandOnStart);
}

static void glfm__activityOnPause(ANativeActivity *activity) {
    glfm__sendCommand(activity, GLFMActivityCommandOnPause);
}

static void glfm__activityOnResume(ANativeActivity *activity) {
    glfm__sendCommand(activity, GLFMActivityCommandOnResume);
}

static void glfm__activityOnStop(ANativeActivity *activity) {
    glfm__sendCommand(activity, GLFMActivityCommandOnStop);
}

static void glfm__activityOnWindowFocusChanged(ANativeActivity *activity, int hasFocus) {
    glfm__sendCommand(activity, (hasFocus ? GLFMActivityCommandOnWindowFocusGained :
        GLFMActivityCommandOnWindowFocusLost));
}

static void glfm__activityOnConfigurationChanged(ANativeActivity *activity) {
    glfm__sendCommand(activity, GLFMActivityCommandOnConfigurationChanged);
}

static void glfm__activityOnLowMemory(ANativeActivity *activity) {
    glfm__sendCommand(activity, GLFMActivityCommandOnLowMemory);
}

static void glfm__activityOnNativeWindowResized(ANativeActivity *activity, ANativeWindow *window) {
    (void)window;
    glfm__sendCommand(activity, GLFMActivityCommandOnNativeWindowResized);
}

static void glfm__activityOnNativeWindowRedrawNeeded(ANativeActivity *activity, ANativeWindow *window) {
    (void)window;
    glfm__sendCommand(activity, GLFMActivityCommandOnNativeWindowRedrawNeeded);
}

static void glfm__activityOnNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
    GLFMPlatformData *platformData = activity->instance;
    pthread_mutex_lock(&platformData->mutex);
    platformData->pendingWindow = window;
    glfm__sendCommand(activity, GLFMActivityCommandOnNativeWindowCreated);
    while (platformData->window != window) {
        pthread_cond_wait(&platformData->cond, &platformData->mutex);
    }
    pthread_mutex_unlock(&platformData->mutex);
}

static void glfm__activityOnNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    (void)window;
    glfm__sendCommand(activity, GLFMActivityCommandOnNativeWindowDestroyed);
}

static void glfm__activityOnInputQueueCreated(ANativeActivity *activity, AInputQueue *queue) {
    GLFMPlatformData *platformData = activity->instance;
    pthread_mutex_lock(&platformData->mutex);
    platformData->pendingInputQueue = queue;
    glfm__sendCommand(activity, GLFMActivityCommandOnInputQueueCreated);
    while (platformData->inputQueue != queue) {
        pthread_cond_wait(&platformData->cond, &platformData->mutex);
    }
    pthread_mutex_unlock(&platformData->mutex);
}

static void glfm__activityOnInputQueueDestroyed(ANativeActivity *activity, AInputQueue *queue) {
    (void)queue;
    glfm__sendCommand(activity, GLFMActivityCommandOnInputQueueDestroyed);
}

static void glfm__activityOnContentRectChanged(ANativeActivity *activity, const ARect *rect) {
    GLFMPlatformData *platformData = activity->instance;
    int nextContentRectIndex = platformData->contentRectIndex ^ 1;
    platformData->contentRectArray[nextContentRectIndex] = *rect;
    platformData->contentRectIndex = nextContentRectIndex;
    glfm__resetContentRect(platformData); // Reset so that onContentRectChanged acts as a global layout listener.
    glfm__sendCommand(activity, GLFMActivityCommandOnContentRectChanged);
}

static void glfm__activityOnDestroy(ANativeActivity *activity) {
    GLFMPlatformData *platformData = activity->instance;
    pthread_mutex_lock(&platformData->mutex);
    glfm__sendCommand(activity, GLFMActivityCommandOnDestroy);
    while (platformData->threadRunning) {
        pthread_cond_wait(&platformData->cond, &platformData->mutex);
    }
    pthread_mutex_unlock(&platformData->mutex);

    close(platformData->commandPipeRead);
    close(platformData->commandPipeWrite);
    pthread_cond_destroy(&platformData->cond);
    pthread_mutex_destroy(&platformData->mutex);

    close(platformData->uiCommandPipeRead);
    close(platformData->uiCommandPipeWrite);
    GLFM_LOG_LIFECYCLE("Goodbye");
}

static void *glfm__activityOnSaveInstanceState(ANativeActivity *activity, size_t *outSize) {
    (void)activity;
    *outSize = 0;
    return NULL;
}

JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    (void)savedState;
    (void)savedStateSize;

    GLFM_LOG_LIFECYCLE("ANativeActivity_onCreate (API %i)", activity->sdkVersion);
    ALooper *looper = ALooper_forThread();
    if (!looper) {
        GLFM_LOG("No looper");
        return;
    }
    int commandPipe[2];
    int uiCommandPipe[2];
    if (pipe(commandPipe)) {
        GLFM_LOG("Couldn't create pipe");
        return;
    }
    if (pipe(uiCommandPipe)) {
        GLFM_LOG("Couldn't create UI pipe");
        return;
    }

    activity->callbacks->onStart = glfm__activityOnStart;
    activity->callbacks->onPause = glfm__activityOnPause;
    activity->callbacks->onResume = glfm__activityOnResume;
    activity->callbacks->onStop = glfm__activityOnStop;
    activity->callbacks->onDestroy = glfm__activityOnDestroy;
    activity->callbacks->onWindowFocusChanged = glfm__activityOnWindowFocusChanged;
    activity->callbacks->onNativeWindowCreated = glfm__activityOnNativeWindowCreated;
    activity->callbacks->onNativeWindowResized = glfm__activityOnNativeWindowResized;
    activity->callbacks->onNativeWindowRedrawNeeded = glfm__activityOnNativeWindowRedrawNeeded;
    activity->callbacks->onNativeWindowDestroyed = glfm__activityOnNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = glfm__activityOnInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = glfm__activityOnInputQueueDestroyed;
    activity->callbacks->onContentRectChanged = glfm__activityOnContentRectChanged;
    activity->callbacks->onConfigurationChanged = glfm__activityOnConfigurationChanged;
    activity->callbacks->onLowMemory = glfm__activityOnLowMemory;
    activity->callbacks->onSaveInstanceState = glfm__activityOnSaveInstanceState;

    if (platformDataGlobal == NULL) {
        // ANativeActivity_onCreate can be called multiple times for the same Activity.
        // For now, use a global to prevent glfmMain() from being called multiple times.
        // This behavior may need to change in the future.
        platformDataGlobal = calloc(1, sizeof(GLFMPlatformData));
    }
    GLFMPlatformData *platformData = platformDataGlobal;

    activity->instance = platformData;
    platformData->activity = activity;
    platformData->window = NULL;
    platformData->threadRunning = false;
    platformData->destroyRequested = false;
    platformData->contentRectArray[0] = (ARect) { 0 };
    platformData->contentRectArray[1] = (ARect) { 0 };
    platformData->commandPipeRead = commandPipe[0];
    platformData->commandPipeWrite = commandPipe[1];

    pthread_mutex_init(&platformData->mutex, NULL);
    pthread_cond_init(&platformData->cond, NULL);

    // Setup UI thread callbacks
    platformData->uiLooper = looper;
    platformData->uiCommandPipeRead = uiCommandPipe[0];
    platformData->uiCommandPipeWrite = uiCommandPipe[1];
    ALooper_addFd(platformData->uiLooper, platformData->uiCommandPipeRead,
                  ALOOPER_POLL_CALLBACK,ALOOPER_EVENT_INPUT,
                  glfm__looperCallback, platformData);

    // Start thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&platformData->thread, &attr, glfm__mainLoop,
                   platformData);

    // Wait for thread to start
    pthread_mutex_lock(&platformData->mutex);
    while (!platformData->threadRunning) {
        pthread_cond_wait(&platformData->cond, &platformData->mutex);
    }
    pthread_mutex_unlock(&platformData->mutex);
    GLFM_LOG_LIFECYCLE("Returning from ANativeActivity_onCreate");
}

// MARK: - UI thread callbacks

typedef struct {
    void (*function)(GLFMPlatformData *platformData, void *userData);
    void *userData;
} GLFMLooperMessage;

// Called from the UI thread
static int glfm__looperCallback(int fd, int events, void *userData) {
    GLFMPlatformData *platformData = userData;
    GLFMLooperMessage message;
    assert(ALooper_forThread() == platformData->uiLooper);
    if ((events & ALOOPER_EVENT_INPUT) != 0) {
        if (read(fd, &message, sizeof(message)) == sizeof(message)) {
            message.function(platformData, message.userData);
        }
    }
    return 1;
}

/// Queues a function to execute on the UI thread.
/// Returns true if the function was queued, false otherwise.
static bool glfm__runOnUIThread(GLFMPlatformData *platformData,
                                void (*function)(GLFMPlatformData *platformData, void *userData),
                                void *userData) {
    assert(platformData->looper == ALooper_forThread());
    assert(function != NULL);
    if (platformData->looper != ALooper_forThread() || !function) {
        return false;
    }

    GLFMLooperMessage message = { 0 };
    message.function = function;
    message.userData = userData;
    if (write(platformData->uiCommandPipeWrite, &message, sizeof(message)) != sizeof(message)) {
        // The pipe is full.
        return false;
    }
    return true;
}

// MARK: - App command callback and input callbacks

static void glfm__setAnimating(GLFMPlatformData *platformData, bool animating) {
    if (platformData->animating != animating) {
        platformData->animating = animating;
        platformData->refreshRequested = true;
        if (platformData->display && platformData->display->focusFunc) {
            platformData->display->focusFunc(platformData->display, animating);
        }
        glfm__setAllRequestedSensorsEnabled(platformData->display, animating);
    }
}

static void glfm__onAppCmd(GLFMPlatformData *platformData, GLFMActivityCommand command) {
    switch (command) {
        case GLFMActivityCommandOnNativeWindowCreated: {
            GLFM_LOG_LIFECYCLE("OnNativeWindowCreated");
            pthread_mutex_lock(&platformData->mutex);
            platformData->window = platformData->pendingWindow;
            pthread_cond_broadcast(&platformData->cond);
            pthread_mutex_unlock(&platformData->mutex);

            const bool success = glfm__eglInit(platformData);
            if (!success) {
                glfm__eglCheckError(platformData);
            }
            platformData->refreshRequested = true;
            glfm__drawFrame(platformData);
            break;
        }
        case GLFMActivityCommandOnNativeWindowResized: {
            GLFM_LOG_LIFECYCLE("OnNativeWindowResized");
            break;
        }
        case GLFMActivityCommandOnNativeWindowDestroyed: {
            GLFM_LOG_LIFECYCLE("OnNativeWindowDestroyed");
            platformData->window = NULL;
            glfm__eglSurfaceDestroy(platformData);
            glfm__setAnimating(platformData, false);
            break;
        }
        case GLFMActivityCommandOnNativeWindowRedrawNeeded: {
            GLFM_LOG_LIFECYCLE("OnNativeWindowRedrawNeeded");
            platformData->refreshRequested = true;
            break;
        }
        case GLFMActivityCommandOnWindowFocusGained: {
            GLFM_LOG_LIFECYCLE("OnWindowFocusGained");
            glfm__setAnimating(platformData, true);
            break;
        }
        case GLFMActivityCommandOnWindowFocusLost: {
            GLFM_LOG_LIFECYCLE("OnWindowFocusLost");
            if (platformData->animating) {
                platformData->refreshRequested = true;
                glfm__drawFrame(platformData);
                glfm__setAnimating(platformData, false);
            }
            break;
        }
        case GLFMActivityCommandOnContentRectChanged: {
            // NOTE: Content rect might be the same, as this is also used as a global layout listener
#if GLFM_LOG_LIFECYCLE_ENABLE
            ARect *oldRect = &platformData->contentRectArray[platformData->contentRectIndex ^ 1];
            ARect *newRect = &platformData->contentRectArray[platformData->contentRectIndex];
            GLFM_LOG_LIFECYCLE("OnContentRectChanged (from %i,%i,%i,%i to %i,%i,%i,%i)",
                               oldRect->left, oldRect->top, oldRect->right, oldRect->bottom,
                               newRect->left, newRect->top, newRect->right, newRect->bottom);
#endif

            platformData->refreshRequested = true;
            if (platformData->window) {
                bool sizedChanged = glfm__updateSurfaceSizeIfNeeded(platformData->display, true);
                if (!sizedChanged) {
                    glfm__reportOrientationChangeIfNeeded(platformData->display);
                    glfm__reportInsetsChangedIfNeeded(platformData->display);
                    glfm__updateKeyboardVisibility(platformData);
                }
            }
            break;
        }
        case GLFMActivityCommandOnLowMemory: {
            GLFM_LOG_LIFECYCLE("OnLowMemory");
            if (platformData->display && platformData->display->lowMemoryFunc) {
                platformData->display->lowMemoryFunc(platformData->display);
            }
            break;
        }
        case GLFMActivityCommandOnStart: {
            GLFM_LOG_LIFECYCLE("OnStart");
            break;
        }
        case GLFMActivityCommandOnResume: {
            GLFM_LOG_LIFECYCLE("OnResume");
            break;
        }
        case GLFMActivityCommandOnPause: {
            GLFM_LOG_LIFECYCLE("OnPause");
            break;
        }
        case GLFMActivityCommandOnStop: {
            GLFM_LOG_LIFECYCLE("OnStop");
            break;
        }
        case GLFMActivityCommandOnDestroy: {
            GLFM_LOG_LIFECYCLE("OnDestroy");
            glfm__eglDestroy(platformData);
            glfm__setAnimating(platformData, false);
            platformData->destroyRequested = true;
            break;
        }
        case GLFMActivityCommandOnInputQueueCreated: {
            GLFM_LOG_LIFECYCLE("OnInputQueueCreated");
            pthread_mutex_lock(&platformData->mutex);
            if (platformData->inputQueue) {
                AInputQueue_detachLooper(platformData->inputQueue);
            }
            platformData->inputQueue = platformData->pendingInputQueue;
            AInputQueue_attachLooper(platformData->inputQueue, platformData->looper,
                                     GLFMLooperIDInput, NULL, NULL);
            pthread_cond_broadcast(&platformData->cond);
            pthread_mutex_unlock(&platformData->mutex);
            break;
        }
        case GLFMActivityCommandOnInputQueueDestroyed: {
            GLFM_LOG_LIFECYCLE("OnInputQueueDestroyed");
            if (platformData->inputQueue) {
                AInputQueue_detachLooper(platformData->inputQueue);
                platformData->inputQueue = NULL;
            }
            break;
        }
        case GLFMActivityCommandOnConfigurationChanged: {
            GLFM_LOG_LIFECYCLE("OnConfigurationChanged");
            AConfiguration_fromAssetManager(platformData->config,
                                            platformData->activity->assetManager);
            break;
        }
        default: {
            // Do nothing
            break;
        }
    }
}

static void glfm__unicodeToUTF8(uint32_t unicode, char utf8[5]) {
    if (unicode < 0x80) {
        utf8[0] = (char)(unicode & 0x7fu);
        utf8[1] = 0;
    } else if (unicode < 0x800) {
        utf8[0] = (char)(0xc0u | (unicode >> 6u));
        utf8[1] = (char)(0x80u | (unicode & 0x3fu));
        utf8[2] = 0;
    } else if (unicode < 0x10000) {
        utf8[0] = (char)(0xe0u | (unicode >> 12u));
        utf8[1] = (char)(0x80u | ((unicode >> 6u) & 0x3fu));
        utf8[2] = (char)(0x80u | (unicode & 0x3fu));
        utf8[3] = 0;
    } else if (unicode < 0x110000) {
        utf8[0] = (char)(0xf0u | (unicode >> 18u));
        utf8[1] = (char)(0x80u | ((unicode >> 12u) & 0x3fu));
        utf8[2] = (char)(0x80u | ((unicode >> 6u) & 0x3fu));
        utf8[3] = (char)(0x80u | (unicode & 0x3fu));
        utf8[4] = 0;
    } else {
        utf8[0] = 0;
    }
}

static uint32_t glfm__getUnicodeChar(GLFMPlatformData *platformData, jint keyCode, jint metaState) {
    JNIEnv *jni = platformData->jniEnv;
    if ((*jni)->ExceptionCheck(jni)) {
        return 0;
    }

    jclass keyEventClass = (*jni)->FindClass(jni, "android/view/KeyEvent");
    if (glfm__wasJavaExceptionThrown(jni) || !keyEventClass) {
        return 0;
    }

    jmethodID getUnicodeChar = (*jni)->GetMethodID(jni, keyEventClass, "getUnicodeChar", "(I)I");
    jmethodID eventConstructor = (*jni)->GetMethodID(jni, keyEventClass, "<init>", "(II)V");

    jobject eventObject = (*jni)->NewObject(jni, keyEventClass, eventConstructor,
                                            (jint)AKEY_EVENT_ACTION_DOWN, keyCode);
    if (glfm__wasJavaExceptionThrown(jni) || !eventObject) {
        return 0;
    }

    jint unicodeKey = (*jni)->CallIntMethod(jni, eventObject, getUnicodeChar, metaState);

    (*jni)->DeleteLocalRef(jni, eventObject);
    (*jni)->DeleteLocalRef(jni, keyEventClass);

    if (glfm__wasJavaExceptionThrown(jni)) {
        return 0;
    } else {
        return (uint32_t)unicodeKey;
    }
}

/*
 * Move task to the back if it is root task. This make the back button have the same behavior
 * as the home button.
 *
 * Without this, when the user presses the back button, the loop in glfm__mainLoop() is exited, the
 * OpenGL context is destroyed, and the main thread is destroyed. The glfm__mainLoop() function
 * would be called again in the same process if the user returns to the app.
 *
 * When this, when the app is in the background, the app will pause in the ALooper_pollAll() call.
 */
static bool glfm__handleBackButton(GLFMPlatformData *platformData) {
    if (!platformData || !platformData->activity) {
        return false;
    }
    JNIEnv *jni = platformData->jniEnv;
    if ((*jni)->ExceptionCheck(jni)) {
        return false;
    }

    jboolean handled = glfm__callJavaMethodWithArgs(jni, platformData->activity->clazz,
                                                    "moveTaskToBack", "(Z)Z", Boolean, false);
    return !glfm__wasJavaExceptionThrown(jni) && handled;
}

static bool glfm__onKeyEvent(GLFMPlatformData *platformData, AInputEvent *event) {
    if (!platformData || !platformData->display) {
        return false;
    }
    GLFMDisplay *display = platformData->display;
    int32_t aAction = AKeyEvent_getAction(event);
    int32_t aKeyCode = AKeyEvent_getKeyCode(event);
    int32_t aMetaState = AKeyEvent_getMetaState(event);
    if (aKeyCode == 0) {
        // aKeyCode is 0 for many non-ASCII keys from the virtual keyboard.
        return false;
    }
    if (aKeyCode == INT32_MAX) {
        // This is a special key code for GLFM where the scancode represents a unicode character.
        if (display->charFunc) {
            uint32_t unicode = (uint32_t)AKeyEvent_getScanCode(event);
            char utf8[5];
            glfm__unicodeToUTF8(unicode, utf8);
            display->charFunc(display, utf8, 0);
        }
        return true;
    }
    bool handled = false;
    if (display->keyFunc) {
        static const GLFMKeyCode AKEYCODE_MAP[] = {
                [AKEYCODE_BACK]            = GLFMKeyCodeNavigationBack,

                [AKEYCODE_0]               = GLFMKeyCode0,
                [AKEYCODE_1]               = GLFMKeyCode1,
                [AKEYCODE_2]               = GLFMKeyCode2,
                [AKEYCODE_3]               = GLFMKeyCode3,
                [AKEYCODE_4]               = GLFMKeyCode4,
                [AKEYCODE_5]               = GLFMKeyCode5,
                [AKEYCODE_6]               = GLFMKeyCode6,
                [AKEYCODE_7]               = GLFMKeyCode7,
                [AKEYCODE_8]               = GLFMKeyCode8,
                [AKEYCODE_9]               = GLFMKeyCode9,

                [AKEYCODE_DPAD_UP]         = GLFMKeyCodeArrowUp,
                [AKEYCODE_DPAD_DOWN]       = GLFMKeyCodeArrowDown,
                [AKEYCODE_DPAD_LEFT]       = GLFMKeyCodeArrowLeft,
                [AKEYCODE_DPAD_RIGHT]      = GLFMKeyCodeArrowRight,

                [AKEYCODE_POWER]           = GLFMKeyCodePower,

                [AKEYCODE_A]               = GLFMKeyCodeA,
                [AKEYCODE_B]               = GLFMKeyCodeB,
                [AKEYCODE_C]               = GLFMKeyCodeC,
                [AKEYCODE_D]               = GLFMKeyCodeD,
                [AKEYCODE_E]               = GLFMKeyCodeE,
                [AKEYCODE_F]               = GLFMKeyCodeF,
                [AKEYCODE_G]               = GLFMKeyCodeG,
                [AKEYCODE_H]               = GLFMKeyCodeH,
                [AKEYCODE_I]               = GLFMKeyCodeI,
                [AKEYCODE_J]               = GLFMKeyCodeJ,
                [AKEYCODE_K]               = GLFMKeyCodeK,
                [AKEYCODE_L]               = GLFMKeyCodeL,
                [AKEYCODE_M]               = GLFMKeyCodeM,
                [AKEYCODE_N]               = GLFMKeyCodeN,
                [AKEYCODE_O]               = GLFMKeyCodeO,
                [AKEYCODE_P]               = GLFMKeyCodeP,
                [AKEYCODE_Q]               = GLFMKeyCodeQ,
                [AKEYCODE_R]               = GLFMKeyCodeR,
                [AKEYCODE_S]               = GLFMKeyCodeS,
                [AKEYCODE_T]               = GLFMKeyCodeT,
                [AKEYCODE_U]               = GLFMKeyCodeU,
                [AKEYCODE_V]               = GLFMKeyCodeV,
                [AKEYCODE_W]               = GLFMKeyCodeW,
                [AKEYCODE_X]               = GLFMKeyCodeX,
                [AKEYCODE_Y]               = GLFMKeyCodeY,
                [AKEYCODE_Z]               = GLFMKeyCodeZ,
                [AKEYCODE_COMMA]           = GLFMKeyCodeComma,
                [AKEYCODE_PERIOD]          = GLFMKeyCodePeriod,
                [AKEYCODE_ALT_LEFT]        = GLFMKeyCodeAltLeft,
                [AKEYCODE_ALT_RIGHT]       = GLFMKeyCodeAltRight,
                [AKEYCODE_SHIFT_LEFT]      = GLFMKeyCodeShiftLeft,
                [AKEYCODE_SHIFT_RIGHT]     = GLFMKeyCodeShiftRight,
                [AKEYCODE_TAB]             = GLFMKeyCodeTab,
                [AKEYCODE_SPACE]           = GLFMKeyCodeSpace,

                [AKEYCODE_ENTER]           = GLFMKeyCodeEnter,
                [AKEYCODE_DEL]             = GLFMKeyCodeBackspace,
                [AKEYCODE_GRAVE]           = GLFMKeyCodeBackquote,
                [AKEYCODE_MINUS]           = GLFMKeyCodeMinus,
                [AKEYCODE_EQUALS]          = GLFMKeyCodeEqual,
                [AKEYCODE_LEFT_BRACKET]    = GLFMKeyCodeBracketLeft,
                [AKEYCODE_RIGHT_BRACKET]   = GLFMKeyCodeBracketRight,
                [AKEYCODE_BACKSLASH]       = GLFMKeyCodeBackslash,
                [AKEYCODE_SEMICOLON]       = GLFMKeyCodeSemicolon,
                [AKEYCODE_APOSTROPHE]      = GLFMKeyCodeQuote,
                [AKEYCODE_SLASH]           = GLFMKeyCodeSlash,

                [AKEYCODE_MENU]            = GLFMKeyCodeMenu,

                [AKEYCODE_PAGE_UP]         = GLFMKeyCodePageUp,
                [AKEYCODE_PAGE_DOWN]       = GLFMKeyCodePageDown,

                [AKEYCODE_ESCAPE]          = GLFMKeyCodeEscape,
                [AKEYCODE_FORWARD_DEL]     = GLFMKeyCodeDelete,
                [AKEYCODE_CTRL_LEFT]       = GLFMKeyCodeControlLeft,
                [AKEYCODE_CTRL_RIGHT]      = GLFMKeyCodeControlRight,
                [AKEYCODE_CAPS_LOCK]       = GLFMKeyCodeCapsLock,
                [AKEYCODE_SCROLL_LOCK]     = GLFMKeyCodeScrollLock,
                [AKEYCODE_META_LEFT]       = GLFMKeyCodeMetaLeft,
                [AKEYCODE_META_RIGHT]      = GLFMKeyCodeMetaRight,
                [AKEYCODE_FUNCTION]        = GLFMKeyCodeFunction,
                [AKEYCODE_SYSRQ]           = GLFMKeyCodePrintScreen,
                [AKEYCODE_BREAK]           = GLFMKeyCodePause,
                [AKEYCODE_MOVE_HOME]       = GLFMKeyCodeHome,
                [AKEYCODE_MOVE_END]        = GLFMKeyCodeEnd,
                [AKEYCODE_INSERT]          = GLFMKeyCodeInsert,

                [AKEYCODE_F1]              = GLFMKeyCodeF1,
                [AKEYCODE_F2]              = GLFMKeyCodeF2,
                [AKEYCODE_F3]              = GLFMKeyCodeF3,
                [AKEYCODE_F4]              = GLFMKeyCodeF4,
                [AKEYCODE_F5]              = GLFMKeyCodeF5,
                [AKEYCODE_F6]              = GLFMKeyCodeF6,
                [AKEYCODE_F7]              = GLFMKeyCodeF7,
                [AKEYCODE_F8]              = GLFMKeyCodeF8,
                [AKEYCODE_F9]              = GLFMKeyCodeF9,
                [AKEYCODE_F10]             = GLFMKeyCodeF10,
                [AKEYCODE_F11]             = GLFMKeyCodeF11,
                [AKEYCODE_F12]             = GLFMKeyCodeF12,
                [AKEYCODE_NUM_LOCK]        = GLFMKeyCodeNumLock,
                [AKEYCODE_NUMPAD_0]        = GLFMKeyCodeNumpad0,
                [AKEYCODE_NUMPAD_1]        = GLFMKeyCodeNumpad1,
                [AKEYCODE_NUMPAD_2]        = GLFMKeyCodeNumpad2,
                [AKEYCODE_NUMPAD_3]        = GLFMKeyCodeNumpad3,
                [AKEYCODE_NUMPAD_4]        = GLFMKeyCodeNumpad4,
                [AKEYCODE_NUMPAD_5]        = GLFMKeyCodeNumpad5,
                [AKEYCODE_NUMPAD_6]        = GLFMKeyCodeNumpad6,
                [AKEYCODE_NUMPAD_7]        = GLFMKeyCodeNumpad7,
                [AKEYCODE_NUMPAD_8]        = GLFMKeyCodeNumpad8,
                [AKEYCODE_NUMPAD_9]        = GLFMKeyCodeNumpad9,
                [AKEYCODE_NUMPAD_DIVIDE]   = GLFMKeyCodeNumpadDivide,
                [AKEYCODE_NUMPAD_MULTIPLY] = GLFMKeyCodeNumpadMultiply,
                [AKEYCODE_NUMPAD_SUBTRACT] = GLFMKeyCodeNumpadSubtract,
                [AKEYCODE_NUMPAD_ADD]      = GLFMKeyCodeNumpadAdd,
                [AKEYCODE_NUMPAD_DOT]      = GLFMKeyCodeNumpadDecimal,
                [AKEYCODE_NUMPAD_ENTER]    = GLFMKeyCodeNumpadEnter,
                [AKEYCODE_NUMPAD_EQUALS]   = GLFMKeyCodeNumpadEqual,
        };

        GLFMKeyCode keyCode = GLFMKeyCodeUnknown;
        if (aKeyCode >= 0 && aKeyCode < (int32_t)(sizeof(AKEYCODE_MAP) / sizeof(*AKEYCODE_MAP))) {
            keyCode = AKEYCODE_MAP[aKeyCode];
        }

        int modifiers = 0;
        if ((aMetaState & AMETA_SHIFT_ON) != 0) {
            modifiers |= GLFMKeyModifierShift;
        }
        if ((aMetaState & AMETA_CTRL_ON) != 0) {
            modifiers |= GLFMKeyModifierControl;
        }
        if ((aMetaState & AMETA_ALT_ON) != 0) {
            modifiers |= GLFMKeyModifierAlt;
        }
        if ((aMetaState & AMETA_META_ON) != 0) {
            modifiers |= GLFMKeyModifierMeta;
        }
        if ((aMetaState & AMETA_FUNCTION_ON) != 0) {
            modifiers |= GLFMKeyModifierFunction;
        }

        if (aAction == AKEY_EVENT_ACTION_UP) {
            handled = display->keyFunc(display, keyCode, GLFMKeyActionReleased, modifiers);
        } else if (aAction == AKEY_EVENT_ACTION_DOWN) {
            GLFMKeyAction keyAction;
            if (AKeyEvent_getRepeatCount(event) > 0) {
                keyAction = GLFMKeyActionRepeated;
            } else {
                keyAction = GLFMKeyActionPressed;
            }
            handled = display->keyFunc(display, keyCode, keyAction, modifiers);
        } else if (aAction == AKEY_EVENT_ACTION_MULTIPLE) {
            int32_t i;
            for (i = AKeyEvent_getRepeatCount(event); i > 0; i--) {
                if (display->keyFunc) {
                    handled |= display->keyFunc(display, keyCode, GLFMKeyActionPressed, modifiers);
                }
                if (display->keyFunc) {
                    handled |= display->keyFunc(display, keyCode, GLFMKeyActionReleased, modifiers);
                }
            }
        }
    }

#if GLFM_HANDLE_BACK_BUTTON
    if (!handled && aAction == AKEY_EVENT_ACTION_UP && aKeyCode == AKEYCODE_BACK) {
        handled = glfm__handleBackButton(platformData) ? 1 : 0;
    }
#endif

    if (display->charFunc && (aAction == AKEY_EVENT_ACTION_DOWN || aAction == AKEY_EVENT_ACTION_MULTIPLE)) {
        uint32_t unicode = glfm__getUnicodeChar(platformData, aKeyCode, aMetaState);
        if (unicode >= ' ') {
            char utf8[5];
            glfm__unicodeToUTF8(unicode, utf8);
            if (aAction == AKEY_EVENT_ACTION_DOWN) {
                display->charFunc(display, utf8, 0);
            } else {
                int32_t i;
                for (i = AKeyEvent_getRepeatCount(event); i > 0; i--) {
                    if (display->charFunc) {
                        display->charFunc(display, utf8, 0);
                    }
                }
            }
        }
    }
    return handled;
}

static bool glfm__onTouchEvent(GLFMPlatformData *platformData, AInputEvent *event) {
    if (!platformData || !platformData->display || !platformData->display->touchFunc) {
        return false;
    }
    GLFMDisplay *display = platformData->display;
    const int maxTouches = platformData->multitouchEnabled ? GLFM_MAX_SIMULTANEOUS_TOUCHES : 1;
    const int32_t action = AMotionEvent_getAction(event);
    const uint32_t maskedAction = (uint32_t)action & (uint32_t)AMOTION_EVENT_ACTION_MASK;

    GLFMTouchPhase phase;
    bool validAction = true;

    switch (maskedAction) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            phase = GLFMTouchPhaseBegan;
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
        case AMOTION_EVENT_ACTION_OUTSIDE:
            phase = GLFMTouchPhaseEnded;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            phase = GLFMTouchPhaseMoved;
            break;
        case AMOTION_EVENT_ACTION_CANCEL:
            phase = GLFMTouchPhaseCancelled;
            break;
        default:
            phase = GLFMTouchPhaseCancelled;
            validAction = false;
            break;
    }
    if (validAction) {
        if (phase == GLFMTouchPhaseMoved) {
            const size_t count = AMotionEvent_getPointerCount(event);
            size_t i;
            for (i = 0; i < count; i++) {
                const int touchNumber = AMotionEvent_getPointerId(event, i);
                if (touchNumber >= 0 && touchNumber < maxTouches && display->touchFunc) {
                    double x = (double)AMotionEvent_getX(event, i);
                    double y = (double)AMotionEvent_getY(event, i);
                    display->touchFunc(display, touchNumber, phase, x, y);
                }
            }
        } else {
            const size_t index = (size_t)(((uint32_t)action &
                    (uint32_t)AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                    (uint32_t)AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            const int touchNumber = AMotionEvent_getPointerId(event, index);
            if (touchNumber >= 0 && touchNumber < maxTouches && display->touchFunc) {
                double x = (double)AMotionEvent_getX(event, index);
                double y = (double)AMotionEvent_getY(event, index);
                display->touchFunc(display, touchNumber, phase, x, y);
            }
        }
    }
    return true;
}

static void glfm__onInputEvent(GLFMPlatformData *platformData) {
    AInputEvent *event = NULL;
    while (AInputQueue_getEvent(platformData->inputQueue, &event) >= 0) {
        if (AInputQueue_preDispatchEvent(platformData->inputQueue, event)) {
            continue;
        }
        bool handled = false;
        int32_t eventType = AInputEvent_getType(event);
        if (eventType == AINPUT_EVENT_TYPE_KEY) {
            handled = glfm__onKeyEvent(platformData, event);
        } else if (eventType == AINPUT_EVENT_TYPE_MOTION) {
            handled = glfm__onTouchEvent(platformData, event);
        }
        AInputQueue_finishEvent(platformData->inputQueue, event, (int)handled);
    }
}

static void glfm__onSensorEvent(GLFMPlatformData *platformData) {
    ASensorEvent event;
    bool sensorEventReceived[GLFM_NUM_SENSORS] = { 0 };
    while (ASensorEventQueue_getEvents(platformData->sensorEventQueue, &event, 1) > 0) {
        if (event.type == ASENSOR_TYPE_ACCELEROMETER) {
            const double G = (double)ASENSOR_STANDARD_GRAVITY;
            // Convert to iOS format
            GLFMSensorEvent *sensorEvent = &platformData->sensorEvent[GLFMSensorAccelerometer];
            sensorEvent->sensor = GLFMSensorAccelerometer;
            sensorEvent->timestamp = (double)event.timestamp / 1000000000.0;
            sensorEvent->vector.x = (double)event.acceleration.x / -G;
            sensorEvent->vector.y = (double)event.acceleration.y / -G;
            sensorEvent->vector.z = (double)event.acceleration.z / -G;
            sensorEventReceived[GLFMSensorAccelerometer] = true;
            platformData->sensorEventValid[GLFMSensorAccelerometer] = true;
        } else if (event.type == ASENSOR_TYPE_MAGNETIC_FIELD) {
            GLFMSensorEvent *sensorEvent = &platformData->sensorEvent[GLFMSensorMagnetometer];
            sensorEvent->sensor = GLFMSensorMagnetometer;
            sensorEvent->timestamp = (double)event.timestamp / 1000000000.0;
            sensorEvent->vector.x = (double)event.magnetic.x;
            sensorEvent->vector.y = (double)event.magnetic.y;
            sensorEvent->vector.z = (double)event.magnetic.z;
            sensorEventReceived[GLFMSensorMagnetometer] = true;
            platformData->sensorEventValid[GLFMSensorMagnetometer] = true;
        } else if (event.type == ASENSOR_TYPE_GYROSCOPE) {
            GLFMSensorEvent *sensorEvent = &platformData->sensorEvent[GLFMSensorGyroscope];
            sensorEvent->sensor = GLFMSensorGyroscope;
            sensorEvent->timestamp = (double)event.timestamp / 1000000000.0;
            sensorEvent->vector.x = (double)event.vector.x;
            sensorEvent->vector.y = (double)event.vector.y;
            sensorEvent->vector.z = (double)event.vector.z;
            sensorEventReceived[GLFMSensorGyroscope] = true;
            platformData->sensorEventValid[GLFMSensorGyroscope] = true;
        } else if (event.type == ASENSOR_TYPE_ROTATION_VECTOR) {
            const int SDK_INT = platformData->activity->sdkVersion;

            GLFMSensorEvent *sensorEvent = &platformData->sensorEvent[GLFMSensorRotationMatrix];
            sensorEvent->sensor = GLFMSensorRotationMatrix;
            sensorEvent->timestamp = (double)event.timestamp / 1000000000.0;

            // Get unit quaternion
            double qx = (double)event.vector.x;
            double qy = (double)event.vector.y;
            double qz = (double)event.vector.z;
            double qw;
            if (SDK_INT >= 18) {
                qw = (double)event.data[3];
            } else {
                qw = 1 - (qx * qx + qy * qy + qz * qz);
                qw = (qw > 0) ? sqrt(qw) : 0;
            }

            /*
             * Convert unit quaternion to rotation matrix.
             *
             * First, convert Android's reference frame to the same as iOS.
             * Android uses a reference frame where the Y axis points north,
             * and iOS uses a reference frame where the X axis points north.
             *
             * To convert the unit quaternion, pre-multiply the unit quaternion by
             * a rotation of -90 degrees around the Z axis.
             *
             * a=-90
             * q1 = cos(a/2) + 0i + 0j + sin(a/2)k
             *
             * Which is the same as:
             *
             * f = sqrt(2)/2
             * q1 = f + 0i + 0j - fk
             *
             * Multiplying two quaternions, where q2 is the original Android quaternion:
             *
             * q1q2 = (w1w2 - x1x2 - y1y2 - z1z2) +
             *        (w1x2 + x1w2 + y1z2 - z1y2)i +
             *        (w1y2 + z1x2 + y1w2 - x1z2)j +
             *        (w1z2 + x1y2 + z1w2 - y1x2)k
             *
             * Where x1 == 0, y1 == 0, z1 == -f, w1 == f:
             *
             * q1q2 = (f * (z2 + w2)) +
             *        (f * (y2 + x2))i +
             *        (f * (y2 - x2))j +
             *        (f * (z2 + w2))k
             *
             * In C:
             *
             * double f = sqrt(2)/2;
             * double qx_ = f * (qy + qx);
             * double qy_ = f * (qy - qx);
             * double qz_ = f * (qz - qw);
             * double qw_ = f * (qz + qw);
             *
             * However, since f*f == 0.5, and we don't need the converted quaternion,
             * we can remove a few multiplications.
            */
#if 0
            // Original (no conversion)
            double qxx2 = qx * qx * 2;
            double qxy2 = qx * qy * 2;
            double qxz2 = qx * qz * 2;
            double qxw2 = qx * qw * 2;
            double qyy2 = qy * qy * 2;
            double qyz2 = qy * qz * 2;
            double qyw2 = qy * qw * 2;
            double qzz2 = qz * qz * 2;
            double qzw2 = qz * qw * 2;
#else
            // Conversion to the same reference frame as iOS
            double qx_ = qy + qx;
            double qy_ = qy - qx;
            double qz_ = qz - qw;
            double qw_ = qz + qw;

            double qxx2 = qx_ * qx_;
            double qxy2 = qx_ * qy_;
            double qxz2 = qx_ * qz_;
            double qxw2 = qx_ * qw_;
            double qyy2 = qy_ * qy_;
            double qyz2 = qy_ * qz_;
            double qyw2 = qy_ * qw_;
            double qzz2 = qz_ * qz_;
            double qzw2 = qz_ * qw_;
#endif
            sensorEvent->matrix.m00 = 1 - qyy2 - qzz2;
            sensorEvent->matrix.m10 = qxy2 - qzw2;
            sensorEvent->matrix.m20 = qxz2 + qyw2;
            sensorEvent->matrix.m01 = qxy2 + qzw2;
            sensorEvent->matrix.m11 = 1 - qxx2 - qzz2;
            sensorEvent->matrix.m21 = qyz2 - qxw2;
            sensorEvent->matrix.m02 = qxz2 - qyw2;
            sensorEvent->matrix.m12 = qyz2 + qxw2;
            sensorEvent->matrix.m22 = 1 - qxx2 - qyy2;

            sensorEventReceived[GLFMSensorRotationMatrix] = true;
            platformData->sensorEventValid[GLFMSensorRotationMatrix] = true;
        }
    }

    // Send callbacks
    for (int i = 0; i < GLFM_NUM_SENSORS; i++) {
        GLFMSensorFunc sensorFunc = platformData->display->sensorFuncs[i];
        if (sensorFunc && sensorEventReceived[i]) {
            sensorFunc(platformData->display, platformData->sensorEvent[i]);
        }
    }
}

// MARK: - Thread entry point

static void *glfm__mainLoop(void *param) {
    GLFM_LOG_LIFECYCLE("glfm__mainLoop");

    // Init platform data
    GLFMPlatformData *platformData = param;
    platformData->refreshRequested = true;
    platformData->lastSwapTime = glfmGetTime();
    platformData->config = AConfiguration_new();
    AConfiguration_fromAssetManager(platformData->config, platformData->activity->assetManager);

    // Init looper
    platformData->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ALooper_addFd(platformData->looper, platformData->commandPipeRead,
                  GLFMLooperIDCommand, ALOOPER_EVENT_INPUT, NULL, NULL);

    // Init java env
    JavaVM *vm = platformData->activity->vm;
    (*vm)->AttachCurrentThread(vm, &platformData->jniEnv, NULL);

    // Get display scale
    const int ACONFIGURATION_DENSITY_ANY = 0xfffe; // Added in API 21
    const int32_t density = AConfiguration_getDensity(platformData->config);
    if (density == ACONFIGURATION_DENSITY_DEFAULT || density == ACONFIGURATION_DENSITY_NONE ||
            density == ACONFIGURATION_DENSITY_ANY || density <= 0) {
        platformData->scale = 1.0;
    } else {
        platformData->scale = density / 160.0;
    }

    // Call glfmMain() (once per instance)
    if (platformData->display == NULL) {
        GLFM_LOG_LIFECYCLE("glfmMain");
        platformData->display = calloc(1, sizeof(GLFMDisplay));
        platformData->display->platformData = platformData;
        platformData->display->supportedOrientations = GLFMInterfaceOrientationAll;
        platformData->display->swapBehavior = GLFMSwapBehaviorPlatformDefault;
        platformData->resizeEventWaitFrames = GLFM_RESIZE_EVENT_MAX_WAIT_FRAMES;
        glfmMain(platformData->display);
    }

    // Setup window params
    int32_t windowFormat;
    switch (platformData->display->colorFormat) {
        case GLFMColorFormatRGB565:
            windowFormat = WINDOW_FORMAT_RGB_565;
            break;
        case GLFMColorFormatRGBA8888: default:
            windowFormat = WINDOW_FORMAT_RGBA_8888;
            break;
    }
    bool fullscreen = platformData->display->uiChrome == GLFMUserInterfaceChromeNone;
    ANativeActivity_setWindowFormat(platformData->activity, windowFormat);
    ANativeActivity_setWindowFlags(platformData->activity,
                                   fullscreen ? AWINDOW_FLAG_FULLSCREEN : 0,
                                   AWINDOW_FLAG_FULLSCREEN);
    glfm__updateUserInterfaceChrome(platformData);
    if (platformData->activity->sdkVersion >= 28) {
        // Allow rendering in cutout areas ("safe area") in both portrait and landscape.
        // Test this code in Settings -> Developer Options -> Simulate a display with a cutout.
        static const int LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES = 0x00000001;

        JNIEnv *jni = platformData->jniEnv;
        jobject window = glfm__callJavaMethod(jni, platformData->activity->clazz, "getWindow",
                                              "()Landroid/view/Window;", Object);
        jobject attributes = glfm__callJavaMethod(jni, window, "getAttributes",
                                                  "()Landroid/view/WindowManager$LayoutParams;",
                                                  Object);
        jclass clazz = (*jni)->GetObjectClass(jni, attributes);
        jfieldID layoutInDisplayCutoutMode = (*jni)->GetFieldID(jni, clazz,
                "layoutInDisplayCutoutMode", "I");

        (*jni)->SetIntField(jni, attributes, layoutInDisplayCutoutMode,
                LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES);
        (*jni)->DeleteLocalRef(jni, clazz);
        (*jni)->DeleteLocalRef(jni, attributes);
        (*jni)->DeleteLocalRef(jni, window);
    }

    // Get initial values for reporting changes. First insets are valid until later.
    platformData->orientation = glfmGetInterfaceOrientation(platformData->display);
    platformData->insets.valid = false;

    // Notify thread running
    pthread_mutex_lock(&platformData->mutex);
    platformData->threadRunning = true;
    pthread_cond_broadcast(&platformData->cond);
    pthread_mutex_unlock(&platformData->mutex);

    // Run the main loop
    while (!platformData->destroyRequested) {
        int eventIdentifier;

        while ((eventIdentifier = ALooper_pollAll(platformData->animating ? 0 : -1,
                                                  NULL, NULL, NULL)) >= 0) {
            if (eventIdentifier == GLFMLooperIDCommand) {
                uint8_t cmd;
                if (read(platformData->commandPipeRead, &cmd, sizeof(cmd)) == sizeof(cmd)) {
                    GLFMActivityCommand command = (GLFMActivityCommand)cmd;
                    glfm__onAppCmd(platformData, command);
                } else {
                    GLFM_LOG("Couldn't read from pipe");
                }
            } else if (eventIdentifier == GLFMLooperIDInput) {
                glfm__onInputEvent(platformData);
            } else if (eventIdentifier == GLFMLooperIDSensor) {
                glfm__onSensorEvent(platformData);
            }
            if (platformData->destroyRequested) {
                break;
            }
        }

        if (platformData->animating && platformData->display) {
            platformData->swapCalled = false;
            glfm__drawFrame(platformData);
            if (!platformData->swapCalled) {
                // Sleep until next swap time (1/60 second after last swap time)
                const float refreshRate = glfm__getRefreshRate(platformData->display);
                const double sleepUntilTime = platformData->lastSwapTime + 1.0 / (double)refreshRate;
                double now = glfmGetTime();
                if (now >= sleepUntilTime) {
                    platformData->lastSwapTime = now;
                } else {
                    // Sleep until 500 microseconds before deadline
                    const double offset = 0.0005;
                    while (true) {
                        double sleepDuration = sleepUntilTime - now - offset;
                        if (sleepDuration <= 0) {
                            platformData->lastSwapTime = sleepUntilTime;
                            break;
                        }
                        useconds_t sleepDurationMicroseconds = (useconds_t) (sleepDuration * 1000000);
                        usleep(sleepDurationMicroseconds);
                        now = glfmGetTime();
                    }
                }
            }
        }
    }

    // Cleanup
    GLFM_LOG_LIFECYCLE("Destroying thread");
    if (platformData->inputQueue) {
        AInputQueue_detachLooper(platformData->inputQueue);
        platformData->inputQueue = NULL;
    }
    if (platformData->sensorEventQueue) {
        glfm__setAllRequestedSensorsEnabled(platformData->display, false);
        ASensorManager *sensorManager = ASensorManager_getInstance();
        ASensorManager_destroyEventQueue(sensorManager, platformData->sensorEventQueue);
        platformData->sensorEventQueue = NULL;
    }
    if (platformData->config) {
        AConfiguration_delete(platformData->config);
        platformData->config = NULL;
    }
    glfm__eglDestroy(platformData);
    glfm__setAnimating(platformData, false);
    (*vm)->DetachCurrentThread(vm);
    platformData->window = NULL;
    platformData->looper = NULL;

    // Notify thread no longer running
    pthread_mutex_lock(&platformData->mutex);
    platformData->threadRunning = false;
    pthread_cond_broadcast(&platformData->cond);
    pthread_mutex_unlock(&platformData->mutex);

    // App is destroyed, but glfm__mainLoop() can be called again in the same process.
    // Set GLFM_HANDLE_BACK_BUTTON to 0 to test this code.
    return NULL;
}

// MARK: - GLFM private functions

static jobject glfm__getDecorView(JNIEnv *jni, GLFMPlatformData *platformData) {
    if (!platformData || !platformData->activity || (*jni)->ExceptionCheck(jni)) {
        return NULL;
    }
    jobject window = glfm__callJavaMethod(jni, platformData->activity->clazz, "getWindow", "()Landroid/view/Window;", Object);
    if (glfm__wasJavaExceptionThrown(jni) || !window) {
        return NULL;
    }
    jobject decorView = glfm__callJavaMethod(jni, window, "getDecorView", "()Landroid/view/View;", Object);
    (*jni)->DeleteLocalRef(jni, window);
    return glfm__wasJavaExceptionThrown(jni) ? NULL : decorView;
}

static ARect glfm__getDecorViewRect(GLFMPlatformData *platformData, const ARect *defaultRect) {
    JNIEnv *jni = platformData->jniEnv;
    if ((*jni)->ExceptionCheck(jni)) {
        return *defaultRect;
    }

    jobject decorView = glfm__getDecorView(jni, platformData);
    if (!decorView) {
        return *defaultRect;
    }

    jintArray locationArray = (*jni)->NewIntArray(jni, 2);
    if (!locationArray) {
        (*jni)->DeleteLocalRef(jni, decorView);
        return *defaultRect;
    }

    jint location[2] = { 0 };
    glfm__callJavaMethodWithArgs(jni, decorView, "getLocationInWindow", "([I)V", Void, locationArray);
    (*jni)->GetIntArrayRegion(jni, locationArray, 0, 2, location);
    (*jni)->DeleteLocalRef(jni, locationArray);
    if ((*jni)->ExceptionCheck(jni)) {
        (*jni)->DeleteLocalRef(jni, decorView);
        return *defaultRect;
    }

    jint width = glfm__callJavaMethod(jni, decorView, "getWidth", "()I", Int);
    jint height = glfm__callJavaMethod(jni, decorView, "getHeight", "()I", Int);
    (*jni)->DeleteLocalRef(jni, decorView);
    if ((*jni)->ExceptionCheck(jni)) {
        return *defaultRect;
    }

    ARect result;
    result.left = location[0];
    result.top = location[1];
    result.right = location[0] + width;
    result.bottom = location[1] + height;
    return result;
}

static void glfm__updateUserInterfaceChromeCallback(GLFMPlatformData *platformData, void *userData) {
    (void)userData;
    glfm__updateUserInterfaceChrome(platformData);
}

// Can be called from either native thread or UI thread
static void glfm__updateUserInterfaceChrome(GLFMPlatformData *platformData) {
    static const unsigned int View_STATUS_BAR_HIDDEN = 0x00000001;
    static const unsigned int View_SYSTEM_UI_FLAG_LOW_PROFILE = 0x00000001;
    static const unsigned int View_SYSTEM_UI_FLAG_HIDE_NAVIGATION = 0x00000002;
    static const unsigned int View_SYSTEM_UI_FLAG_FULLSCREEN = 0x00000004;
    static const unsigned int View_SYSTEM_UI_FLAG_LAYOUT_STABLE = 0x00000100;
    static const unsigned int View_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = 0x00000200;
    static const unsigned int View_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = 0x00000400;
    static const unsigned int View_SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 0x00001000;

    if (!platformData || !platformData->activity) {
        return;
    }
    const int SDK_INT = platformData->activity->sdkVersion;
    if (SDK_INT < 11) {
        return;
    }

    JavaVM *vm = platformData->activity->vm;
    JNIEnv *jni = NULL;
    (*vm)->GetEnv(vm, (void **) &jni, JNI_VERSION_1_2);
    if (!jni || (*jni)->ExceptionCheck(jni)) {
        return;
    }

    jobject decorView = glfm__getDecorView(jni, platformData);
    if (!decorView) {
        return;
    }

    GLFMUserInterfaceChrome uiChrome = platformData->display->uiChrome;
    bool setNow;
    bool isUiThread = ALooper_forThread() == platformData->uiLooper;
    if (isUiThread) {
        setNow = true;
    } else {
        jboolean isDecorViewAttached;
        if (SDK_INT >= 19) {
            isDecorViewAttached = glfm__callJavaMethod(jni, decorView, "isAttachedToWindow", "()Z", Boolean);
        } else {
            isDecorViewAttached = glfm__callJavaMethod(jni, decorView, "getWindowToken", "()Landroid/os/IBinder;", Object) != NULL;
        }
        if (glfm__wasJavaExceptionThrown(jni)) {
            (*jni)->DeleteLocalRef(jni, decorView);
            return;
        }
        setNow = !isDecorViewAttached;
    }

    if (!setNow) {
        // Set on the UI thread
        glfm__runOnUIThread(platformData, glfm__updateUserInterfaceChromeCallback, NULL);
    } else {
        // Set now
        if (SDK_INT >= 30) {
            jobject windowInsetsController = glfm__callJavaMethod(jni, decorView, "getWindowInsetsController", "()Landroid/view/WindowInsetsController;", Object);
            jclass windowInsetsTypeClass = (*jni)->FindClass(jni, "android/view/WindowInsets$Type");
            if (windowInsetsController && windowInsetsTypeClass && !glfm__wasJavaExceptionThrown(jni)) {
                static const jint WindowInsetsController_BEHAVIOR_DEFAULT = 1;
                static const jint WindowInsetsController_BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE = 2;

                const jint systemBars = glfm__callJavaStaticMethod(jni, windowInsetsTypeClass, "systemBars", "()I", Int);

                if (uiChrome == GLFMUserInterfaceChromeNone) {
                    glfm__callJavaMethodWithArgs(jni, windowInsetsController, "setSystemBarsBehavior", "(I)V", Void,
                                                 WindowInsetsController_BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                    glfm__callJavaMethodWithArgs(jni, windowInsetsController, "hide", "(I)V", Void, systemBars);
                } else {
                    glfm__callJavaMethodWithArgs(jni, windowInsetsController, "setSystemBarsBehavior", "(I)V", Void,
                                                 WindowInsetsController_BEHAVIOR_DEFAULT);
                    if (uiChrome == GLFMUserInterfaceChromeNavigationAndStatusBar) {
                        glfm__callJavaMethodWithArgs(jni, windowInsetsController, "show", "(I)V", Void, systemBars);
                    } else if (uiChrome == GLFMUserInterfaceChromeNavigation) {
                        const jint statusBars = glfm__callJavaStaticMethod(jni, windowInsetsTypeClass, "statusBars", "()I", Int);
                        glfm__callJavaMethodWithArgs(jni, windowInsetsController, "hide", "(I)V", Void, statusBars);
                        glfm__callJavaMethodWithArgs(jni, windowInsetsController, "show", "(I)V", Void, systemBars & ~statusBars);
                    }
                }

                (*jni)->DeleteLocalRef(jni, windowInsetsController);
                (*jni)->DeleteLocalRef(jni, windowInsetsTypeClass);
                glfm__clearJavaException(jni);
            }
        } else {
            unsigned int systemUiVisibility = 0;
            if (uiChrome == GLFMUserInterfaceChromeNavigationAndStatusBar) {
                systemUiVisibility = 0;
            } else if (SDK_INT >= 11 && SDK_INT < 14) {
                systemUiVisibility = View_STATUS_BAR_HIDDEN;
            } else if (SDK_INT >= 14 && SDK_INT < 19) {
                if (uiChrome == GLFMUserInterfaceChromeNavigation) {
                    systemUiVisibility = View_SYSTEM_UI_FLAG_FULLSCREEN;
                } else {
                    systemUiVisibility = (View_SYSTEM_UI_FLAG_LOW_PROFILE | View_SYSTEM_UI_FLAG_FULLSCREEN);
                }
            } else if (SDK_INT >= 19) {
                if (uiChrome == GLFMUserInterfaceChromeNavigation) {
                    systemUiVisibility = View_SYSTEM_UI_FLAG_FULLSCREEN;
                } else {
                    systemUiVisibility = (View_SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                                          View_SYSTEM_UI_FLAG_FULLSCREEN |
                                          View_SYSTEM_UI_FLAG_LAYOUT_STABLE |
                                          View_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                                          View_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                                          View_SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
                }
            }

            glfm__callJavaMethodWithArgs(jni, decorView, "setSystemUiVisibility", "(I)V", Void,
                                         (jint)systemUiVisibility);
            glfm__clearJavaException(jni);
        }
    }
    (*jni)->DeleteLocalRef(jni, decorView);
}

static void glfm__resetContentRect(GLFMPlatformData *platformData) {
    // Reset's NativeActivity's content rect so that onContentRectChanged acts as a
    // OnGlobalLayoutListener. This is needed to detect changes to getWindowVisibleDisplayFrame()
    // HACK: This uses undocumented fields.

    JavaVM *vm = platformData->activity->vm;
    JNIEnv *jni = NULL;
    (*vm)->GetEnv(vm, (void **) &jni, JNI_VERSION_1_2);
    if (!jni || (*jni)->ExceptionCheck(jni)) {
        return;
    }

    jfieldID field = glfm__getJavaFieldID(jni, platformData->activity->clazz,
                                         "mLastContentWidth", "I");
    if (glfm__wasJavaExceptionThrown(jni) || !field) {
        return;
    }

    (*jni)->SetIntField(jni, platformData->activity->clazz, field, -1);
    glfm__clearJavaException(jni);
}

static ARect glfm__getWindowVisibleDisplayFrame(GLFMPlatformData *platformData, const ARect *defaultRect) {
    JNIEnv *jni = platformData->jniEnv;
    if ((*jni)->ExceptionCheck(jni)) {
        return *defaultRect;
    }

    jobject decorView = glfm__getDecorView(jni, platformData);
    if (!decorView) {
        return *defaultRect;
    }

    jclass javaRectClass = (*jni)->FindClass(jni, "android/graphics/Rect");
    if (glfm__wasJavaExceptionThrown(jni)) {
        return *defaultRect;
    }

    jobject javaRect = (*jni)->AllocObject(jni, javaRectClass);
    if (glfm__wasJavaExceptionThrown(jni)) {
        return *defaultRect;
    }

    glfm__callJavaMethodWithArgs(jni, decorView, "getWindowVisibleDisplayFrame",
                                 "(Landroid/graphics/Rect;)V", Void, javaRect);
    if (glfm__wasJavaExceptionThrown(jni)) {
        return *defaultRect;
    }

    ARect rect;
    rect.left = glfm__getJavaField(jni, javaRect, "left", "I", Int);
    rect.right = glfm__getJavaField(jni, javaRect, "right", "I", Int);
    rect.top = glfm__getJavaField(jni, javaRect, "top", "I", Int);
    rect.bottom = glfm__getJavaField(jni, javaRect, "bottom", "I", Int);
    (*jni)->DeleteLocalRef(jni, javaRect);
    (*jni)->DeleteLocalRef(jni, javaRectClass);
    if (glfm__wasJavaExceptionThrown(jni)) {
        return *defaultRect;
    }

    jintArray locationArray = (*jni)->NewIntArray(jni, 2);
    if (locationArray) {
        jint location[2] = { 0 };
        glfm__callJavaMethodWithArgs(jni, decorView, "getLocationOnScreen", "([I)V", Void,
                                     locationArray);
        (*jni)->GetIntArrayRegion(jni, locationArray, 0, 2, location);
        (*jni)->DeleteLocalRef(jni, locationArray);
        if (!glfm__wasJavaExceptionThrown(jni)) {
            rect.left -= location[0];
            rect.top -= location[1];
        }
    }

    (*jni)->DeleteLocalRef(jni, decorView);
    return rect;
}

static bool glfm__getSafeInsets(const GLFMDisplay *display, int *top, int *right,
                                int *bottom, int *left) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    const int SDK_INT = platformData->activity->sdkVersion;
    if (SDK_INT < 28) {
        return false;
    }

    JNIEnv *jni = platformData->jniEnv;
    jobject decorView = glfm__getDecorView(jni, platformData);
    if (!decorView) {
        return false;
    }

    jobject insets = glfm__callJavaMethod(jni, decorView, "getRootWindowInsets",
                                          "()Landroid/view/WindowInsets;", Object);
    (*jni)->DeleteLocalRef(jni, decorView);
    if (!insets) {
        return false;
    }

    jobject cutouts = glfm__callJavaMethod(jni, insets, "getDisplayCutout", "()Landroid/view/DisplayCutout;", Object);
    (*jni)->DeleteLocalRef(jni, insets);
    if (!cutouts) {
        return false;
    }

    *top = glfm__callJavaMethod(jni, cutouts, "getSafeInsetTop", "()I", Int);
    *right = glfm__callJavaMethod(jni, cutouts, "getSafeInsetRight", "()I", Int);
    *bottom = glfm__callJavaMethod(jni, cutouts, "getSafeInsetBottom", "()I", Int);
    *left = glfm__callJavaMethod(jni, cutouts, "getSafeInsetLeft", "()I", Int);

    (*jni)->DeleteLocalRef(jni, cutouts);
    return true;
}

static bool glfm__getSystemWindowInsets(const GLFMDisplay *display, int *top, int *right,
                                        int *bottom, int *left) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    const int SDK_INT = platformData->activity->sdkVersion;
    if (SDK_INT < 20) {
        return false;
    }

    JNIEnv *jni = platformData->jniEnv;
    jobject decorView = glfm__getDecorView(jni, platformData);
    if (!decorView) {
        return false;
    }

    jobject insets = glfm__callJavaMethod(jni, decorView, "getRootWindowInsets",
                                          "()Landroid/view/WindowInsets;", Object);
    (*jni)->DeleteLocalRef(jni, decorView);
    if (!insets) {
        return false;
    }

    *top = glfm__callJavaMethod(jni, insets, "getSystemWindowInsetTop", "()I", Int);
    *right = glfm__callJavaMethod(jni, insets, "getSystemWindowInsetRight", "()I", Int);
    *bottom = glfm__callJavaMethod(jni, insets, "getSystemWindowInsetBottom", "()I", Int);
    *left = glfm__callJavaMethod(jni, insets, "getSystemWindowInsetLeft", "()I", Int);

    (*jni)->DeleteLocalRef(jni, insets);
    return true;
}

// Calls activity.getWindow().getWindowManager().getDefaultDisplay()
static jobject glfm__getWindowDisplay(GLFMPlatformData *platformData) {
    JNIEnv *jni = platformData->jniEnv;
    jobject activity = platformData->activity->clazz;
    jobject window = glfm__callJavaMethod(jni, activity, "getWindow",
                                          "()Landroid/view/Window;", Object);
    if (glfm__wasJavaExceptionThrown(jni) || !window) {
        return NULL;
    }
    jobject windowManager = glfm__callJavaMethod(jni, window, "getWindowManager",
                                                 "()Landroid/view/WindowManager;", Object);
    (*jni)->DeleteLocalRef(jni, window);
    if (glfm__wasJavaExceptionThrown(jni) || !windowManager) {
        return NULL;
    }
    jobject windowDisplay = glfm__callJavaMethod(jni, windowManager, "getDefaultDisplay",
                                                 "()Landroid/view/Display;", Object);
    (*jni)->DeleteLocalRef(jni, windowManager);
    if (glfm__wasJavaExceptionThrown(jni)) {
        return NULL;
    } else {
        return windowDisplay;
    }
}

static float glfm__getRefreshRate(const GLFMDisplay *display) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    JNIEnv *jni = platformData->jniEnv;
    float refreshRate = -1;
    jobject windowDisplay = glfm__getWindowDisplay(platformData);
    if (windowDisplay) {
        refreshRate = glfm__callJavaMethod(jni, windowDisplay, "getRefreshRate","()F", Float);
        (*jni)->DeleteLocalRef(jni, windowDisplay);
    }
    if (glfm__wasJavaExceptionThrown(jni) || refreshRate <= 0) {
        return 60;
    }
    return refreshRate;
}

static bool glfm__updateSurfaceSizeIfNeeded(GLFMDisplay *display, bool force) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    int32_t width;
    int32_t height;
    eglQuerySurface(platformData->eglDisplay, platformData->eglSurface, EGL_WIDTH, &width);
    eglQuerySurface(platformData->eglDisplay, platformData->eglSurface, EGL_HEIGHT, &height);
    if (width != platformData->width || height != platformData->height) {
        if (force || platformData->resizeEventWaitFrames <= 0) {
            GLFM_LOG_LIFECYCLE("Resize: %i x %i", width, height);
            platformData->resizeEventWaitFrames = GLFM_RESIZE_EVENT_MAX_WAIT_FRAMES;
            platformData->refreshRequested = true;
            platformData->width = width;
            platformData->height = height;
            if (platformData->display && platformData->display->surfaceResizedFunc) {
                platformData->display->surfaceResizedFunc(platformData->display, width, height);
            }
            glfm__reportOrientationChangeIfNeeded(platformData->display);
            glfm__reportInsetsChangedIfNeeded(platformData->display);
            glfm__updateKeyboardVisibility(platformData);
            return true;
        } else {
            // Prefer to wait until after content rect changed, if possible
            platformData->resizeEventWaitFrames--;
        }
    }
    return false;
}

static void glfm__getDisplayChromeInsets(const GLFMDisplay *display, int *top, int *right,
                                         int *bottom, int *left) {

    bool success;
    if (glfmGetDisplayChrome(display) == GLFMUserInterfaceChromeNone) {
        success = glfm__getSafeInsets(display, top, right, bottom, left);
    } else {
        success = glfm__getSystemWindowInsets(display, top, right, bottom, left);
    }
    if (!success) {
        GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
        const ARect *contentRect = &platformData->contentRectArray[platformData->contentRectIndex];
        ARect visibleRect = glfm__getWindowVisibleDisplayFrame(platformData, contentRect);
        if (visibleRect.right - visibleRect.left <= 0 || visibleRect.bottom - visibleRect.top <= 0) {
            *top = 0;
            *right = 0;
            *bottom = 0;
            *left = 0;
        } else {
            *top = visibleRect.top;
            *right = platformData->width - visibleRect.right;
            *bottom = platformData->height - visibleRect.bottom;
            *left = visibleRect.left;
        }
    }
}

static void glfm__reportInsetsChangedIfNeeded(GLFMDisplay *display) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    int top, right, bottom, left;
    glfm__getDisplayChromeInsets(display, &top, &right, &bottom, &left);
    if (platformData->insets.top != top || platformData->insets.right != right ||
        platformData->insets.bottom != bottom || platformData->insets.left != left) {
        platformData->insets.top = top;
        platformData->insets.right = right;
        platformData->insets.bottom = bottom;
        platformData->insets.left = left;
        if (display->displayChromeInsetsChangedFunc && platformData->insets.valid) {
            display->displayChromeInsetsChangedFunc(display, (double)top, (double)top,
                                                    (double)bottom, (double)left);
        }
    }
    platformData->insets.valid = true;
}

static void glfm__reportOrientationChangeIfNeeded(GLFMDisplay *display) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    GLFMInterfaceOrientation orientation = glfmGetInterfaceOrientation(display);
    if (platformData->orientation != orientation) {
        platformData->orientation = orientation;
        platformData->refreshRequested = true;
        if (display->orientationChangedFunc) {
            display->orientationChangedFunc(display, orientation);
        }
    }
}

static void glfm__setOrientation(GLFMPlatformData *platformData) {
    static const int ActivityInfo_SCREEN_ORIENTATION_SENSOR = 0x00000004;
    static const int ActivityInfo_SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 0x00000006;
    static const int ActivityInfo_SCREEN_ORIENTATION_SENSOR_PORTRAIT = 0x00000007;

    if (!platformData || !platformData->activity) {
        return;
    }
    GLFMInterfaceOrientation orientations = platformData->display->supportedOrientations;
    bool portraitRequested = (
            ((uint8_t)orientations & (uint8_t)GLFMInterfaceOrientationPortrait) ||
            ((uint8_t)orientations & (uint8_t)GLFMInterfaceOrientationPortraitUpsideDown));
    bool landscapeRequested = ((uint8_t)orientations & (uint8_t)GLFMInterfaceOrientationLandscape);
    int orientation;
    if (portraitRequested && landscapeRequested) {
        orientation = ActivityInfo_SCREEN_ORIENTATION_SENSOR;
    } else if (landscapeRequested) {
        orientation = ActivityInfo_SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
    } else {
        orientation = ActivityInfo_SCREEN_ORIENTATION_SENSOR_PORTRAIT;
    }

    JNIEnv *jni = platformData->jniEnv;
    if ((*jni)->ExceptionCheck(jni)) {
        return;
    }

    glfm__callJavaMethodWithArgs(jni, platformData->activity->clazz, "setRequestedOrientation", "(I)V", Void, orientation);
    glfm__clearJavaException(jni);
}

static void glfm__displayChromeUpdated(GLFMDisplay *display) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    glfm__updateUserInterfaceChrome(platformData);
}

static const ASensor *glfm__getDeviceSensor(GLFMSensor sensor) {
    ASensorManager *sensorManager = ASensorManager_getInstance();
    switch (sensor) {
        case GLFMSensorAccelerometer:
            return ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
        case GLFMSensorMagnetometer:
            return ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);
        case GLFMSensorGyroscope:
            return ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_GYROSCOPE);
        case GLFMSensorRotationMatrix:
            return ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ROTATION_VECTOR);
        default:
            return NULL;
    }
}

static void glfm__setAllRequestedSensorsEnabled(GLFMDisplay *display, bool enabledGlobally) {
    if (!display) {
        return;
    }
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    for (int i = 0; i < GLFM_NUM_SENSORS; i++) {
        GLFMSensor sensor = (GLFMSensor)i;
        const ASensor *deviceSensor = glfm__getDeviceSensor(sensor);
        bool isNeededEnabled = display->sensorFuncs[i] != NULL;
        bool shouldEnable = enabledGlobally && isNeededEnabled;
        bool isEnabled = platformData->deviceSensorEnabled[i];
        if (!shouldEnable) {
            platformData->sensorEventValid[i] = false;
        }

        if (isEnabled == shouldEnable || deviceSensor == NULL) {
            continue;
        }
        if (platformData->sensorEventQueue == NULL) {
            ASensorManager *sensorManager = ASensorManager_getInstance();
            platformData->sensorEventQueue = ASensorManager_createEventQueue(sensorManager,
                    ALooper_forThread(), GLFMLooperIDSensor, NULL, NULL);
            if (!platformData->sensorEventQueue) {
                continue;
            }
        }
        if (shouldEnable && !isEnabled) {
            if (ASensorEventQueue_enableSensor(platformData->sensorEventQueue, deviceSensor) == 0) {
                int minDelay = ASensor_getMinDelay(deviceSensor);
                if (minDelay > 0) {
                    int delay = GLFM_SENSOR_UPDATE_INTERVAL_MICROS;
                    if (delay < minDelay) {
                        delay = minDelay;
                    }
                    ASensorEventQueue_setEventRate(platformData->sensorEventQueue, deviceSensor, delay);
                }
                platformData->deviceSensorEnabled[i] = true;
            }
        } else if (!shouldEnable && isEnabled) {
            if (ASensorEventQueue_disableSensor(platformData->sensorEventQueue, deviceSensor) == 0) {
                platformData->deviceSensorEnabled[i] = false;
            }
        }
    }
}

static void glfm__sensorFuncUpdated(GLFMDisplay *display) {
    if (display) {
        GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
        glfm__setAllRequestedSensorsEnabled(display, platformData->animating);
    }
}

/// Gets an Android system service. The "serviceName" is a field from android.content.Context,
/// like "INPUT_METHOD_SERVICE" or "VIBRATOR_SERVICE".
///
/// The C code:
///     glfm__getSystemService(platformData, "INPUT_METHOD_SERVICE")
/// will invoke the java code:
///     activity.getSystemService(Context.INPUT_METHOD_SERVICE);
static jobject glfm__getSystemService(GLFMPlatformData *platformData, const char *serviceName) {
    JNIEnv *jni = platformData->jniEnv;
    jclass contextClass = (*jni)->FindClass(jni, "android/content/Context");
    if (glfm__wasJavaExceptionThrown(jni)) {
        return NULL;
    }

    jstring serviceNameString = glfm__getJavaStaticField(jni, contextClass, serviceName,
                                                         "Ljava/lang/String;", Object);
    (*jni)->DeleteLocalRef(jni, contextClass);
    if (glfm__wasJavaExceptionThrown(jni) || !serviceNameString) {
        return NULL;
    }
    jobject service = glfm__callJavaMethodWithArgs(jni, platformData->activity->clazz,
                                                   "getSystemService",
                                                   "(Ljava/lang/String;)Ljava/lang/Object;",
                                                   Object, serviceNameString);
    (*jni)->DeleteLocalRef(jni, serviceNameString);
    if (glfm__wasJavaExceptionThrown(jni)) {
        return NULL;
    } else {
        return service;
    }
}

static bool glfm__setKeyboardVisible(GLFMPlatformData *platformData, bool visible) {
    static const int InputMethodManager_SHOW_FORCED = 2;

    JNIEnv *jni = platformData->jniEnv;
    if ((*jni)->ExceptionCheck(jni)) {
        return false;
    }

    jobject decorView = glfm__getDecorView(jni, platformData);
    if (!decorView) {
        return false;
    }

    jobject ime = glfm__getSystemService(platformData, "INPUT_METHOD_SERVICE");
    if (!ime) {
        return false;
    }
    if (visible) {
        int flags = 0;
        if (platformData->activity->sdkVersion < 23) {
            // This flag was deprecated in API 33. It was required for older versions of Android,
            // but it kept the soft keyboard open when leaving the app. At some point, the flag was
            // no longer required (possibly for versions prior to 23.)
            flags = InputMethodManager_SHOW_FORCED;
        }
        glfm__callJavaMethodWithArgs(jni, ime, "showSoftInput", "(Landroid/view/View;I)Z", Boolean,
                                     decorView, flags);
    } else {
        jobject windowToken = glfm__callJavaMethod(jni, decorView, "getWindowToken", "()Landroid/os/IBinder;", Object);
        if (glfm__wasJavaExceptionThrown(jni) || !windowToken) {
            return false;
        }
        glfm__callJavaMethodWithArgs(jni, ime, "hideSoftInputFromWindow",
                                     "(Landroid/os/IBinder;I)Z", Boolean, windowToken, 0);
        (*jni)->DeleteLocalRef(jni, windowToken);
    }

    (*jni)->DeleteLocalRef(jni, ime);
    (*jni)->DeleteLocalRef(jni, decorView);

    return !glfm__wasJavaExceptionThrown(jni);
}

static void glfm__updateKeyboardVisibility(GLFMPlatformData *platformData) {
    if (platformData->display) {
        const ARect *contentRect = &platformData->contentRectArray[platformData->contentRectIndex];
        ARect windowRect = glfm__getDecorViewRect(platformData, contentRect);
        ARect visibleRect = glfm__getWindowVisibleDisplayFrame(platformData, &windowRect);
        ARect nonVisibleRect[4];

        // Left
        nonVisibleRect[0].left = windowRect.left;
        nonVisibleRect[0].right = visibleRect.left;
        nonVisibleRect[0].top = windowRect.top;
        nonVisibleRect[0].bottom = windowRect.bottom;

        // Right
        nonVisibleRect[1].left = visibleRect.right;
        nonVisibleRect[1].right = windowRect.right;
        nonVisibleRect[1].top = windowRect.top;
        nonVisibleRect[1].bottom = windowRect.bottom;

        // Top
        nonVisibleRect[2].left = windowRect.left;
        nonVisibleRect[2].right = windowRect.right;
        nonVisibleRect[2].top = windowRect.top;
        nonVisibleRect[2].bottom = visibleRect.top;

        // Bottom
        nonVisibleRect[3].left = windowRect.left;
        nonVisibleRect[3].right = windowRect.right;
        nonVisibleRect[3].top = visibleRect.bottom;
        nonVisibleRect[3].bottom = windowRect.bottom;

        // Find largest with minimum keyboard size
        const int minimumKeyboardSize = (int)(100 * platformData->scale);
        int largestIndex = 0;
        int largestArea = -1;
        for (int i = 0; i < 4; i++) {
            int w = nonVisibleRect[i].right - nonVisibleRect[i].left;
            int h = nonVisibleRect[i].bottom - nonVisibleRect[i].top;
            int area = w * h;
            if (w >= minimumKeyboardSize && h >= minimumKeyboardSize && area > largestArea) {
                largestIndex = i;
                largestArea = area;
            }
        }

        bool keyboardVisible = largestArea > 0;
        ARect keyboardFrame = keyboardVisible ? nonVisibleRect[largestIndex] : (ARect){0, 0, 0, 0};

        // Send update notification
        if (platformData->keyboardVisible != keyboardVisible ||
            platformData->keyboardFrame.left != keyboardFrame.left ||
            platformData->keyboardFrame.top != keyboardFrame.top ||
            platformData->keyboardFrame.right != keyboardFrame.right ||
            platformData->keyboardFrame.bottom != keyboardFrame.bottom) {
            if (platformData->keyboardVisible != keyboardVisible) {
                glfm__updateUserInterfaceChrome(platformData);
            }
            platformData->keyboardVisible = keyboardVisible;
            platformData->keyboardFrame = keyboardFrame;
            platformData->refreshRequested = true;
            if (platformData->display->keyboardVisibilityChangedFunc) {
                double x = keyboardFrame.left;
                double y = keyboardFrame.top;
                double w = keyboardFrame.right - keyboardFrame.left;
                double h = keyboardFrame.bottom - keyboardFrame.top;
                platformData->display->keyboardVisibilityChangedFunc(platformData->display,
                                                                     keyboardVisible,
                                                                     x, y, w, h);
            }
        }
    }
}

// MARK: - GLFM public functions

double glfmGetTime() {
    static int clockID;
    static time_t initTime;
    static bool initialized = false;

    struct timespec time;

    if (!initialized) {
        if (clock_gettime(CLOCK_MONOTONIC_RAW, &time) == 0) {
            clockID = CLOCK_MONOTONIC_RAW;
        } else if (clock_gettime(CLOCK_MONOTONIC, &time) == 0) {
            clockID = CLOCK_MONOTONIC;
        } else {
            clock_gettime(CLOCK_REALTIME, &time);
            clockID = CLOCK_REALTIME;
        }
        initTime = time.tv_sec;
        initialized = true;
    } else {
        clock_gettime(clockID, &time);
    }
    // Subtract by initTime to ensure that conversion to double keeps nanosecond accuracy
    return (double)(time.tv_sec - initTime) + (double)time.tv_nsec / 1e9;
}

void glfmSwapBuffers(GLFMDisplay *display) {
    if (display) {
        GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
        EGLBoolean result = eglSwapBuffers(platformData->eglDisplay, platformData->eglSurface);
        platformData->swapCalled = true;
        platformData->lastSwapTime = glfmGetTime();
        if (!result) {
            glfm__eglCheckError(platformData);
        }
    }
}

void glfmSetSupportedInterfaceOrientation(GLFMDisplay *display, GLFMInterfaceOrientation supportedOrientations) {
    if (display && display->supportedOrientations != supportedOrientations) {
        display->supportedOrientations = supportedOrientations;
        GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
        glfm__setOrientation(platformData);
    }
}

GLFMInterfaceOrientation glfmGetInterfaceOrientation(const GLFMDisplay *display) {
    static const int Surface_ROTATION_0 = 0;
    static const int Surface_ROTATION_90 = 1;
    static const int Surface_ROTATION_180 = 2;
    static const int Surface_ROTATION_270 = 3;

    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    JNIEnv *jni = platformData->jniEnv;
    jobject windowDisplay = glfm__getWindowDisplay(platformData);
    if (!windowDisplay) {
        return GLFMInterfaceOrientationUnknown;
    }
    int rotation = glfm__callJavaMethod(jni, windowDisplay, "getRotation","()I", Int);
    (*jni)->DeleteLocalRef(jni, windowDisplay);
    if (glfm__wasJavaExceptionThrown(jni)) {
        return GLFMInterfaceOrientationUnknown;
    }

    if (rotation == Surface_ROTATION_0) {
        return GLFMInterfaceOrientationPortrait;
    } else if (rotation == Surface_ROTATION_90) {
        return GLFMInterfaceOrientationLandscapeRight;
    } else if (rotation == Surface_ROTATION_180) {
        return GLFMInterfaceOrientationPortraitUpsideDown;
    } else if (rotation == Surface_ROTATION_270) {
        return GLFMInterfaceOrientationLandscapeLeft;
    } else {
        return GLFMInterfaceOrientationUnknown;
    }
}

void glfmGetDisplaySize(const GLFMDisplay *display, int *width, int *height) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    if (width) *width = platformData->width;
    if (height) *height = platformData->height;
}

double glfmGetDisplayScale(const GLFMDisplay *display) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    return platformData->scale;
}

void glfmGetDisplayChromeInsets(const GLFMDisplay *display, double *top, double *right,
                                double *bottom, double *left) {
    int intTop, intRight, intBottom, intLeft;
    glfm__getDisplayChromeInsets(display, &intTop, &intRight, &intBottom, &intLeft);
    if (top) *top = (double)intTop;
    if (right) *right = (double)intRight;
    if (bottom) *bottom = (double)intBottom;
    if (left) *left = (double)intLeft;
}

GLFMRenderingAPI glfmGetRenderingAPI(const GLFMDisplay *display) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    return platformData->renderingAPI;
}

bool glfmHasTouch(const GLFMDisplay *display) {
    (void)display;
    // This will need to change, for say, TV apps
    return true;
}

void glfmSetMouseCursor(GLFMDisplay *display, GLFMMouseCursor mouseCursor) {
    (void)display;
    (void)mouseCursor;
    // Do nothing
}

void glfmSetMultitouchEnabled(GLFMDisplay *display, bool multitouchEnabled) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    platformData->multitouchEnabled = multitouchEnabled;
}

bool glfmGetMultitouchEnabled(const GLFMDisplay *display) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    return platformData->multitouchEnabled;
}

GLFMProc glfmGetProcAddress(const char *functionName) {
    GLFMProc function = eglGetProcAddress(functionName);
    if (!function) {
        static void *handle = NULL;
        if (!handle) {
            handle = dlopen(NULL, RTLD_LAZY);
        }
        function = handle ? (GLFMProc)dlsym(handle, functionName) : NULL;
    }
    return function;
}

bool glfmHasVirtualKeyboard(const GLFMDisplay *display) {
    (void)display;
    return true;
}

void glfmSetKeyboardVisible(GLFMDisplay *display, bool visible) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    if (glfm__setKeyboardVisible(platformData, visible)) {
        glfm__updateUserInterfaceChrome(platformData);
    }
}

bool glfmIsKeyboardVisible(const GLFMDisplay *display) {
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    return platformData->keyboardVisible;
}

bool glfmIsSensorAvailable(const GLFMDisplay *display, GLFMSensor sensor) {
    (void)display;
    return glfm__getDeviceSensor(sensor) != NULL;
}

bool glfmIsHapticFeedbackSupported(const GLFMDisplay *display) {
    /*
    Vibrator vibrator = (Vibrator)context.getSystemService(Context.VIBRATOR_SERVICE);
    return vibrator ? vibrator.hasVibrator() : false;
    */
    if (!display) {
        return false;
    }
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    JNIEnv *jni = platformData->jniEnv;
    if ((*jni)->ExceptionCheck(jni)) {
        return false;
    }
    jobject vibratorService = glfm__getSystemService(platformData, "VIBRATOR_SERVICE");
    if (!vibratorService) {
        return false;
    }
    jboolean result = glfm__callJavaMethod(jni, vibratorService, "hasVibrator", "()Z", Boolean);
    (*jni)->DeleteLocalRef(jni, vibratorService);
    if (glfm__wasJavaExceptionThrown(jni)) {
        return false;
    } else {
        return result;
    }
}

void glfmPerformHapticFeedback(GLFMDisplay *display, GLFMHapticFeedbackStyle style) {
    // decorView.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP, flags);
    static const jint HapticFeedbackConstants_CONTEXT_CLICK = 6; // Light, API 23
    static const jint HapticFeedbackConstants_VIRTUAL_KEY = 1; // Medium
    static const jint HapticFeedbackConstants_LONG_PRESS = 0; // Heavy
    static const jint HapticFeedbackConstants_REJECT = 17; // Heavy, API 30
    static const jint HapticFeedbackConstants_FLAG_IGNORE_VIEW_SETTING = 0x01;
    static const jint HapticFeedbackConstants_FLAG_IGNORE_GLOBAL_SETTING = 0x02;

    if (!display) {
        return;
    }
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    JNIEnv *jni = platformData->jniEnv;
    if ((*jni)->ExceptionCheck(jni)) {
        return;
    }

    const int SDK_INT = platformData->activity->sdkVersion;
    jint defaultFeedbackConstant = HapticFeedbackConstants_LONG_PRESS;
    jint feedbackConstant;
    jint feedbackFlags = HapticFeedbackConstants_FLAG_IGNORE_VIEW_SETTING | HapticFeedbackConstants_FLAG_IGNORE_GLOBAL_SETTING;
    switch (style) {
        case GLFMHapticFeedbackLight: default:
            if (SDK_INT < 23) {
                feedbackConstant = HapticFeedbackConstants_VIRTUAL_KEY;
            } else {
                feedbackConstant = HapticFeedbackConstants_CONTEXT_CLICK;
            }
            break;
        case GLFMHapticFeedbackMedium:
            feedbackConstant = HapticFeedbackConstants_VIRTUAL_KEY;
            break;
        case GLFMHapticFeedbackHeavy:
            if (SDK_INT < 30) {
                feedbackConstant = HapticFeedbackConstants_LONG_PRESS;
            } else {
                feedbackConstant = HapticFeedbackConstants_REJECT;
            }
            break;
    }

    jobject decorView = glfm__getDecorView(jni, platformData);
    if (!decorView) {
        return;
    }
    bool performed = glfm__callJavaMethodWithArgs(jni, decorView, "performHapticFeedback", "(II)Z", Boolean, feedbackConstant, feedbackFlags);
    if (!performed) {
        // Some devices (Samsung S8) don't support all constants
        glfm__callJavaMethodWithArgs(jni, decorView, "performHapticFeedback", "(II)Z", Boolean, defaultFeedbackConstant, feedbackFlags);
    }
    (*jni)->DeleteLocalRef(jni, decorView);
}

bool glfmHasClipboardText(const GLFMDisplay *display) {
    if (!display || !display->platformData) {
        return false;
    }
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    JNIEnv *jni = platformData->jniEnv;

    // ClipboardManager clipboardManager = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
    jobject clipboardManager = glfm__getSystemService(platformData, "CLIPBOARD_SERVICE");
    if (!clipboardManager) {
        return false;
    }

    // Invoke clipboardManager.getPrimaryClipDescription()
    jobject primaryClipDescription = glfm__callJavaMethod(jni, clipboardManager, "getPrimaryClipDescription",
                                                   "()Landroid/content/ClipDescription;", Object);
    (*jni)->DeleteLocalRef(jni, clipboardManager);
    if (glfm__wasJavaExceptionThrown(jni) || !primaryClipDescription) {
        return false;
    }

    // Invoke primaryClipDescription.hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN)
    jclass class = (*jni)->GetObjectClass(jni, primaryClipDescription);
    jobject mimeType = glfm__getJavaStaticField(jni, class, "MIMETYPE_TEXT_PLAIN",
                                                "Ljava/lang/String;", Object);
    (*jni)->DeleteLocalRef(jni, class);
    if (glfm__wasJavaExceptionThrown(jni) || !mimeType) {
        return false;
    }
    jboolean hasText = glfm__callJavaMethodWithArgs(jni, primaryClipDescription, "hasMimeType",
                                                    "(Ljava/lang/String;)Z", Boolean, mimeType);

    (*jni)->DeleteLocalRef(jni, primaryClipDescription);
    if (glfm__wasJavaExceptionThrown(jni)) {
        return false;
    } else {
        return hasText;
    }
}

void glfmRequestClipboardText(GLFMDisplay *display, GLFMClipboardTextFunc clipboardTextFunc) {
    if (!clipboardTextFunc) {
        return;
    }

    // First check glfmHasClipboardText(), to prevent a toast from being show if there is something
    // other than text in the clipboard
    if (!display || !display->platformData || !glfmHasClipboardText(display)) {
        clipboardTextFunc(display, NULL);
    }
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    JNIEnv *jni = platformData->jniEnv;

    // ClipboardManager clipboardManager = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
    jobject clipboardManager = glfm__getSystemService(platformData, "CLIPBOARD_SERVICE");
    if (!clipboardManager) {
        clipboardTextFunc(display, NULL);
        return;
    }

    // Invoke clipboardManager.getPrimaryClip()?.getItemAt(0)?.getText()?.toString()
    // Note, there appears no reason to do this asynchronously.
    jobject clipData = glfm__callJavaMethod(jni, clipboardManager, "getPrimaryClip",
                                            "()Landroid/content/ClipData;", Object);
    (*jni)->DeleteLocalRef(jni, clipboardManager);
    if (glfm__wasJavaExceptionThrown(jni) || !clipData) {
        clipboardTextFunc(display, NULL);
        return;
    }
    jobject clipDataItem = glfm__callJavaMethodWithArgs(jni, clipData, "getItemAt",
                                                        "(I)Landroid/content/ClipData$Item;",
                                                        Object, 0);
    (*jni)->DeleteLocalRef(jni, clipData);
    if (glfm__wasJavaExceptionThrown(jni) || !clipDataItem) {
        clipboardTextFunc(display, NULL);
        return;
    }
    jobject clipDataItemText = glfm__callJavaMethod(jni, clipDataItem, "getText",
                                                    "()Ljava/lang/CharSequence;", Object);
    (*jni)->DeleteLocalRef(jni, clipDataItem);
    if (glfm__wasJavaExceptionThrown(jni) || !clipDataItemText) {
        clipboardTextFunc(display, NULL);
        return;
    }
    jstring javaString = glfm__callJavaMethod(jni, clipDataItemText, "toString",
                                              "()Ljava/lang/String;", Object);
    (*jni)->DeleteLocalRef(jni, clipDataItemText);
    if (glfm__wasJavaExceptionThrown(jni) || !javaString) {
        clipboardTextFunc(display, NULL);
        return;
    }

    // Convert Java string to C string
    const char *cString = (*jni)->GetStringUTFChars(jni, javaString, NULL);
    if (glfm__wasJavaExceptionThrown(jni) || !cString) {
        clipboardTextFunc(display, NULL);
        return;
    }

    // Call
    clipboardTextFunc(display, cString);

    // Cleanup
    (*jni)->ReleaseStringUTFChars(jni, javaString, cString);
    (*jni)->DeleteLocalRef(jni, javaString);
}

bool glfmSetClipboardText(GLFMDisplay *display, const char *string) {
    if (!string || !display || !display->platformData) {
        return false;
    }
    GLFMPlatformData *platformData = (GLFMPlatformData *)display->platformData;
    JNIEnv *jni = platformData->jniEnv;

    // Convert C string to java String
    jstring javaString = (*jni)->NewStringUTF(jni, string);
    if (glfm__wasJavaExceptionThrown(jni) || !javaString) {
        return false;
    }

    // Create ClipData
    // ClipData clipData = ClipData.newPlainText("simple text", javaString);
    jclass clipDataClass = (*jni)->FindClass(jni, "android/content/ClipData");
    if (glfm__wasJavaExceptionThrown(jni) || !clipDataClass) {
        (*jni)->DeleteLocalRef(jni, javaString);
        return false;
    }
    jstring label = (*jni)->NewStringUTF(jni, "simple text");
    if (glfm__wasJavaExceptionThrown(jni) || !label) {
        (*jni)->DeleteLocalRef(jni, clipDataClass);
        (*jni)->DeleteLocalRef(jni, javaString);
        return false;
    }
    jobject clipData = glfm__callJavaStaticMethodWithArgs(jni, clipDataClass, "newPlainText",
                                                          "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;",
                                                          Object, label, javaString);
    (*jni)->DeleteLocalRef(jni, clipDataClass);
    (*jni)->DeleteLocalRef(jni, javaString);
    if (glfm__wasJavaExceptionThrown(jni) || !clipData) {
        return false;
    }

    // Set the clipboard text
    // ClipboardManager clipboardManager = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
    // clipboardManager.setPrimaryClip(clipData);
    jobject clipboardManager = glfm__getSystemService(platformData, "CLIPBOARD_SERVICE");
    if (glfm__wasJavaExceptionThrown(jni) || !clipboardManager) {
        (*jni)->DeleteLocalRef(jni, clipData);
        return false;
    }
    glfm__callJavaMethodWithArgs(jni, clipboardManager, "setPrimaryClip",
                                 "(Landroid/content/ClipData;)V", Void, clipData);
    (*jni)->DeleteLocalRef(jni, clipData);
    (*jni)->DeleteLocalRef(jni, clipboardManager);

    return !glfm__wasJavaExceptionThrown(jni);
}

// MARK: - Platform-specific functions

bool glfmIsMetalSupported(const GLFMDisplay *display) {
    (void)display;
    return false;
}

ANativeActivity *glfmAndroidGetActivity() {
    if (platformDataGlobal) {
        return platformDataGlobal->activity;
    } else {
        return NULL;
    }
}

ANativeActivity *glfmGetAndroidActivity(const GLFMDisplay *display) {
    if (display && display->platformData) {
        GLFMPlatformData *platformData = display->platformData;
        return platformData->activity;
    } else {
        return NULL;
    }
}

#endif // __ANDROID__
