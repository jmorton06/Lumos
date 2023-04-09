// GLFM
// https://github.com/brackeen/glfm

#if defined(__EMSCRIPTEN__)

#include "glfm.h"

#include <EGL/egl.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "glfm_internal.h"

#ifdef NDEBUG
#  define GLFM_LOG(...) do { } while (0)
#else
#  define GLFM_LOG(...) do { printf("%.3f: ", glfmGetTime()); printf(__VA_ARGS__); printf("\n"); } while (0)
#endif

#define GLFM_MAX_ACTIVE_TOUCHES 10

// If 1, test if keyboard event arrays are sorted.
#define GLFM_TEST_KEYBOARD_EVENT_ARRAYS 0

typedef struct {
    long identifier;
    bool active;
} GLFMActiveTouch;

typedef struct {
    bool multitouchEnabled;
    int32_t width;
    int32_t height;
    double scale;
    GLFMRenderingAPI renderingAPI;

    bool mouseDown;
    GLFMActiveTouch activeTouches[GLFM_MAX_ACTIVE_TOUCHES];

    bool isVisible;
    bool isFocused;
    bool refreshRequested;
    
    GLFMInterfaceOrientation orientation;
} GLFMPlatformData;

// MARK: - GLFM private functions

#if GLFM_TEST_KEYBOARD_EVENT_ARRAYS

static bool glfm__listIsSorted(const char *list[], size_t size) {
    for (size_t i = 1; i < size; i++) {
        if (strcmp(list[i - 1], list[i]) > 0) {
            return false;
        }
    }
    return true;
}

#endif

static int glfm__sortedListSearch(const char *list[], size_t size, const char *word) {
    int left = 0;
    int right = (int)size - 1;

    while (left <= right) {
        int i = (left + right) / 2;
        int result = strcmp(list[i], word);
        if (result > 0) {
            right = i - 1;
        } else if (result < 0) {
            left = i + 1;
        } else {
            return i;
        }
    }
    return -1;
}

static void glfm__clearActiveTouches(GLFMPlatformData *platformData) {
    for (int i = 0; i < GLFM_MAX_ACTIVE_TOUCHES; i++) {
        platformData->activeTouches[i].active = false;
    }
}

static void glfm__displayChromeUpdated(GLFMDisplay *display) {
    (void)display;
}

void glfm__sensorFuncUpdated(GLFMDisplay *display) {
    (void)display;
    // TODO: Sensors
}

EMSCRIPTEN_KEEPALIVE extern
void glfm__requestClipboardTextCallback(GLFMDisplay *display,
                                        GLFMClipboardTextFunc clipboardTextFunc, const char *text);

void glfm__requestClipboardTextCallback(GLFMDisplay *display,
                                        GLFMClipboardTextFunc clipboardTextFunc, const char *text) {
    if (text && text[0] != '\0') {
        clipboardTextFunc(display, text);
    } else {
        clipboardTextFunc(display, NULL);
    }
}

// MARK: - GLFM public functions

double glfmGetTime(void) {
    return emscripten_get_now() / 1000.0;
}

void glfmSwapBuffers(GLFMDisplay *display) {
    (void)display;
    // Do nothing; swap is implicit
}

void glfmSetSupportedInterfaceOrientation(GLFMDisplay *display,
                                          GLFMInterfaceOrientation supportedOrientations) {
    if (display->supportedOrientations != supportedOrientations) {
        display->supportedOrientations = supportedOrientations;

        bool portraitRequested = (supportedOrientations & (GLFMInterfaceOrientationPortrait | GLFMInterfaceOrientationPortraitUpsideDown));
        bool landscapeRequested = (supportedOrientations & GLFMInterfaceOrientationLandscape);
        if (portraitRequested && landscapeRequested) {
            emscripten_lock_orientation(EMSCRIPTEN_ORIENTATION_PORTRAIT_PRIMARY |
                                        EMSCRIPTEN_ORIENTATION_PORTRAIT_SECONDARY |
                                        EMSCRIPTEN_ORIENTATION_LANDSCAPE_PRIMARY |
                                        EMSCRIPTEN_ORIENTATION_LANDSCAPE_SECONDARY);
        } else if (landscapeRequested) {
            emscripten_lock_orientation(EMSCRIPTEN_ORIENTATION_LANDSCAPE_PRIMARY |
                                        EMSCRIPTEN_ORIENTATION_LANDSCAPE_SECONDARY);
        } else {
            emscripten_lock_orientation(EMSCRIPTEN_ORIENTATION_PORTRAIT_PRIMARY |
                                        EMSCRIPTEN_ORIENTATION_PORTRAIT_SECONDARY);
        }
    }
}

GLFMInterfaceOrientation glfmGetInterfaceOrientation(const GLFMDisplay *display) {
    (void)display;
    
    EmscriptenOrientationChangeEvent orientationStatus;
    emscripten_get_orientation_status(&orientationStatus);
    int orientation = orientationStatus.orientationIndex;
    int angle = orientationStatus.orientationAngle;
    
    if (orientation == EMSCRIPTEN_ORIENTATION_PORTRAIT_PRIMARY) {
        return GLFMInterfaceOrientationPortrait;
    } else if (orientation == EMSCRIPTEN_ORIENTATION_PORTRAIT_SECONDARY) {
        return GLFMInterfaceOrientationPortraitUpsideDown;
    } else if (orientation == EMSCRIPTEN_ORIENTATION_LANDSCAPE_PRIMARY ||
               orientation == EMSCRIPTEN_ORIENTATION_LANDSCAPE_SECONDARY) {
        if (angle == 0 || angle == 90) {
            return GLFMInterfaceOrientationLandscapeRight;
        } else if (angle == 180 || angle == 270) {
            return GLFMInterfaceOrientationLandscapeLeft;
        } else {
            return GLFMInterfaceOrientationUnknown;
        }
    } else {
        return GLFMInterfaceOrientationUnknown;
    }
}

