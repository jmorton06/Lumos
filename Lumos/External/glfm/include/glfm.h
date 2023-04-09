// GLFM
// https://github.com/brackeen/glfm

#ifndef GLFM_H
#define GLFM_H

#define GLFM_VERSION_MAJOR 0
#define GLFM_VERSION_MINOR 10
#define GLFM_VERSION_REVISION 0

#if !defined(__APPLE__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
#  error Unsupported platform
#endif

#if defined(__APPLE__)
#  include <TargetConditionals.h>
#  if !(TARGET_OS_IOS || TARGET_OS_TV || TARGET_OS_OSX)
#    error Unsupported Apple platform
#  endif
#  if !defined(GL_SILENCE_DEPRECATION)
#    define GL_SILENCE_DEPRECATION
#  endif
#endif

// OpenGL ES includes

#if defined(GLFM_INCLUDE_ES32)
#  if defined(__ANDROID__)
#    include <GLES3/gl32.h>
#    include <GLES3/gl3ext.h>
#  else
#    error OpenGL ES 3.2 only supported on Android
#  endif
#elif defined(GLFM_INCLUDE_ES31)
#  if defined(__ANDROID__)
#    include <GLES3/gl31.h>
#    include <GLES3/gl3ext.h>
#  else
#    error OpenGL ES 3.1 only supported on Android
#  endif
#elif defined(GLFM_INCLUDE_ES3)
#  if defined(__APPLE__)
#    if TARGET_OS_OSX
#      error OpenGL ES unavailable on macOS
#    else
#      include <OpenGLES/ES3/gl.h>
#      include <OpenGLES/ES3/glext.h>
#    endif
#  elif defined(__EMSCRIPTEN__)
#    include <GLES3/gl3.h>
#    include <GLES3/gl2ext.h>
#  else
#    include <GLES3/gl3.h>
#    include <GLES3/gl3ext.h>
#  endif
#elif !defined(GLFM_INCLUDE_NONE)
#  if defined(__APPLE__)
#    if TARGET_OS_OSX
#      include <OpenGL/gl3.h>
#    else
#      include <OpenGLES/ES2/gl.h>
#      include <OpenGLES/ES2/glext.h>
#    endif
#  else
#    include <GLES2/gl2.h>
#    include <GLES2/gl2ext.h>
#  endif
#endif

#ifdef __GNUC__
#  define GLFM_DEPRECATED(message) __attribute__((deprecated(message)))
#else
#  define GLFM_DEPRECATED(message)
#endif

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// MARK: - Enums

typedef enum {
    GLFMRenderingAPIOpenGLES2,
    GLFMRenderingAPIOpenGLES3,
    GLFMRenderingAPIOpenGLES31,
    GLFMRenderingAPIOpenGLES32,
    GLFMRenderingAPIMetal,
    GLFMRenderingAPIVulkan,
} GLFMRenderingAPI;

typedef enum {
    GLFMColorFormatRGBA8888,
    GLFMColorFormatRGB565,
} GLFMColorFormat;

typedef enum {
    GLFMDepthFormatNone,
    GLFMDepthFormat16,
    GLFMDepthFormat24,
} GLFMDepthFormat;

typedef enum {
    GLFMStencilFormatNone,
    GLFMStencilFormat8,
} GLFMStencilFormat;

typedef enum {
    GLFMMultisampleNone,
    GLFMMultisample4X,
} GLFMMultisample;

typedef enum {
    GLFMSwapBehaviorPlatformDefault,
    GLFMSwapBehaviorBufferDestroyed,
    GLFMSwapBehaviorBufferPreserved,
} GLFMSwapBehavior;

/// Defines whether system UI chrome (status bar, navigation bar) is shown.
typedef enum {
    /// Displays the app with the navigation bar.
    ///  - Android: Show the navigation bar.
    ///  - iOS: Show the home indicator.
    GLFMUserInterfaceChromeNavigation,
    /// Displays the app with both the navigation bar and the status bar.
    ///  - Android: Show the navigation bar and status bar.
    ///  - iOS: Show both the home indicator and the status bar.
    GLFMUserInterfaceChromeNavigationAndStatusBar,
    /// Displays the app without any UI chrome.
    ///  - Android 2.3: Fullscreen.
    ///  - Android 4.0 - 4.3: Navigation bar dimmed.
    ///  - Android 4.4: Fullscreen immersive mode.
    ///  - iOS: Show the home indicator dimmed.
    GLFMUserInterfaceChromeNone,
} GLFMUserInterfaceChrome;

enum {
    GLFMUserInterfaceChromeFullscreen GLFM_DEPRECATED("Replaced with GLFMUserInterfaceChromeNone") = GLFMUserInterfaceChromeNone,
};

typedef enum {
    GLFMInterfaceOrientationUnknown = 0,
    GLFMInterfaceOrientationPortrait = (1 << 0),
    GLFMInterfaceOrientationPortraitUpsideDown = (1 << 1),
    GLFMInterfaceOrientationLandscapeLeft = (1 << 2),
    GLFMInterfaceOrientationLandscapeRight = (1 << 3),
    GLFMInterfaceOrientationLandscape = (GLFMInterfaceOrientationLandscapeLeft |
                                         GLFMInterfaceOrientationLandscapeRight),
    GLFMInterfaceOrientationAll = (GLFMInterfaceOrientationPortrait |
                                   GLFMInterfaceOrientationPortraitUpsideDown |
                                   GLFMInterfaceOrientationLandscapeLeft |
                                   GLFMInterfaceOrientationLandscapeRight),
    GLFMInterfaceOrientationAllButUpsideDown = (GLFMInterfaceOrientationPortrait |
                                                GLFMInterfaceOrientationLandscapeLeft |
                                                GLFMInterfaceOrientationLandscapeRight),
} GLFMInterfaceOrientation;