extern EMSCRIPTEN_RESULT emscripten_get_orientation_status(EmscriptenOrientationChangeEvent *orientationStatus);

void glfmGetDisplaySize(const GLFMDisplay *display, int *width, int *height) {
    GLFMPlatformData *platformData = display->platformData;
    if (width) *width = platformData->width;
    if (height) *height = platformData->height;
}

double glfmGetDisplayScale(const GLFMDisplay *display) {
    GLFMPlatformData *platformData = display->platformData;
    return platformData->scale;
}

void glfmGetDisplayChromeInsets(const GLFMDisplay *display, double *top, double *right,
                                double *bottom, double *left) {
    GLFMPlatformData *platformData = display->platformData;

    if (top) *top = platformData->scale * EM_ASM_DOUBLE_V( {
        var htmlStyles = window.getComputedStyle(document.querySelector("html"));
        return (parseInt(htmlStyles.getPropertyValue("--glfm-chrome-top-old")) || 0) +
               (parseInt(htmlStyles.getPropertyValue("--glfm-chrome-top")) || 0);
    } );
    if (right) *right = platformData->scale * EM_ASM_DOUBLE_V( {
        var htmlStyles = window.getComputedStyle(document.querySelector("html"));
        return (parseInt(htmlStyles.getPropertyValue("--glfm-chrome-right-old")) || 0) +
               (parseInt(htmlStyles.getPropertyValue("--glfm-chrome-right")) || 0);
    } );
    if (bottom) *bottom = platformData->scale * EM_ASM_DOUBLE_V( {
        var htmlStyles = window.getComputedStyle(document.querySelector("html"));
        return (parseInt(htmlStyles.getPropertyValue("--glfm-chrome-bottom-old")) || 0) +
               (parseInt(htmlStyles.getPropertyValue("--glfm-chrome-bottom")) || 0);
    } );
    if (left) *left = platformData->scale * EM_ASM_DOUBLE_V( {
        var htmlStyles = window.getComputedStyle(document.querySelector("html"));
        return (parseInt(htmlStyles.getPropertyValue("--glfm-chrome-left-old")) || 0) +
               (parseInt(htmlStyles.getPropertyValue("--glfm-chrome-left")) || 0);
    } );
}

GLFMRenderingAPI glfmGetRenderingAPI(const GLFMDisplay *display) {
    GLFMPlatformData *platformData = display->platformData;
    return platformData->renderingAPI;
}

bool glfmHasTouch(const GLFMDisplay *display) {
    (void)display;
    return EM_ASM_INT_V({
        return (('ontouchstart' in window) || (navigator.msMaxTouchPoints > 0));
    });
}

void glfmSetMouseCursor(GLFMDisplay *display, GLFMMouseCursor mouseCursor) {
    (void)display;
    // Make sure the javascript array emCursors is referenced properly
    int emCursor = 0;
    switch (mouseCursor) {
        case GLFMMouseCursorAuto:
            emCursor = 0;
            break;
        case GLFMMouseCursorNone:
            emCursor = 1;
            break;
        case GLFMMouseCursorDefault:
            emCursor = 2;
            break;
        case GLFMMouseCursorPointer:
            emCursor = 3;
            break;
        case GLFMMouseCursorCrosshair:
            emCursor = 4;
            break;
        case GLFMMouseCursorText:
            emCursor = 5;
            break;
        case GLFMMouseCursorVerticalText:
            emCursor = 6;
            break;
    }
    EM_ASM_({
        var emCursors = new Array('auto', 'none', 'default', 'pointer', 'crosshair', 'text', 'vertical-text');
        Module['canvas'].style.cursor = emCursors[$0];
    }, emCursor);
}

void glfmSetMultitouchEnabled(GLFMDisplay *display, bool multitouchEnabled) {
    GLFMPlatformData *platformData = display->platformData;
    platformData->multitouchEnabled = multitouchEnabled;
}

bool glfmGetMultitouchEnabled(const GLFMDisplay *display) {
    GLFMPlatformData *platformData = display->platformData;
    return platformData->multitouchEnabled;
}

bool glfmHasVirtualKeyboard(const GLFMDisplay *display) {
    (void)display;
    return false;
}

void glfmSetKeyboardVisible(GLFMDisplay *display, bool visible) {
    (void)display;
    (void)visible;
    // Do nothing
}

bool glfmIsKeyboardVisible(const GLFMDisplay *display) {
    (void)display;
    return false;
}

GLFMProc glfmGetProcAddress(const char *functionName) {
    return eglGetProcAddress(functionName);
}

bool glfmIsSensorAvailable(const GLFMDisplay *display, GLFMSensor sensor) {
    (void)display;
    (void)sensor;
    // TODO: Sensors
    return false;
}

bool glfmIsHapticFeedbackSupported(const GLFMDisplay *display) {
    (void)display;
    return false;
}

void glfmPerformHapticFeedback(GLFMDisplay *display, GLFMHapticFeedbackStyle style) {
    (void)display;
    (void)style;
    // Do nothing
}

bool glfmHasClipboardText(const GLFMDisplay *display) {
    (void)display;
    // Currently, chrome supports navigator.userActivation, but Safari and Firefox do not.
    int result = EM_ASM_INT({
        var hasReadText = (navigator && navigator.clipboard && navigator.clipboard.readText);
        var hasUserActivation = (navigator && navigator.userActivation) ? navigator.userActivation.isActive : true;
        return (hasReadText && hasUserActivation) ? 1 : 0;
    });
    return result != 0;
}