/// *Deprecated:* See ``GLFMInterfaceOrientation``.
typedef enum {
    GLFMUserInterfaceOrientationAny GLFM_DEPRECATED("Replaced with GLFMInterfaceOrientationAll") = GLFMInterfaceOrientationAll,
    GLFMUserInterfaceOrientationPortrait GLFM_DEPRECATED("Replaced with GLFMInterfaceOrientationPortrait") = GLFMInterfaceOrientationPortrait,
    GLFMUserInterfaceOrientationLandscape GLFM_DEPRECATED("Replaced with GLFMInterfaceOrientationLandscape") = GLFMInterfaceOrientationLandscape,
} GLFMUserInterfaceOrientation GLFM_DEPRECATED("Replaced with GLFMInterfaceOrientation");

typedef enum {
    GLFMTouchPhaseHover,
    GLFMTouchPhaseBegan,
    GLFMTouchPhaseMoved,
    GLFMTouchPhaseEnded,
    GLFMTouchPhaseCancelled,
} GLFMTouchPhase;

typedef enum {
    GLFMMouseCursorAuto,
    GLFMMouseCursorNone,
    GLFMMouseCursorDefault,
    GLFMMouseCursorPointer,
    GLFMMouseCursorCrosshair,
    GLFMMouseCursorText,
    GLFMMouseCursorVerticalText,
} GLFMMouseCursor;

typedef enum {
    GLFMMouseWheelDeltaPixel,
    GLFMMouseWheelDeltaLine,
    GLFMMouseWheelDeltaPage
} GLFMMouseWheelDeltaType;

typedef enum {
    GLFMKeyCodeUnknown           = 0x00,

    // The key codes below have the same values as ASCII codes (uppercase)

    GLFMKeyCodeBackspace         = 0x08, // Apple: Delete
    GLFMKeyCodeTab               = 0x09,
    GLFMKeyCodeEnter             = 0x0D, // Apple: Return
    GLFMKeyCodeEscape            = 0x1B,
    GLFMKeyCodeSpace             = 0x20,
    GLFMKeyCodeQuote             = 0x27,
    GLFMKeyCodeComma             = 0x2C,
    GLFMKeyCodeMinus             = 0x2D,
    GLFMKeyCodePeriod            = 0x2E,
    GLFMKeyCodeSlash             = 0x2F,
    GLFMKeyCode0                 = 0x30,
    GLFMKeyCode1                 = 0x31,
    GLFMKeyCode2                 = 0x32,
    GLFMKeyCode3                 = 0x33,
    GLFMKeyCode4                 = 0x34,
    GLFMKeyCode5                 = 0x35,
    GLFMKeyCode6                 = 0x36,
    GLFMKeyCode7                 = 0x37,
    GLFMKeyCode8                 = 0x38,
    GLFMKeyCode9                 = 0x39,
    GLFMKeyCodeSemicolon         = 0x3B,
    GLFMKeyCodeEqual             = 0x3D,
    GLFMKeyCodeA                 = 0x41,
    GLFMKeyCodeB                 = 0x42,
    GLFMKeyCodeC                 = 0x43,
    GLFMKeyCodeD                 = 0x44,
    GLFMKeyCodeE                 = 0x45,
    GLFMKeyCodeF                 = 0x46,
    GLFMKeyCodeG                 = 0x47,
    GLFMKeyCodeH                 = 0x48,
    GLFMKeyCodeI                 = 0x49,
    GLFMKeyCodeJ                 = 0x4A,
    GLFMKeyCodeK                 = 0x4B,
    GLFMKeyCodeL                 = 0x4C,
    GLFMKeyCodeM                 = 0x4D,
    GLFMKeyCodeN                 = 0x4E,
    GLFMKeyCodeO                 = 0x4F,
    GLFMKeyCodeP                 = 0x50,
    GLFMKeyCodeQ                 = 0x51,
    GLFMKeyCodeR                 = 0x52,
    GLFMKeyCodeS                 = 0x53,
    GLFMKeyCodeT                 = 0x54,
    GLFMKeyCodeU                 = 0x55,
    GLFMKeyCodeV                 = 0x56,
    GLFMKeyCodeW                 = 0x57,
    GLFMKeyCodeX                 = 0x58,
    GLFMKeyCodeY                 = 0x59,
    GLFMKeyCodeZ                 = 0x5A,
    GLFMKeyCodeBracketLeft       = 0x5B,
    GLFMKeyCodeBackslash         = 0x5C,
    GLFMKeyCodeBracketRight      = 0x5D,
    GLFMKeyCodeBackquote         = 0x60, // Grave
    GLFMKeyCodeDelete            = 0x7F, // Apple: Forward Delete

    // Non-ASCII keys. These values may change in the future.

    GLFMKeyCodeCapsLock          = 0x80,
    GLFMKeyCodeShiftLeft         = 0x81,
    GLFMKeyCodeShiftRight        = 0x82,
    GLFMKeyCodeControlLeft       = 0x83,
    GLFMKeyCodeControlRight      = 0x84,
    GLFMKeyCodeAltLeft           = 0x85, // Apple: Option
    GLFMKeyCodeAltRight          = 0x86, // Apple: Option
    GLFMKeyCodeMetaLeft          = 0x87, // Apple: Command
    GLFMKeyCodeMetaRight         = 0x88, // Apple: Command
    GLFMKeyCodeMenu              = 0x89, // Context menu

    GLFMKeyCodeInsert            = 0x90,
    GLFMKeyCodePageUp            = 0x91,
    GLFMKeyCodePageDown          = 0x92,
    GLFMKeyCodeEnd               = 0x93,
    GLFMKeyCodeHome              = 0x94,
    GLFMKeyCodeArrowLeft         = 0x95,
    GLFMKeyCodeArrowUp           = 0x96,
    GLFMKeyCodeArrowRight        = 0x97,
    GLFMKeyCodeArrowDown         = 0x98,

    GLFMKeyCodePower             = 0x99,
    GLFMKeyCodeFunction          = 0x9A, // Apple: Fn
    GLFMKeyCodePrintScreen       = 0x9B, // System Request
    GLFMKeyCodeScrollLock        = 0x9C,
    GLFMKeyCodePause             = 0x9D, // Break

    GLFMKeyCodeNumLock           = 0xA0, // Apple: Clear
    GLFMKeyCodeNumpadDecimal     = 0xA1,
    GLFMKeyCodeNumpadMultiply    = 0xA2,
    GLFMKeyCodeNumpadAdd         = 0xA3,
    GLFMKeyCodeNumpadDivide      = 0xA4,
    GLFMKeyCodeNumpadEnter       = 0xA5,
    GLFMKeyCodeNumpadSubtract    = 0xA6,
    GLFMKeyCodeNumpadEqual       = 0xA7,

    GLFMKeyCodeNumpad0           = 0xB0,
    GLFMKeyCodeNumpad1           = 0xB1,
    GLFMKeyCodeNumpad2           = 0xB2,
    GLFMKeyCodeNumpad3           = 0xB3,
    GLFMKeyCodeNumpad4           = 0xB4,
    GLFMKeyCodeNumpad5           = 0xB5,
    GLFMKeyCodeNumpad6           = 0xB6,
    GLFMKeyCodeNumpad7           = 0xB7,
    GLFMKeyCodeNumpad8           = 0xB8,
    GLFMKeyCodeNumpad9           = 0xB9,

    GLFMKeyCodeF1                = 0xC1,
    GLFMKeyCodeF2                = 0xC2,
    GLFMKeyCodeF3                = 0xC3,
    GLFMKeyCodeF4                = 0xC4,
    GLFMKeyCodeF5                = 0xC5,
    GLFMKeyCodeF6                = 0xC6,
    GLFMKeyCodeF7                = 0xC7,
    GLFMKeyCodeF8                = 0xC8,
    GLFMKeyCodeF9                = 0xC9,
    GLFMKeyCodeF10               = 0xD0,
    GLFMKeyCodeF11               = 0xD1,
    GLFMKeyCodeF12               = 0xD2,
    GLFMKeyCodeF13               = 0xD3,
    GLFMKeyCodeF14               = 0xD4,
    GLFMKeyCodeF15               = 0xD5,
    GLFMKeyCodeF16               = 0xD6,
    GLFMKeyCodeF17               = 0xD7,
    GLFMKeyCodeF18               = 0xD8,
    GLFMKeyCodeF19               = 0xD9,
    GLFMKeyCodeF20               = 0xDA,
    GLFMKeyCodeF21               = 0xDB,
    GLFMKeyCodeF22               = 0xDC,
    GLFMKeyCodeF23               = 0xDD,
    GLFMKeyCodeF24               = 0xDE,

    GLFMKeyCodeNavigationBack    = 0xE0, // Android (soft) back button, tvOS menu/back button.
    GLFMKeyCodeMediaSelect       = 0xE1, // tvOS
    GLFMKeyCodeMediaPlayPause    = 0xE2, // tvOS
} GLFMKeyCode;