void glfmRequestClipboardText(GLFMDisplay *display, GLFMClipboardTextFunc clipboardTextFunc) {
    if (!clipboardTextFunc) {
        return;
    }
    if (!glfmHasClipboardText(display)) {
        clipboardTextFunc(display, NULL);
        return;
    }

    EM_ASM({
        if (!navigator.clipboard || !navigator.clipboard.readText) {
            _glfm__requestClipboardTextCallback($0, $1, null);
            return;
        }
        var promise = navigator.clipboard.readText();
        if (promise === undefined) {
            _glfm__requestClipboardTextCallback($0, $1, null);
            return;
        }
        var errorCaught = false;
        promise.catch(error => {
            _glfm__requestClipboardTextCallback($0, $1, null);
            errorCaught = true;
        }).then((clipText) => {
            if (clipText === undefined || clipText.length == 0) {
                if (!errorCaught) {
                    _glfm__requestClipboardTextCallback($0, $1, null);
                }
                return;
            }
            var len = lengthBytesUTF8(clipText);
            var buffer = _malloc(len + 1);
            if (buffer) {
                stringToUTF8(clipText, buffer, len + 1);
                _glfm__requestClipboardTextCallback($0, $1, buffer);
                _free(buffer);
            } else {
                _glfm__requestClipboardTextCallback($0, $1, null);
            }
        });
    }, display, clipboardTextFunc);
}

bool glfmSetClipboardText(GLFMDisplay *display, const char *string) {
    (void)display;
    if (!string) {
        return false;
    }
    int result = EM_ASM_INT({
        if (navigator.clipboard && navigator.clipboard.writeText) {
            var text = UTF8ToString($0);
            if (text) {
                navigator.clipboard.writeText(text);
                return 1;
            }
        }
        return 0;
    }, string);
    return result == 1;
}

// MARK: - Platform-specific functions

bool glfmIsMetalSupported(const GLFMDisplay *display) {
    (void)display;
    return false;
}

// MARK: - Emscripten glue

static int glfm__getDisplayWidth(GLFMDisplay *display) {
    (void)display;
    const double width = EM_ASM_DOUBLE_V({
        var canvas = Module['canvas'];
        return canvas.width;
    });
    return (int)(round(width));
}

static int glfm__getDisplayHeight(GLFMDisplay *display) {
    (void)display;
    const double height = EM_ASM_DOUBLE_V({
        var canvas = Module['canvas'];
        return canvas.height;
    });
    return (int)(round(height));
}

static void glfm__setVisibleAndFocused(GLFMDisplay *display, bool visible, bool focused) {
    GLFMPlatformData *platformData = display->platformData;
    bool wasActive = platformData->isVisible && platformData->isFocused;
    platformData->isVisible = visible;
    platformData->isFocused = focused;
    bool isActive = platformData->isVisible && platformData->isFocused;
    if (wasActive != isActive) {
        platformData->refreshRequested = true;
        glfm__clearActiveTouches(platformData);
        if (display->focusFunc) {
            display->focusFunc(display, isActive);
        }
    }
}

static void glfm__mainLoopFunc(void *userData) {
    GLFMDisplay *display = userData;
    if (display) {
        GLFMPlatformData *platformData = display->platformData;
        
        // Check if canvas size has changed
        int displayChanged = EM_ASM_INT_V({
            var canvas = Module['canvas'];
            var devicePixelRatio = window.devicePixelRatio || 1;
            var width = canvas.clientWidth * devicePixelRatio;
            var height = canvas.clientHeight * devicePixelRatio;
            if (width != canvas.width || height != canvas.height) {
                canvas.width = width;
                canvas.height = height;
                return 1;
            } else {
                return 0;
            }
        });
        if (displayChanged) {
            platformData->refreshRequested = true;
            platformData->width = glfm__getDisplayWidth(display);
            platformData->height = glfm__getDisplayHeight(display);
            platformData->scale = emscripten_get_device_pixel_ratio();
            if (display->surfaceResizedFunc) {
                display->surfaceResizedFunc(display, platformData->width, platformData->height);
            }
        }

        // Tick
        if (platformData->refreshRequested) {
            platformData->refreshRequested = false;
            if (display->surfaceRefreshFunc) {
                display->surfaceRefreshFunc(display);
            }
        }
        if (display->renderFunc) {
            display->renderFunc(display);
        }
    }
}

static EM_BOOL glfm__webGLContextCallback(int eventType, const void *reserved, void *userData) {
    (void)reserved;
    GLFMDisplay *display = userData;
    GLFMPlatformData *platformData = display->platformData;
    platformData->refreshRequested = true;
    if (eventType == EMSCRIPTEN_EVENT_WEBGLCONTEXTLOST) {
        if (display->surfaceDestroyedFunc) {
            display->surfaceDestroyedFunc(display);
        }
        return 1;
    } else if (eventType == EMSCRIPTEN_EVENT_WEBGLCONTEXTRESTORED) {
        if (display->surfaceCreatedFunc) {
            display->surfaceCreatedFunc(display, platformData->width, platformData->height);
        }
        return 1;
    } else {
        return 0;
    }
}

static EM_BOOL glfm__focusCallback(int eventType, const EmscriptenFocusEvent *focusEvent, void *userData) {
    (void)focusEvent;
    GLFMDisplay *display = userData;
    GLFMPlatformData *platformData = display->platformData;
    bool focused = (eventType == EMSCRIPTEN_EVENT_FOCUS || eventType == EMSCRIPTEN_EVENT_FOCUSIN);
    glfm__setVisibleAndFocused(display, platformData->isVisible, focused);
    return 1;
}

static EM_BOOL glfm__visibilityChangeCallback(int eventType, const EmscriptenVisibilityChangeEvent *e, void *userData) {
    (void)eventType;
    GLFMDisplay *display = userData;
    GLFMPlatformData *platformData = display->platformData;
    glfm__setVisibleAndFocused(display, !e->hidden, platformData->isFocused);
    return 1;
}

static const char *glfm__beforeUnloadCallback(int eventType, const void *reserved, void *userData) {
    (void)eventType;
    (void)reserved;
    GLFMDisplay *display = userData;
    glfm__setVisibleAndFocused(display, false, false);
    return NULL;
}