typedef GLFMKeyCode GLFMKey GLFM_DEPRECATED("Replaced with GLFMKeyCode");

enum {
    GLFMKeyBackspace GLFM_DEPRECATED("Replaced with GLFMKeyCodeBackspace") = GLFMKeyCodeBackspace,
    GLFMKeyTab GLFM_DEPRECATED("Replaced with GLFMKeyCodeTab") = GLFMKeyCodeTab,
    GLFMKeyEnter GLFM_DEPRECATED("Replaced with GLFMKeyCodeEnter") = GLFMKeyCodeEnter,
    GLFMKeyEscape GLFM_DEPRECATED("Replaced with GLFMKeyCodeEscape") = GLFMKeyCodeEscape,
    GLFMKeySpace GLFM_DEPRECATED("Replaced with GLFMKeyCodeSpace") = GLFMKeyCodeSpace,
    GLFMKeyPageUp GLFM_DEPRECATED("Replaced with GLFMKeyCodePageUp") = GLFMKeyCodePageUp,
    GLFMKeyPageDown GLFM_DEPRECATED("Replaced with GLFMKeyCodePageDown") = GLFMKeyCodePageDown,
    GLFMKeyEnd GLFM_DEPRECATED("Replaced with GLFMKeyCodeEnd") = GLFMKeyCodeEnd,
    GLFMKeyHome GLFM_DEPRECATED("Replaced with GLFMKeyCodeHome") = GLFMKeyCodeHome,
    GLFMKeyLeft GLFM_DEPRECATED("Replaced with GLFMKeyCodeArrowLeft") = GLFMKeyCodeArrowLeft,
    GLFMKeyUp GLFM_DEPRECATED("Replaced with GLFMKeyCodeArrowUp") = GLFMKeyCodeArrowUp,
    GLFMKeyRight GLFM_DEPRECATED("Replaced with GLFMKeyCodeArrowRight") = GLFMKeyCodeArrowRight,
    GLFMKeyDown GLFM_DEPRECATED("Replaced with GLFMKeyCodeArrowDown") = GLFMKeyCodeArrowDown,
    GLFMKeyDelete GLFM_DEPRECATED("Replaced with GLFMKeyCodeDelete") = GLFMKeyCodeDelete,
    GLFMKeyNavBack GLFM_DEPRECATED("Replaced with GLFMKeyCodeNavigationBack") = GLFMKeyCodeNavigationBack,
    GLFMKeyNavMenu GLFM_DEPRECATED("Replaced with GLFMKeyCodeMenu") = GLFMKeyCodeMenu,
    GLFMKeyNavSelect GLFM_DEPRECATED("Replaced with GLFMKeyCodeMediaSelect") = GLFMKeyCodeMediaSelect,
    GLFMKeyPlayPause GLFM_DEPRECATED("Replaced with GLFMKeyCodeMediaPlayPause") = GLFMKeyCodeMediaPlayPause,
};

typedef enum {
    GLFMKeyModifierShift    = (1 << 0),
    GLFMKeyModifierControl  = (1 << 1),
    GLFMKeyModifierAlt      = (1 << 2), // Apple: Option
    GLFMKeyModifierMeta     = (1 << 3), // Apple: Command
    GLFMKeyModifierFunction = (1 << 4), // Apple: Fn
} GLFMKeyModifier;

enum {
    GLFMKeyModifierCtrl GLFM_DEPRECATED("Replaced with GLFMKeyModifierControl") = GLFMKeyModifierControl,
};

typedef enum {
    GLFMKeyActionPressed,
    GLFMKeyActionRepeated,
    GLFMKeyActionReleased,
} GLFMKeyAction;

/// The hardware sensor type. See ``glfmIsSensorAvailable`` and ``glfmSetSensorFunc``.
typedef enum {
    /// Accelerometer sensor.
    /// In ``GLFMSensorFunc``, the `GLFMSensorEvent` vector is the acceleration in G's.
    GLFMSensorAccelerometer,
    /// Magnetometer sensor.
    /// In ``GLFMSensorFunc``, the `GLFMSensorEvent` vector is the magnetic field in microteslas.
    GLFMSensorMagnetometer,
    /// Gyroscope sensor.
    /// In ``GLFMSensorFunc``, the `GLFMSensorEvent` vector is the rotation rate in radians/second.
    GLFMSensorGyroscope,
    /// Rotation sensor.
    /// In ``GLFMSensorFunc``, the `GLFMSensorEvent` matrix is the rotation matrix where the
    /// X axis points North and the Z axis is vertical.
    GLFMSensorRotationMatrix,
} GLFMSensor;

typedef enum {
    GLFMHapticFeedbackLight,
    GLFMHapticFeedbackMedium,
    GLFMHapticFeedbackHeavy,
} GLFMHapticFeedbackStyle;

// MARK: - Structs and function pointers

typedef struct GLFMDisplay GLFMDisplay;

/// Function pointer returned from ``glfmGetProcAddress``.
typedef void (*GLFMProc)(void);

/// Render callback function. See ``glfmSetRenderFunc``.
typedef void (*GLFMRenderFunc)(GLFMDisplay *display);

/// *Deprecated:* Use ``GLFMRenderFunc``.
typedef void (*GLFMMainLoopFunc)(GLFMDisplay *display, double frameTime)
GLFM_DEPRECATED("See glfmSetRenderFunc and glfmSwapBuffers");

/// Callback function when mouse or touch events occur. See ``glfmSetTouchFunc``.
///
/// - Parameters:
///   - touch: The touch number (zero for primary touch, 1+ for multitouch), or
///            the mouse button number (zero for the primary button, one for secondary, etc.).
///   - phase: The touch phase.
///   - x: The x location of the event, in pixels.
///   - y: The y location of the event, in pixels.
/// - Returns: `true` if the event was handled, `false` otherwise.
typedef bool (*GLFMTouchFunc)(GLFMDisplay *display, int touch, GLFMTouchPhase phase,
                              double x, double y);

/// Callback function when key events occur. See ``glfmSetKeyFunc``.
///
/// For each keypress, this function is called before ``GLFMCharFunc``.
///
/// - Android and tvOS: When the user presses the back button (`GLFMKeyCodeNavigationBack`), this
/// function should return `false` to allow the user to exit the app, or return `true` if the back
/// button was handled in-app.
/// - Returns: `true` if the event was handled, `false` otherwise.
typedef bool (*GLFMKeyFunc)(GLFMDisplay *display, GLFMKeyCode keyCode, GLFMKeyAction action,
                            int modifiers);

/// Callback function when character input events occur. See ``glfmSetCharFunc``.
///
/// - Parameters:
///   - string: A NULL-terminated UTF-8 string. The string is only valid during the callback, and
///             should be copied if it is needed after the callback returns. The string is never
///             NULL.
///   - modifier: Deprecated and always set to 0.
typedef void (*GLFMCharFunc)(GLFMDisplay *display, const char *string, int modifiers);

/// Callback function when clipboard text is received. See ``glfmRequestClipboardText``.
///
/// - Parameters:
///   - string: A NULL-terminated UTF-8 string. The string is only valid during the callback, and
///             should be copied if it is needed after the callback returns. May be NULL if there
///             is no text in the clipboard, or if the text could not be converted to UTF-8.
typedef void (*GLFMClipboardTextFunc)(GLFMDisplay *display, const char *string);