static EM_BOOL glfm__orientationChangeCallback(int eventType,
                                               const EmscriptenDeviceOrientationEvent *deviceOrientationEvent,
                                               void *userData) {
    (void)eventType;
    (void)deviceOrientationEvent;
    GLFMDisplay *display = userData;
    GLFMPlatformData *platformData = display->platformData;
    GLFMInterfaceOrientation orientation = glfmGetInterfaceOrientation(display);
    if (platformData->orientation != orientation) {
        platformData->orientation = orientation;
        platformData->refreshRequested = true;
        if (display->orientationChangedFunc) {
            display->orientationChangedFunc(display, orientation);
        }
    }
    return 1;
}

static EM_BOOL glfm__keyCallback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    GLFMDisplay *display = userData;
    EM_BOOL handled = 0;

    // Key input
    if (display->keyFunc && (eventType == EMSCRIPTEN_EVENT_KEYDOWN || eventType == EMSCRIPTEN_EVENT_KEYUP)) {
        // This list of code values is from https://www.w3.org/TR/uievents-code/
        // (Added functions keys F13-F24)
        // egrep -o '<code class="code" id="code-.*?</code>' uievents-code.html | sort | awk -F"[><]" '{print $3}' | awk 1 ORS=', '
        // This array must be sorted for binary search. See GLFM_TEST_KEYBOARD_EVENT_ARRAYS.
        // NOTE: e->keyCode is obsolete. Only e->key or e->code should be used.
        static const char *KEYBOARD_EVENT_CODES[] = {
            "AltLeft", "AltRight", "ArrowDown", "ArrowLeft", "ArrowRight", "ArrowUp",
            "Backquote", "Backslash", "Backspace", "BracketLeft", "BracketRight", "BrowserBack",
            "CapsLock", "Comma", "ContextMenu", "ControlLeft", "ControlRight", "Delete", "Digit0",
            "Digit1", "Digit2", "Digit3", "Digit4", "Digit5", "Digit6", "Digit7", "Digit8",
            "Digit9", "End", "Enter", "Equal", "Escape", "F1", "F10", "F11", "F12", "F13", "F14",
            "F15", "F16", "F17", "F18", "F19", "F2", "F20", "F21", "F22", "F23", "F24", "F3", "F4",
            "F5", "F6", "F7", "F8", "F9", "Fn", "Help", "Home", "Insert", "KeyA", "KeyB", "KeyC", "KeyD",
            "KeyE", "KeyF", "KeyG", "KeyH", "KeyI", "KeyJ", "KeyK", "KeyL", "KeyM", "KeyN", "KeyO",
            "KeyP", "KeyQ", "KeyR", "KeyS", "KeyT", "KeyU", "KeyV", "KeyW", "KeyX", "KeyY", "KeyZ",
            "MediaPlayPause", "MetaLeft", "MetaRight", "Minus", "NumLock", "Numpad0", "Numpad1",
            "Numpad2", "Numpad3", "Numpad4", "Numpad5", "Numpad6", "Numpad7", "Numpad8", "Numpad9",
            "NumpadAdd", "NumpadDecimal", "NumpadDivide", "NumpadEnter", "NumpadEqual",
            "NumpadMultiply", "NumpadSubtract", "PageDown", "PageUp", "Pause", "Period",
            "Power", "PrintScreen", "Quote", "ScrollLock", "Semicolon", "ShiftLeft", "ShiftRight",
            "Slash", "Space", "Tab",
        };
        static const size_t KEYBOARD_EVENT_CODES_LENGTH = sizeof(KEYBOARD_EVENT_CODES) / sizeof(*KEYBOARD_EVENT_CODES);
        static const GLFMKeyCode GLFM_KEY_CODES[] = {
            GLFMKeyCodeAltLeft, GLFMKeyCodeAltRight, GLFMKeyCodeArrowDown, GLFMKeyCodeArrowLeft, GLFMKeyCodeArrowRight, GLFMKeyCodeArrowUp,
            GLFMKeyCodeBackquote, GLFMKeyCodeBackslash, GLFMKeyCodeBackspace, GLFMKeyCodeBracketLeft, GLFMKeyCodeBracketRight, GLFMKeyCodeNavigationBack,
            GLFMKeyCodeCapsLock, GLFMKeyCodeComma, GLFMKeyCodeMenu, GLFMKeyCodeControlLeft, GLFMKeyCodeControlRight, GLFMKeyCodeDelete, GLFMKeyCode0,
            GLFMKeyCode1, GLFMKeyCode2, GLFMKeyCode3, GLFMKeyCode4, GLFMKeyCode5, GLFMKeyCode6, GLFMKeyCode7, GLFMKeyCode8,
            GLFMKeyCode9, GLFMKeyCodeEnd, GLFMKeyCodeEnter, GLFMKeyCodeEqual, GLFMKeyCodeEscape, GLFMKeyCodeF1, GLFMKeyCodeF10, GLFMKeyCodeF11, GLFMKeyCodeF12, GLFMKeyCodeF13, GLFMKeyCodeF14,
            GLFMKeyCodeF15, GLFMKeyCodeF16, GLFMKeyCodeF17, GLFMKeyCodeF18, GLFMKeyCodeF19, GLFMKeyCodeF2, GLFMKeyCodeF20, GLFMKeyCodeF21, GLFMKeyCodeF22, GLFMKeyCodeF23, GLFMKeyCodeF24, GLFMKeyCodeF3, GLFMKeyCodeF4,
            GLFMKeyCodeF5, GLFMKeyCodeF6, GLFMKeyCodeF7, GLFMKeyCodeF8, GLFMKeyCodeF9, GLFMKeyCodeFunction, GLFMKeyCodeInsert, GLFMKeyCodeHome, GLFMKeyCodeInsert, GLFMKeyCodeA, GLFMKeyCodeB, GLFMKeyCodeC, GLFMKeyCodeD,
            GLFMKeyCodeE, GLFMKeyCodeF, GLFMKeyCodeG, GLFMKeyCodeH, GLFMKeyCodeI, GLFMKeyCodeJ, GLFMKeyCodeK, GLFMKeyCodeL, GLFMKeyCodeM, GLFMKeyCodeN, GLFMKeyCodeO,
            GLFMKeyCodeP, GLFMKeyCodeQ, GLFMKeyCodeR, GLFMKeyCodeS, GLFMKeyCodeT, GLFMKeyCodeU, GLFMKeyCodeV, GLFMKeyCodeW, GLFMKeyCodeX, GLFMKeyCodeY, GLFMKeyCodeZ,
            GLFMKeyCodeMediaPlayPause, GLFMKeyCodeMetaLeft, GLFMKeyCodeMetaRight, GLFMKeyCodeMinus, GLFMKeyCodeNumLock, GLFMKeyCodeNumpad0, GLFMKeyCodeNumpad1,
            GLFMKeyCodeNumpad2, GLFMKeyCodeNumpad3, GLFMKeyCodeNumpad4, GLFMKeyCodeNumpad5, GLFMKeyCodeNumpad6, GLFMKeyCodeNumpad7, GLFMKeyCodeNumpad8, GLFMKeyCodeNumpad9,
            GLFMKeyCodeNumpadAdd, GLFMKeyCodeNumpadDecimal, GLFMKeyCodeNumpadDivide, GLFMKeyCodeNumpadEnter, GLFMKeyCodeNumpadEqual,
            GLFMKeyCodeNumpadMultiply, GLFMKeyCodeNumpadSubtract, GLFMKeyCodePageDown, GLFMKeyCodePageUp, GLFMKeyCodePause ,GLFMKeyCodePeriod,
            GLFMKeyCodePower, GLFMKeyCodePrintScreen, GLFMKeyCodeQuote, GLFMKeyCodeScrollLock, GLFMKeyCodeSemicolon, GLFMKeyCodeShiftLeft, GLFMKeyCodeShiftRight,
            GLFMKeyCodeSlash, GLFMKeyCodeSpace, GLFMKeyCodeTab,
        };

#if GLFM_TEST_KEYBOARD_EVENT_ARRAYS
        static bool KEYBOARD_EVENT_CODES_TESTED = false;
        if (!KEYBOARD_EVENT_CODES_TESTED) {
            KEYBOARD_EVENT_CODES_TESTED = true;
            if (glfm__listIsSorted(KEYBOARD_EVENT_CODES, KEYBOARD_EVENT_CODES_LENGTH)) {
                GLFM_LOG("Success: KEYBOARD_EVENT_CODES is sorted");
            } else {
                GLFM_LOG("Failure: KEYBOARD_EVENT_CODES is not sorted");
            }
            if (KEYBOARD_EVENT_CODES_LENGTH == sizeof(GLFM_KEY_CODES) / sizeof(*GLFM_KEY_CODES)) {
                GLFM_LOG("Success: GLFM_KEYBOARD_EVENT_CODES is the correct length");
            } else {
                GLFM_LOG("Failure: GLFM_KEYBOARD_EVENT_CODES is not the correct length");
            }
        }
#endif

        GLFMKeyAction action;
        if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
            if (e->repeat) {
                action = GLFMKeyActionRepeated;
            } else {
                action = GLFMKeyActionPressed;
            }
        } else {
            action = GLFMKeyActionReleased;
        }

        // Modifiers
        // Note, Emscripten doesn't provide a way to get extended modifiers like the function key.
        // (See KeyboardEvent's getModifierState() function).
        // Commands like "fn-f" ("Fullscreen" on macOS) will be treated as text input.
        int modifiers = 0;
        if (e->shiftKey) {
            modifiers |= GLFMKeyModifierShift;
        }
        if (e->ctrlKey) {
            modifiers |= GLFMKeyModifierControl;
        }
        if (e->altKey) {
            modifiers |= GLFMKeyModifierAlt;
        }
        if (e->metaKey) {
            modifiers |= GLFMKeyModifierMeta;
        }

        int codeIndex = glfm__sortedListSearch(KEYBOARD_EVENT_CODES, KEYBOARD_EVENT_CODES_LENGTH, e->code);
        GLFMKeyCode keyCode = codeIndex >= 0 ? GLFM_KEY_CODES[codeIndex] : GLFMKeyCodeUnknown;
        handled = display->keyFunc(display, keyCode, action, modifiers);
    }

    // Character input
    if (display->charFunc && eventType == EMSCRIPTEN_EVENT_KEYDOWN && !e->ctrlKey && !e->metaKey) {
        // It appears the only way to detect printable character input is to check if the "key" value is
        // not one of the pre-defined key values.
        // This list of pre-defined key values is from https://www.w3.org/TR/uievents-key/
        // (Added functions keys F13-F24 and Soft5-Soft10)
        // This array must be sorted for binary search. See GLFM_TEST_KEYBOARD_EVENT_ARRAYS.
        // egrep -o '<code class="key" id="key-.*?</code>' uievents-key.html | sort | awk -F"[><]" '{print $3}' | awk 1 ORS=', '
        static const char *KEYBOARD_EVENT_KEYS[] = {
            "AVRInput", "AVRPower", "Accept", "Again", "AllCandidates", "Alphanumeric", "Alt", "AltGraph", "AppSwitch",
            "ArrowDown", "ArrowLeft", "ArrowRight", "ArrowUp", "Attn", "AudioBalanceLeft", "AudioBalanceRight",
            "AudioBassBoostDown", "AudioBassBoostToggle", "AudioBassBoostUp", "AudioFaderFront", "AudioFaderRear",
            "AudioSurroundModeNext", "AudioTrebleDown", "AudioTrebleUp", "AudioVolumeDown", "AudioVolumeMute",
            "AudioVolumeUp", "Backspace", "BrightnessDown", "BrightnessUp", "BrowserBack", "BrowserFavorites",
            "BrowserForward", "BrowserHome", "BrowserRefresh", "BrowserSearch", "BrowserStop", "Call", "Camera",
            "CameraFocus", "Cancel", "CapsLock", "ChannelDown", "ChannelUp", "Clear", "Close", "ClosedCaptionToggle",
            "CodeInput", "ColorF0Red", "ColorF1Green", "ColorF2Yellow", "ColorF3Blue", "ColorF4Grey", "ColorF5Brown",
            "Compose", "ContextMenu", "Control", "Convert", "Copy", "CrSel", "Cut", "DVR", "Dead", "Delete", "Dimmer",
            "DisplaySwap", "Eisu", "Eject", "End", "EndCall", "Enter", "EraseEof", "Escape", "ExSel", "Execute", "Exit",
            "F1", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F2",
            "F20", "F21", "F22", "F23", "F24", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FavoriteClear0",
            "FavoriteClear1", "FavoriteClear2", "FavoriteClear3", "FavoriteRecall0", "FavoriteRecall1",
            "FavoriteRecall2", "FavoriteRecall3", "FavoriteStore0", "FavoriteStore1", "FavoriteStore2",
            "FavoriteStore3", "FinalMode", "Find", "Fn", "FnLock", "GoBack", "GoHome", "GroupFirst", "GroupLast",
            "GroupNext", "GroupPrevious", "Guide", "GuideNextDay", "GuidePreviousDay", "HangulMode", "HanjaMode",
            "Hankaku", "HeadsetHook", "Help", "Hibernate", "Hiragana", "HiraganaKatakana", "Home", "Hyper", "Info",
            "Insert", "InstantReplay", "JunjaMode", "KanaMode", "KanjiMode", "Katakana", "Key11", "Key12",
            "LastNumberRedial", "LaunchApplication1", "LaunchApplication2", "LaunchCalendar", "LaunchContacts",
            "LaunchMail", "LaunchMediaPlayer", "LaunchMusicPlayer", "LaunchPhone", "LaunchScreenSaver",
            "LaunchSpreadsheet", "LaunchWebBrowser", "LaunchWebCam", "LaunchWordProcessor", "Link", "ListProgram",
            "LiveContent", "Lock", "LogOff", "MailForward", "MailReply", "MailSend", "MannerMode", "MediaApps",
            "MediaAudioTrack", "MediaClose", "MediaFastForward", "MediaLast", "MediaPause", "MediaPlay",
            "MediaPlayPause", "MediaRecord", "MediaRewind", "MediaSkipBackward", "MediaSkipForward",
            "MediaStepBackward", "MediaStepForward", "MediaStop", "MediaTopMenu", "MediaTrackNext",
            "MediaTrackPrevious", "Meta", "MicrophoneToggle", "MicrophoneVolumeDown", "MicrophoneVolumeMute",
            "MicrophoneVolumeUp", "ModeChange", "NavigateIn", "NavigateNext", "NavigateOut", "NavigatePrevious", "New",
            "NextCandidate", "NextFavoriteChannel", "NextUserProfile", "NonConvert", "Notification", "NumLock",
            "OnDemand", "Open", "PageDown", "PageUp", "Pairing", "Paste", "Pause", "PinPDown", "PinPMove", "PinPToggle",
            "PinPUp", "Play", "PlaySpeedDown", "PlaySpeedReset", "PlaySpeedUp", "Power", "PowerOff",
            "PreviousCandidate", "Print", "PrintScreen", "Process", "Props", "RandomToggle", "RcLowBattery",
            "RecordSpeedNext", "Redo", "RfBypass", "Romaji", "STBInput", "STBPower", "Save", "ScanChannelsToggle",
            "ScreenModeNext", "ScrollLock", "Select", "Settings", "Shift", "SingleCandidate",
            "Soft1", "Soft10", "Soft2", "Soft3", "Soft4", "Soft5", "Soft6", "Soft7", "Soft8", "Soft9",
            "SpeechCorrectionList", "SpeechInputToggle", "SpellCheck", "SplitScreenToggle", "Standby",
            "Subtitle", "Super", "Symbol", "SymbolLock", "TV", "TV3DMode", "TVAntennaCable", "TVAudioDescription",
            "TVAudioDescriptionMixDown", "TVAudioDescriptionMixUp", "TVContentsMenu", "TVDataService", "TVInput",
            "TVInputComponent1", "TVInputComponent2", "TVInputComposite1", "TVInputComposite2", "TVInputHDMI1",
            "TVInputHDMI2", "TVInputHDMI3", "TVInputHDMI4", "TVInputVGA1", "TVMediaContext", "TVNetwork",
            "TVNumberEntry", "TVPower", "TVRadioService", "TVSatellite", "TVSatelliteBS", "TVSatelliteCS",
            "TVSatelliteToggle", "TVTerrestrialAnalog", "TVTerrestrialDigital", "TVTimer", "Tab", "Teletext",
            "Undo", "Unidentified", "VideoModeNext", "VoiceDial", "WakeUp", "Wink", "Zenkaku", "ZenkakuHankaku",
            "ZoomIn", "ZoomOut", "ZoomToggle",
        };
        static const size_t KEYBOARD_EVENT_KEYS_LENGTH = sizeof(KEYBOARD_EVENT_KEYS) / sizeof(*KEYBOARD_EVENT_KEYS);

#if GLFM_TEST_KEYBOARD_EVENT_ARRAYS
        static bool KEYBOARD_EVENT_KEYS_TESTED = false;
        if (!KEYBOARD_EVENT_KEYS_TESTED) {
            KEYBOARD_EVENT_KEYS_TESTED = true;
            if (glfm__listIsSorted(KEYBOARD_EVENT_KEYS, KEYBOARD_EVENT_KEYS_LENGTH)) {
                GLFM_LOG("Success: KEYBOARD_EVENT_KEYS is sorted");
            } else {
                GLFM_LOG("Failure: KEYBOARD_EVENT_KEYS is not sorted");
            }
        }
#endif
        if (e->key[0] != '\0') {
            bool isSingleChar = (e->key[1] == '\0');
            bool isPredefinedKey = false;
            if (!isSingleChar) {
                isPredefinedKey = glfm__sortedListSearch(KEYBOARD_EVENT_KEYS, KEYBOARD_EVENT_KEYS_LENGTH, e->key) >= 0;
            }
            if (isSingleChar || !isPredefinedKey) {
                display->charFunc(display, e->key, 0);
                handled = 1;
            }
        }
    }
    
    return handled;
}

static EM_BOOL glfm__mouseCallback(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    GLFMDisplay *display = userData;
    GLFMPlatformData *platformData = display->platformData;
    if (!display->touchFunc) {
        platformData->mouseDown = false;
        return 0;
    }
    
    // The mouse event handler targets EMSCRIPTEN_EVENT_TARGET_WINDOW so that dragging the mouse outside the canvas can be detected.
    // If a mouse drag begins inside the canvas, the mouse release event is sent even if the mouse is released outside the canvas.
    float canvasX, canvasY, canvasW, canvasH;
    EM_ASM({
        var rect = Module['canvas'].getBoundingClientRect();
        setValue($0, rect.x, "float");
        setValue($1, rect.y, "float");
        setValue($2, rect.width, "float");
        setValue($3, rect.height, "float");
    }, &canvasX, &canvasY, &canvasW, &canvasH);
    const float mouseX = (float)e->targetX - canvasX;
    const float mouseY = (float)e->targetY - canvasY;
    const bool mouseInside = mouseX >= 0 && mouseY >= 0 && mouseX < canvasW && mouseY < canvasH;
    if (!mouseInside && eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
        // Mouse click outside canvas
        return 0;
    }
    if (!mouseInside && eventType != EMSCRIPTEN_EVENT_MOUSEDOWN && !platformData->mouseDown) {
        // Mouse hover or click outside canvas
        return 0;
    }
    
    GLFMTouchPhase touchPhase;
    switch (eventType) {
        case EMSCRIPTEN_EVENT_MOUSEDOWN:
            touchPhase = GLFMTouchPhaseBegan;
            platformData->mouseDown = true;
            break;
            
        case EMSCRIPTEN_EVENT_MOUSEMOVE:
            if (platformData->mouseDown) {
                touchPhase = GLFMTouchPhaseMoved;
            } else {
                touchPhase = GLFMTouchPhaseHover;
            }
            break;
            
        case EMSCRIPTEN_EVENT_MOUSEUP:
            touchPhase = GLFMTouchPhaseEnded;
            platformData->mouseDown = false;
            break;
            
        default:
            touchPhase = GLFMTouchPhaseCancelled;
            platformData->mouseDown = false;
            break;
    }
    bool handled = display->touchFunc(display, e->button, touchPhase,
                                      platformData->scale * (double)mouseX,
                                      platformData->scale * (double)mouseY);
    // Always return `false` when the event is `mouseDown` for iframe support.
    // Returning `true` invokes `preventDefault`, and invoking `preventDefault` on
    // `mouseDown` events prevents `mouseMove` events outside the iframe.
    return handled && eventType != EMSCRIPTEN_EVENT_MOUSEDOWN;
}