/// Callback function when mouse wheel input events occur. See ``glfmSetMouseWheelFunc``.
/// - Parameters:
///   - x: The x location of the event, in pixels.
///   - y: The y location of the event, in pixels.
/// - Returns: `true` if the event was handled, `false` otherwise.
typedef bool (*GLFMMouseWheelFunc)(GLFMDisplay *display, double x, double y,
                                   GLFMMouseWheelDeltaType deltaType,
                                   double deltaX, double deltaY, double deltaZ);

/// Callback function when the virtual keyboard visibility changes.
/// See ``glfmSetKeyboardVisibilityChangedFunc``.
typedef void (*GLFMKeyboardVisibilityChangedFunc)(GLFMDisplay *display, bool visible,
                                                  double x, double y, double width, double height);

/// Callback function when the app interface orientation changes.
/// See ``glfmSetOrientationChangedFunc``.
typedef void (*GLFMOrientationChangedFunc)(GLFMDisplay *display,
                                           GLFMInterfaceOrientation orientation);

/// Callback function when the chrome insets ("safe area insets") changes.
/// See ``glfmSetDisplayChromeInsetsChangedFunc`` and ``glfmGetDisplayChromeInsets``.
typedef void (*GLFMDisplayChromeInsetsChangedFunc)(GLFMDisplay *display, double top, double right,
                                                   double bottom, double left);

/// Callback function when the surface could not be created.
/// See ``glfmSetSurfaceErrorFunc``.
typedef void (*GLFMSurfaceErrorFunc)(GLFMDisplay *display, const char *message);

/// Callback function when the OpenGL surface was created.
/// See ``glfmSetSurfaceCreatedFunc``.
typedef void (*GLFMSurfaceCreatedFunc)(GLFMDisplay *display, int width, int height);

/// Callback function when the OpenGL surface was resized (or rotated).
/// See ``glfmSetSurfaceResizedFunc``.
typedef void (*GLFMSurfaceResizedFunc)(GLFMDisplay *display, int width, int height);

/// Callback function to notify that the surface needs to be redrawn.
/// See ``glfmSetSurfaceRefreshFunc``.
typedef void (*GLFMSurfaceRefreshFunc)(GLFMDisplay *display);

/// Callback function when the OpenGL surface was destroyed.
/// See ``glfmSetSurfaceDestroyedFunc``.
typedef void (*GLFMSurfaceDestroyedFunc)(GLFMDisplay *display);

/// Callback function when the system receives a low memory warning.
/// See ``glfmSetMemoryWarningFunc``.
typedef void (*GLFMMemoryWarningFunc)(GLFMDisplay *display);

/// Callback function when the app loses or gains focus. See ``glfmSetAppFocusFunc``.
///
/// This function is called on startup after `glfmMain()`.
///
/// - Emscripten: This function is called when switching browser tabs and
/// before the page is unloaded.
typedef void (*GLFMAppFocusFunc)(GLFMDisplay *display, bool focused);

/// The result used in the hardware sensor callback. See ``glfmSetSensorFunc``.
///
/// The `vector` is used for all sensor types except for `GLFMSensorRotationMatrix`,
/// which uses `matrix`.
typedef struct {
    /// The sensor type
    GLFMSensor sensor;
    /// The timestamp of the event, which may not be related to wall-clock time.
    double timestamp;
    union {
        /// A three-dimensional vector.
        struct {
            double x, y, z;
        } vector;
        /// A 3x3 matrix.
        struct {
            double m00, m01, m02;
            double m10, m11, m12;
            double m20, m21, m22;
        } matrix;
    };
} GLFMSensorEvent;

/// Callback function when sensor events occur. See ``glfmSetSensorFunc``.
typedef void (*GLFMSensorFunc)(GLFMDisplay *display, GLFMSensorEvent event);

// MARK: - Functions

/// Main entry point for a GLFM app.
///
/// In this function, call ``glfmSetDisplayConfig`` and ``glfmSetRenderFunc``.
extern void glfmMain(GLFMDisplay *display);

/// Sets the requested display configuration.
///
/// This function should only be called in ``glfmMain``.
///
/// If the device does not support the preferred rendering API, the next available rendering API is
/// used (OpenGL ES 2.0 if OpenGL ES 3.0 is not available, for example).
/// Call ``glfmGetRenderingAPI`` in the ``GLFMSurfaceCreatedFunc`` to check which rendering API is
/// used.
void glfmSetDisplayConfig(GLFMDisplay *display,
                          GLFMRenderingAPI preferredAPI,
                          GLFMColorFormat colorFormat,
                          GLFMDepthFormat depthFormat,
                          GLFMStencilFormat stencilFormat,
                          GLFMMultisample multisample);

/// Sets the user data pointer.
///
/// The data is neither read nor modified. See ``glfmGetUserData``.
void glfmSetUserData(GLFMDisplay *display, void *userData);

/// Gets the user data pointer.
///
/// See ``glfmSetUserData``.
void *glfmGetUserData(const GLFMDisplay *display);

/// Swap buffers.
///
/// This function should be called at the end of the ``GLFMRenderFunc`` if any content was rendered.
///
/// - Emscripten: Rhis function does nothing. Buffer swapping happens automatically if any
/// OpenGL calls were made.
///
/// - Apple platforms: When using the Metal rendering API, this function does nothing.
/// Presenting the Metal drawable must happen in application code.
void glfmSwapBuffers(GLFMDisplay *display);

/// *Deprecated:* Use ``glfmGetSupportedInterfaceOrientation``.
GLFMUserInterfaceOrientation glfmGetUserInterfaceOrientation(GLFMDisplay *display)
GLFM_DEPRECATED("Replaced with glfmGetSupportedInterfaceOrientation");