static EM_BOOL glfm__mouseWheelCallback(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData) {
    (void)eventType;
    GLFMDisplay *display = userData;
    if (display->mouseWheelFunc) {
        GLFMPlatformData *platformData = display->platformData;
        GLFMMouseWheelDeltaType deltaType;
        switch (wheelEvent->deltaMode) {
            case DOM_DELTA_PIXEL: default:
                deltaType = GLFMMouseWheelDeltaPixel;
                break;
            case DOM_DELTA_LINE:
                deltaType = GLFMMouseWheelDeltaLine;
                break;
            case DOM_DELTA_PAGE:
                deltaType = GLFMMouseWheelDeltaPage;
                break;
        }
        return display->mouseWheelFunc(display,
                                       platformData->scale * (double)wheelEvent->mouse.targetX,
                                       platformData->scale * (double)wheelEvent->mouse.targetY,
                                       deltaType, wheelEvent->deltaX, wheelEvent->deltaY, wheelEvent->deltaZ);
    } else {
        return 0;
    }
}

static int glfm__getTouchIdentifier(GLFMPlatformData *platformData, const EmscriptenTouchPoint *t) {
    int firstNullIndex = -1;
    int index = -1;
    for (int i = 0; i < GLFM_MAX_ACTIVE_TOUCHES; i++) {
        if (platformData->activeTouches[i].identifier == t->identifier &&
            platformData->activeTouches[i].active) {
            index = i;
            break;
        } else if (firstNullIndex == -1 && !platformData->activeTouches[i].active) {
            firstNullIndex = i;
        }
    }
    if (index == -1) {
        if (firstNullIndex == -1) {
            // Shouldn't happen
            return -1;
        }
        index = firstNullIndex;
        platformData->activeTouches[index].identifier = t->identifier;
        platformData->activeTouches[index].active = true;
    }
    return index;
}