/// *Deprecated:* Use ``glfmSetSupportedInterfaceOrientation``.
void glfmSetUserInterfaceOrientation(GLFMDisplay *display,
                                     GLFMUserInterfaceOrientation supportedOrientations)
GLFM_DEPRECATED("Replaced with glfmSetSupportedInterfaceOrientation");

/// Returns the supported user interface orientations. Default is `GLFMInterfaceOrientationAll`.
///
/// Actual support may be limited by the device or platform.
GLFMInterfaceOrientation glfmGetSupportedInterfaceOrientation(const GLFMDisplay *display);

/// Sets the supported user interface orientations.
///
/// Typical values are `GLFMInterfaceOrientationAll`, `GLFMInterfaceOrientationPortrait`, or
/// `GLFMInterfaceOrientationLandscape.`
///
/// Actual support may be limited by the device or platform.
void glfmSetSupportedInterfaceOrientation(GLFMDisplay *display,
                                          GLFMInterfaceOrientation supportedOrientations);

/// Gets the current user interface orientation.
///
/// - Returns: Either `GLFMInterfaceOrientationPortrait`, `GLFMInterfaceOrientationLandscapeLeft`,
///   `GLFMInterfaceOrientationLandscapeRight`, `GLFMInterfaceOrientationPortraitUpsideDown`, or
///   `GLFMInterfaceOrientationUnknown`.
GLFMInterfaceOrientation glfmGetInterfaceOrientation(const GLFMDisplay *display);

/// Gets the display size, in pixels.
///
/// The arguments for the `width` and `height` parameters may be `NULL`.
void glfmGetDisplaySize(const GLFMDisplay *display, int *width, int *height);

/// Gets the display scale.
///
/// On Apple platforms, the value will be 1.0 for non-retina displays and 2.0
/// for retina. Similar values will be returned for Android and Emscripten.
double glfmGetDisplayScale(const GLFMDisplay *display);

/// Gets the chrome insets, in pixels (AKA "safe area insets" in iOS).
///
/// The "insets" are the space taken on the outer edges of the display by status bars,
/// navigation bars, and other UI elements.
///
/// The arguments for the `top`, `right`, `bottom`, and `left` parameters may be `NULL`.
void glfmGetDisplayChromeInsets(const GLFMDisplay *display, double *top, double *right,
                                double *bottom, double *left);

/// Gets the user interface chrome.
GLFMUserInterfaceChrome glfmGetDisplayChrome(const GLFMDisplay *display);

/// Sets the user interface chrome.
///
/// This may modify the chrome insets, but not immediately.
/// Use ``glfmSetDisplayChromeInsetsChangedFunc`` to be notified when insets change.
///
/// - Emscripten: To switch to fullscreen, this function must be called from an user-generated
/// event handler.
void glfmSetDisplayChrome(GLFMDisplay *display, GLFMUserInterfaceChrome uiChrome);

/// Gets the rendering API of the display.
///
/// Defaults to `GLFMRenderingAPIOpenGLES2`.
///
/// The return value is not valid until the surface is created.
GLFMRenderingAPI glfmGetRenderingAPI(const GLFMDisplay *display);

/// Sets the swap behavior for newly created surfaces (Android only).
///
/// In order to take effect, the behavior should be set before the surface
/// is created, preferable at the very beginning of the ``glfmMain`` function.
void glfmSetSwapBehavior(GLFMDisplay *display, GLFMSwapBehavior behavior);

/// Returns the swap buffer behavior.
GLFMSwapBehavior glfmGetSwapBehavior(const GLFMDisplay *display);

/// Gets the address of the specified function.
GLFMProc glfmGetProcAddress(const char *functionName);

/// Gets whether there is currently text available in the system clipboard.
///
/// - Emscripten: Returns true if the Clipboard API is available. It is not possible to know if
///               text is available in the system clipboard until it is requested.
/// - tvOS: No clipboard API is available. Always returns false.
bool glfmHasClipboardText(const GLFMDisplay *display);

/// Requests the system clipboard text.
///
/// The `clipboardTextFunc` callback may be invoked after a delay because the clipboard text
/// may be retrieved from the network (iCloud clipboard) or may require user confirmation (web
/// dialog) before proceeding.
///
/// The `clipboardTextFunc` callback is invoked only once.
///
/// If there was no text in the clipboard, the `clipboardTextFunc` callback is invoked with a NULL
/// string.
///
/// - Emscripten: On some browsers, this function can only be called in an event handler, like
///               ``GLFMTouchFunc`` or ``GLFMKeyFunc``. Currently, Firefox does not support reading
///               from the clipboard.
/// - tvOS: No clipboard API is available. The `clipboardTextFunc` callback is invoked with a NULL
///         string.
void glfmRequestClipboardText(GLFMDisplay *display, GLFMClipboardTextFunc clipboardTextFunc);

/// Set the system clipboard text.
///
/// - tvOS: No clipboard API is available. Always returns false.
///
/// - Parameters:
///   - string: A NULL-terminated UTF-8 string.
/// - Returns: `true` on success, `false` otherwise.
bool glfmSetClipboardText(GLFMDisplay *display, const char *string);

/// Gets the value of the highest precision time available, in seconds.
///
/// The time should not be considered related to wall-clock time.
double glfmGetTime(void);

// MARK: - Callback functions

/// Sets the function to call before each frame is displayed.
///
/// This function is called at regular intervals (typically 60fps).
/// Applications will typically render in this callback. If the application rendered any content,
/// the application should call ``glfmSwapBuffers`` before returning. If the application did
/// not render content, it should return without calling ``glfmSwapBuffers``.
GLFMRenderFunc glfmSetRenderFunc(GLFMDisplay *display, GLFMRenderFunc renderFunc);

/// *Deprecated:* Use ``glfmSetRenderFunc``.
///
/// If this function is set, ``glfmSwapBuffers`` is called after calling the `GLFMMainLoopFunc`.
GLFMMainLoopFunc glfmSetMainLoopFunc(GLFMDisplay *display, GLFMMainLoopFunc mainLoopFunc)
GLFM_DEPRECATED("See glfmSetRenderFunc and glfmSwapBuffers");

/// Sets the function to call when the surface could not be created.
///
/// For example, the browser does not support WebGL.
GLFMSurfaceErrorFunc glfmSetSurfaceErrorFunc(GLFMDisplay *display,
                                             GLFMSurfaceErrorFunc surfaceErrorFunc);

/// Sets the function to call when the surface was created.
GLFMSurfaceCreatedFunc glfmSetSurfaceCreatedFunc(GLFMDisplay *display,
                                                 GLFMSurfaceCreatedFunc surfaceCreatedFunc);

/// Sets the function to call when the surface was resized (or rotated).
GLFMSurfaceResizedFunc glfmSetSurfaceResizedFunc(GLFMDisplay *display,
                                                 GLFMSurfaceResizedFunc surfaceResizedFunc);

/// Sets the function to call to notify that the surface needs to be redrawn.
///
/// This callback is called when returning from the background, or when the device was rotated.
/// The `GLFMRenderFunc` is called immediately after `GLFMSurfaceRefreshFunc`.
GLFMSurfaceRefreshFunc glfmSetSurfaceRefreshFunc(GLFMDisplay *display,
                                                 GLFMSurfaceRefreshFunc surfaceRefreshFunc);

/// Sets the function to call when the surface was destroyed.
///
/// The surface may be destroyed during OpenGL context loss.
/// All OpenGL resources should be deleted in this call.
GLFMSurfaceDestroyedFunc glfmSetSurfaceDestroyedFunc(GLFMDisplay *display,
                                                     GLFMSurfaceDestroyedFunc surfaceDestroyedFunc);

/// Sets the function to call when app interface orientation changes.
GLFMOrientationChangedFunc
glfmSetOrientationChangedFunc(GLFMDisplay *display,
                              GLFMOrientationChangedFunc orientationChangedFunc);

/// Sets the function to call when display chrome insets ("safe area insets") changes.
/// See also ``glfmGetDisplayChromeInsets``
GLFMDisplayChromeInsetsChangedFunc
glfmSetDisplayChromeInsetsChangedFunc(GLFMDisplay *display,
                                      GLFMDisplayChromeInsetsChangedFunc chromeInsetsChangedFunc);

/// Sets the function to call when the system sends a "low memory" warning.
GLFMMemoryWarningFunc glfmSetMemoryWarningFunc(GLFMDisplay *display,
                                               GLFMMemoryWarningFunc lowMemoryFunc);

/// Sets the function to call when the app loses or gains focus (goes into the background or returns
/// from the background).
GLFMAppFocusFunc glfmSetAppFocusFunc(GLFMDisplay *display, GLFMAppFocusFunc focusFunc);

// MARK: - Input functions

/// Sets whether multitouch input is enabled. By default, multitouch is disabled.
///
/// - tvOS: This function does nothing.
void glfmSetMultitouchEnabled(GLFMDisplay *display, bool multitouchEnabled);

/// Gets whether multitouch input is enabled. By default, multitouch is disabled.
///
/// - tvOS: Always returns false.
bool glfmGetMultitouchEnabled(const GLFMDisplay *display);

/// Gets whether the display has touch capabilities.
bool glfmHasTouch(const GLFMDisplay *display);

/// Checks if a hardware sensor is available.
///
/// - Emscripten: Always returns `false`.
bool glfmIsSensorAvailable(const GLFMDisplay *display, GLFMSensor sensor);

/// Sets the mouse cursor (only on platforms with a mouse).
void glfmSetMouseCursor(GLFMDisplay *display, GLFMMouseCursor mouseCursor);

/// Gets whether a virtual onscreen keyboard can be displayed.
///
/// Returns `true` on iOS and Android, `false` on other platforms.
bool glfmHasVirtualKeyboard(const GLFMDisplay *display);

/// Requests to show or hide the onscreen virtual keyboard.
///
/// On iPad, if a hardware keyboard is attached, the virtual keyboard will not actually be shown.
///
/// - Emscripten: this function does nothing.
/// - tvOS: This function does nothing.
void glfmSetKeyboardVisible(GLFMDisplay *display, bool visible);

/// Returns `true` if the virtual keyboard is currently visible.
///
/// - Emscripten: Always returns false.
/// - tvOS: Always returns false.
bool glfmIsKeyboardVisible(const GLFMDisplay *display);

/// Sets the function to call when the virtual keyboard changes visibility or changes bounds.
GLFMKeyboardVisibilityChangedFunc
glfmSetKeyboardVisibilityChangedFunc(GLFMDisplay *display,
                                     GLFMKeyboardVisibilityChangedFunc visibilityChangedFunc);

/// Sets the function to call when a mouse or touch event occurs.
GLFMTouchFunc glfmSetTouchFunc(GLFMDisplay *display, GLFMTouchFunc touchFunc);