static EM_BOOL glfm__touchCallback(int eventType, const EmscriptenTouchEvent *e, void *userData) {
    GLFMDisplay *display = userData;
    if (display->touchFunc) {
        GLFMPlatformData *platformData = display->platformData;
        GLFMTouchPhase touchPhase;
        switch (eventType) {
            case EMSCRIPTEN_EVENT_TOUCHSTART:
                touchPhase = GLFMTouchPhaseBegan;
                break;

            case EMSCRIPTEN_EVENT_TOUCHMOVE:
                touchPhase = GLFMTouchPhaseMoved;
                break;

            case EMSCRIPTEN_EVENT_TOUCHEND:
                touchPhase = GLFMTouchPhaseEnded;
                break;

            case EMSCRIPTEN_EVENT_TOUCHCANCEL:
            default:
                touchPhase = GLFMTouchPhaseCancelled;
                break;
        }

        int handled = 0;
        for (int i = 0; i < e->numTouches; i++) {
            const EmscriptenTouchPoint *t = &e->touches[i];
            if (t->isChanged) {
                int identifier = glfm__getTouchIdentifier(platformData, t);
                if (identifier >= 0) {
                    if ((platformData->multitouchEnabled || identifier == 0)) {
                        handled |= display->touchFunc(display, identifier, touchPhase,
                                                      platformData->scale * (double)t->targetX,
                                                      platformData->scale * (double)t->targetY);
                    }

                    if (touchPhase == GLFMTouchPhaseEnded || touchPhase == GLFMTouchPhaseCancelled) {
                        platformData->activeTouches[identifier].active = false;
                    }
                }
            }
        }
        return handled;
    } else {
        return 0;
    }
}

// MARK: - main

int main(void) {
    GLFMDisplay *glfmDisplay = calloc(1, sizeof(GLFMDisplay));
    GLFMPlatformData *platformData = calloc(1, sizeof(GLFMPlatformData));
    glfmDisplay->platformData = platformData;
    glfmDisplay->supportedOrientations = GLFMInterfaceOrientationAll;
    platformData->orientation = glfmGetInterfaceOrientation(glfmDisplay);

    // Main entry
    glfmMain(glfmDisplay);

    // Init resizable canvas
    EM_ASM({
        var canvas = Module['canvas'];
        var devicePixelRatio = window.devicePixelRatio || 1;
        canvas.width = canvas.clientWidth * devicePixelRatio;
        canvas.height = canvas.clientHeight * devicePixelRatio;
    });
    platformData->width = glfm__getDisplayWidth(glfmDisplay);
    platformData->height = glfm__getDisplayHeight(glfmDisplay);
    platformData->scale = emscripten_get_device_pixel_ratio();

    // Create WebGL context
    EmscriptenWebGLContextAttributes attribs;
    emscripten_webgl_init_context_attributes(&attribs);
    attribs.alpha = glfmDisplay->colorFormat == GLFMColorFormatRGBA8888;
    attribs.depth = glfmDisplay->depthFormat != GLFMDepthFormatNone;
    attribs.stencil = glfmDisplay->stencilFormat != GLFMStencilFormatNone;
    attribs.antialias = glfmDisplay->multisample != GLFMMultisampleNone;
    attribs.premultipliedAlpha = 1;
    attribs.preserveDrawingBuffer = 0;
    attribs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attribs.failIfMajorPerformanceCaveat = 0;
    attribs.enableExtensionsByDefault = 0;

    const char *webGLTarget = "#canvas";
    int contextHandle = 0;
    if (glfmDisplay->preferredAPI >= GLFMRenderingAPIOpenGLES3) {
        // OpenGL ES 3.0 / WebGL 2.0
        attribs.majorVersion = 2;
        attribs.minorVersion = 0;
        contextHandle = emscripten_webgl_create_context(webGLTarget, &attribs);
        if (contextHandle) {
            platformData->renderingAPI = GLFMRenderingAPIOpenGLES3;
        }
    }
    if (!contextHandle) {
        // OpenGL ES 2.0 / WebGL 1.0
        attribs.majorVersion = 1;
        attribs.minorVersion = 0;
        contextHandle = emscripten_webgl_create_context(webGLTarget, &attribs);
        if (contextHandle) {
            platformData->renderingAPI = GLFMRenderingAPIOpenGLES2;
        }
    }
    if (!contextHandle) {
        GLFM_LOG("Couldn't create GL context");
        glfm__reportSurfaceError(glfmDisplay, "Couldn't create GL context");
        return 0;
    }

    emscripten_webgl_make_context_current(contextHandle);

    if (glfmDisplay->surfaceCreatedFunc) {
        glfmDisplay->surfaceCreatedFunc(glfmDisplay, platformData->width, platformData->height);
    }
    glfm__setVisibleAndFocused(glfmDisplay, true, true);

    // Setup callbacks
    emscripten_set_main_loop_arg(glfm__mainLoopFunc, glfmDisplay, 0, 0);
    emscripten_set_touchstart_callback(webGLTarget, glfmDisplay, 1, glfm__touchCallback);
    emscripten_set_touchend_callback(webGLTarget, glfmDisplay, 1, glfm__touchCallback);
    emscripten_set_touchmove_callback(webGLTarget, glfmDisplay, 1, glfm__touchCallback);
    emscripten_set_touchcancel_callback(webGLTarget, glfmDisplay, 1, glfm__touchCallback);
    emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, glfmDisplay, 1, glfm__mouseCallback);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, glfmDisplay, 1, glfm__mouseCallback);
    emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, glfmDisplay, 1, glfm__mouseCallback);
    emscripten_set_wheel_callback(webGLTarget, glfmDisplay, 1, glfm__mouseWheelCallback);
    //emscripten_set_click_callback(webGLTarget, glfmDisplay, 1, glfm__mouseCallback);
    //emscripten_set_dblclick_callback(webGLTarget, glfmDisplay, 1, glfm__mouseCallback);
    emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, glfmDisplay, 1, glfm__keyCallback);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, glfmDisplay, 1, glfm__keyCallback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, glfmDisplay, 1, glfm__keyCallback);
    emscripten_set_webglcontextlost_callback(webGLTarget, glfmDisplay, 1, glfm__webGLContextCallback);
    emscripten_set_webglcontextrestored_callback(webGLTarget, glfmDisplay, 1, glfm__webGLContextCallback);
    emscripten_set_visibilitychange_callback(glfmDisplay, 1, glfm__visibilityChangeCallback);
    emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, glfmDisplay, 1, glfm__focusCallback);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, glfmDisplay, 1, glfm__focusCallback);
    emscripten_set_beforeunload_callback(glfmDisplay, glfm__beforeUnloadCallback);
    emscripten_set_deviceorientation_callback(glfmDisplay, 1, glfm__orientationChangeCallback);
    return 0;
}

#endif // __EMSCRIPTEN__