/// Sets the function to call when a key event occurs.
///
/// - iOS and tvOS: Key events require iOS 13.4 and tvOS 13.4. No repeated events
/// (`GLFMKeyActionRepeated`) are sent.
///
/// - iOS and Android: Use ``glfmSetKeyboardVisible`` to show the virtual keyboard.

/// - Android and tvOS: When the user presses the back button (`GLFMKeyCodeNavigationBack`), the
/// `GLFMKeyFunc` function should return `false` to allow the user to exit the app, or return `true`
/// if the back button was handled in-app.
GLFMKeyFunc glfmSetKeyFunc(GLFMDisplay *display, GLFMKeyFunc keyFunc);

/// Sets the function to call when character input events occur.
///
/// Character events occur when a user types with a virtual keyboard on iOS and Android, or when
/// typing with a connected physical keyboard.
///
/// On iOS and Android, use ``glfmSetKeyboardVisible`` to show the virtual keyboard.
///
/// The callback function is called for key repeat events in some case, but not all. No repeat
/// events occur in the following situations:
/// * Android virtual keyboard (Except for the backspace key, which does send repeat events).
/// * iOS virtual keyboard.
/// * tvOS physical keyboard.
///
/// On Android, non-ASCII characters are not automatically received due to a limitation in the NDK.
/// To receive them, add this code in your `Activity`:
///
/// ```
/// public class MyActivity extends NativeActivity {
///
///     // Send unicode keyboard input to GLFM (using keycode value Integer.MAX_VALUE)
///     // Override
///     public boolean dispatchKeyEvent(KeyEvent event) {
///         if (event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN && event.getCharacters() != null &&
///             getWindow() != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
///             String s = event.getCharacters();
///             for (int offset = 0; offset < s.length(); ) {
///                 int codepoint = s.codePointAt(offset);
///                 getWindow().injectInputEvent(new KeyEvent(event.getDownTime(), event.getEventTime(),
///                     KeyEvent.ACTION_DOWN, Integer.MAX_VALUE, 0, 0, 0, codepoint));
///                 offset += Character.charCount(codepoint);
///             }
///             return true;
///         }
///         return super.dispatchKeyEvent(event);
///     }
/// }
/// ```
/// This code intercepts character events and re-dispatches them with the unicode values to GLFM.
GLFMCharFunc glfmSetCharFunc(GLFMDisplay *display, GLFMCharFunc charFunc);

/// Sets the function to call when the mouse wheel is moved.
///
/// Only enabled on Emscripten.
GLFMMouseWheelFunc glfmSetMouseWheelFunc(GLFMDisplay *display, GLFMMouseWheelFunc mouseWheelFunc);

/// Sets the function to call when the hardware sensor events occur for a particular sensor.
///
/// If the hardware sensor is not available, this function does nothing.
/// See ``glfmIsSensorAvailable``.
///
/// Each ``GLFMSensor`` type can have it's own ``GLFMSensorFunc``.
///
/// The hardware sensor is enabled when the `sensorFunc` is not `NULL`.
///
/// Sensor events can drain battery. To save battery, when sensor events are not needed,
/// set the `sensorFunc` to `NULL` to disable the sensor.
///
/// Sensors are automatically disabled when the app is inactive, and re-enabled when active again.
GLFMSensorFunc glfmSetSensorFunc(GLFMDisplay *display, GLFMSensor sensor, GLFMSensorFunc sensorFunc);

// MARK: - Haptics

/// Returns true if the device supports haptic feedback.
///
/// - iOS: Returns `true` if the device supports haptic feedback (iPhone 7 or newer) and
///   the device is running iOS 13 or newer.
/// - Emscripten: Always returns `false`.
bool glfmIsHapticFeedbackSupported(const GLFMDisplay *display);

/// Performs haptic feedback.
///
/// - Emscripten: This function does nothing.
void glfmPerformHapticFeedback(GLFMDisplay *display, GLFMHapticFeedbackStyle style);

// MARK: - Platform-specific functions

/// Returns `true` if this is an Apple platform that supports Metal, `false` otherwise.
bool glfmIsMetalSupported(const GLFMDisplay *display);

#if defined(__APPLE__) || defined(GLFM_EXPOSE_NATIVE_APPLE)

/// *Apple platforms only*: Returns a pointer to an `MTKView` instance, or `NULL` if Metal is not
/// available.
///
/// This will only return a valid reference after the surface was created.
void *glfmGetMetalView(const GLFMDisplay *display);

/// *Apple platforms only*: Returns a pointer to the `UIViewController` (iOS, tvOS) or the
/// `NSViewController` (macOS) used to display content.
void *glfmGetViewController(const GLFMDisplay *display);

#endif // GLFM_EXPOSE_NATIVE_APPLE

#if defined(__ANDROID__) || defined(GLFM_EXPOSE_NATIVE_ANDROID)

#if defined(__ANDROID__)
#  include <android/native_activity.h>
#else
typedef struct ANativeActivity ANativeActivity;
#endif

ANativeActivity *glfmAndroidGetActivity(void) GLFM_DEPRECATED("Use glfmGetAndroidActivity");

/// *Android only*: Returns a pointer to the display's `ANativeActivity` instance.
///
/// The returned `ANativeActivity` may be invalidated when the surface is destroyed. If a reference
/// to the `ANativeActivity` is kept, call this function again to get an updated reference in
/// ``GLFMSurfaceCreatedFunc`` or ``GLFMAppFocusFunc``.
ANativeActivity *glfmGetAndroidActivity(const GLFMDisplay *display);

#endif // GLFM_EXPOSE_NATIVE_ANDROID

#ifdef __cplusplus
}
#endif

#endif // GLFM_H
