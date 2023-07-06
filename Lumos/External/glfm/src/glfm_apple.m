// GLFM
// https://github.com/brackeen/glfm

#if defined(__APPLE__)

#include "glfm.h"

#if !defined(GLFM_INCLUDE_METAL)
#  define GLFM_INCLUDE_METAL 1
#endif

#if TARGET_OS_IOS || TARGET_OS_TV
#  import <UIKit/UIKit.h>
#elif TARGET_OS_OSX
#  import <AppKit/AppKit.h>
#  import <Carbon/Carbon.h> // For kVK key codes in HIToolbox/Events.h
#  import <IOKit/hidsystem/IOLLEvent.h> // For NX_DEVICE modifier keys
#  define UIView NSView
#  define UIViewAutoresizingFlexibleWidth NSViewWidthSizable
#  define UIViewAutoresizingFlexibleHeight NSViewHeightSizable
#  define UIViewController NSViewController
#  define UIWindow NSWindow
#endif
#if TARGET_OS_IOS
#  import <CoreHaptics/CoreHaptics.h>
#  import <CoreMotion/CoreMotion.h>
#endif
#if GLFM_INCLUDE_METAL
#  import <MetalKit/MetalKit.h>
#endif

#include <dlfcn.h>
#include "glfm_internal.h"

#ifdef NDEBUG
#  define GLFM_LOG(...) do { } while (0)
#else
#  define GLFM_LOG(...) NSLog(@__VA_ARGS__)
#endif

#if __has_feature(objc_arc)
#  define GLFM_AUTORELEASE(value) value
#  define GLFM_RELEASE(value) ((void)0)
#  define GLFM_WEAK __weak
#else
#  define GLFM_AUTORELEASE(value) [value autorelease]
#  define GLFM_RELEASE(value) [value release]
#  define GLFM_WEAK __unsafe_unretained
#endif

#if TARGET_OS_IOS || TARGET_OS_TV
#  define GLFM_MAX_SIMULTANEOUS_TOUCHES 10
#  ifdef NDEBUG
#    define GLFM_CHECK_GL_ERROR() ((void)0)
#  else
#    define GLFM_CHECK_GL_ERROR() do { GLenum error = glGetError(); if (error != GL_NO_ERROR) \
       GLFM_LOG("OpenGL error 0x%04x at glfm_apple.m:%i", error, __LINE__); } while(0)
#  endif
#endif

static bool glfm__isCGFloatEqual(CGFloat a, CGFloat b) {
#if CGFLOAT_IS_DOUBLE
    return fabs(a - b) <= DBL_EPSILON;
#else
    return fabsf(a - b) <= FLT_EPSILON;
#endif
}

static void glfm__getDefaultDisplaySize(const GLFMDisplay *display,
                                        double *width, double *height, double *scale);
static void glfm__getDrawableSize(double displayWidth, double displayHeight, double displayScale,
                                  int *width, int *height);

// MARK: - GLFMView protocol

@protocol GLFMView

@property(nonatomic, readonly) GLFMRenderingAPI renderingAPI;
@property(nonatomic, readonly) int drawableWidth;
@property(nonatomic, readonly) int drawableHeight;
@property(nonatomic, readonly) BOOL surfaceCreatedNotified;
@property(nonatomic, assign) BOOL animating;
@property(nonatomic, copy, nullable) void (^preRenderCallback)(void);

- (void)draw;
- (void)swapBuffers;
- (void)requestRefresh;

@end

// MARK: GLFMNullView

@interface GLFMNullView : UIView <GLFMView>

@end

@implementation GLFMNullView

@synthesize preRenderCallback = _preRenderCallback;

- (GLFMRenderingAPI)renderingAPI {
    return GLFMRenderingAPIOpenGLES2;
}

- (int)drawableWidth {
    return 0;
}

- (int)drawableHeight {
    return 0;
}

- (BOOL)animating {
    return NO;
}

- (BOOL)surfaceCreatedNotified {
    return NO;
}

- (void)setAnimating:(BOOL)animating {
    (void)animating;
}

- (void)draw {
    if (_preRenderCallback) {
        _preRenderCallback();
    }
}

- (void)swapBuffers {
    
}

- (void)requestRefresh {
    
}

- (void)dealloc {
    GLFM_RELEASE(_preRenderCallback);
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

@end

#if GLFM_INCLUDE_METAL

// MARK: - GLFMMetalView

@interface GLFMMetalView : MTKView <GLFMView, MTKViewDelegate>

@property(nonatomic, assign) GLFMDisplay *glfmDisplay;
@property(nonatomic, assign) int drawableWidth;
@property(nonatomic, assign) int drawableHeight;
@property(nonatomic, assign) BOOL surfaceCreatedNotified;
@property(nonatomic, assign) BOOL refreshRequested;
@property(nonatomic, assign) BOOL isDrawing;

@end

@implementation GLFMMetalView

@synthesize drawableWidth, drawableHeight, surfaceCreatedNotified, refreshRequested, isDrawing;
@synthesize glfmDisplay = _glfmDisplay, preRenderCallback = _preRenderCallback;
@dynamic renderingAPI, animating;

- (instancetype)initWithFrame:(CGRect)frame contentScaleFactor:(CGFloat)contentScaleFactor
                       device:(id<MTLDevice>)device glfmDisplay:(GLFMDisplay *)glfmDisplay {
    if ((self = [super initWithFrame:frame device:device])) {
#if TARGET_OS_IOS || TARGET_OS_TV
        self.contentScaleFactor = contentScaleFactor;
#else
        self.layer.contentsScale = contentScaleFactor;
#endif
        self.delegate = self;
        self.glfmDisplay = glfmDisplay;
        self.drawableWidth = (int)self.drawableSize.width;
        self.drawableHeight = (int)self.drawableSize.height;
        [self requestRefresh];

        switch (glfmDisplay->colorFormat) {
            case GLFMColorFormatRGB565:
                if (@available(iOS 8, tvOS 8, macOS 11, *)) {
                    self.colorPixelFormat = MTLPixelFormatB5G6R5Unorm;
                } else {
                    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
                }
                break;
            case GLFMColorFormatRGBA8888:
            default:
                self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
                break;
        }
        
        if (glfmDisplay->depthFormat == GLFMDepthFormatNone &&
            glfmDisplay->stencilFormat == GLFMStencilFormatNone) {
            self.depthStencilPixelFormat = MTLPixelFormatInvalid;
        } else if (glfmDisplay->depthFormat == GLFMDepthFormatNone) {
            self.depthStencilPixelFormat = MTLPixelFormatStencil8;
        } else if (glfmDisplay->stencilFormat == GLFMStencilFormatNone) {
            if (@available(iOS 13, tvOS 13, macOS 10.12, *)) {
                if (glfmDisplay->depthFormat == GLFMDepthFormat16) {
                    self.depthStencilPixelFormat = MTLPixelFormatDepth16Unorm;
                } else {
                    self.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
                }
            } else {
                self.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
            }
            
        } else {
            self.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
        }
        
        self.sampleCount = (glfmDisplay->multisample == GLFMMultisampleNone) ? 1 : 4;
    }
    return self;
}

- (GLFMRenderingAPI)renderingAPI {
    return GLFMRenderingAPIMetal;
}

#if TARGET_OS_OSX

- (void)viewDidChangeBackingProperties {
    [super viewDidChangeBackingProperties];
    if (self.window) {
        self.layer.contentsScale = self.window.backingScaleFactor;
    }
}

- (void)setFrameSize:(NSSize)newSize {
    // For live resizing
    [super setFrameSize:newSize];
    if (self.surfaceCreatedNotified) {
        [self requestRefresh];
        if (self.isLiveResizing) {
            [self draw];
        }
    }
}

- (void)viewWillStartLiveResize {
    self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
    [super viewWillStartLiveResize];
}

- (void)viewDidEndLiveResize {
    self.layerContentsPlacement = NSViewLayerContentsPlacementScaleAxesIndependently;
    [super viewDidEndLiveResize];
}

- (BOOL)isLiveResizing {
    return self.layerContentsPlacement == NSViewLayerContentsPlacementTopLeft;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

#endif // TARGET_OS_OSX

- (void)draw {
    if (!self.isDrawing) {
        [super draw];
    }
}

- (BOOL)animating {
    return !self.paused;
}

- (void)setAnimating:(BOOL)animating {
    if (self.animating != animating) {
        self.paused = !animating;
        [self requestRefresh];
    }
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    
}

- (void)drawInMTKView:(MTKView *)view {
    if (self.isDrawing) {
        return;
    }
    self.isDrawing = YES;
    int newDrawableWidth = (int)self.drawableSize.width;
    int newDrawableHeight = (int)self.drawableSize.height;
    if (!self.surfaceCreatedNotified) {
        self.surfaceCreatedNotified = YES;
        [self requestRefresh];
        self.drawableWidth = newDrawableWidth;
        self.drawableHeight = newDrawableHeight;
        if (self.glfmDisplay->surfaceCreatedFunc) {
            self.glfmDisplay->surfaceCreatedFunc(self.glfmDisplay,
                                                 self.drawableWidth, self.drawableHeight);
        }
    } else if (self.drawableWidth != newDrawableWidth || self.drawableHeight != newDrawableHeight) {
        [self requestRefresh];
        self.drawableWidth = newDrawableWidth;
        self.drawableHeight = newDrawableHeight;
        if (self.glfmDisplay->surfaceResizedFunc) {
            self.glfmDisplay->surfaceResizedFunc(self.glfmDisplay,
                                                 self.drawableWidth, self.drawableHeight);
        }
    }
    
    if (_preRenderCallback) {
        _preRenderCallback();
    }
    
    if (self.refreshRequested) {
        self.refreshRequested = NO;
        if (self.glfmDisplay->surfaceRefreshFunc) {
            self.glfmDisplay->surfaceRefreshFunc(self.glfmDisplay);
        }
    }
    
    if (self.glfmDisplay->renderFunc) {
        self.glfmDisplay->renderFunc(self.glfmDisplay);
    }

    self.isDrawing = NO;
}

- (void)swapBuffers {
    // Do nothing
}

- (void)requestRefresh {
    self.refreshRequested = YES;
}

#if TARGET_OS_IOS || TARGET_OS_TV

- (void)layoutSubviews {
    // First render as soon as safeAreaInsets are set
    if (!self.surfaceCreatedNotified) {
        [self requestRefresh];
        [self draw];
    }
}

#endif

- (void)dealloc {
    GLFM_RELEASE(_preRenderCallback);
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

@end

#endif // GLFM_INCLUDE_METAL

#if TARGET_OS_IOS || TARGET_OS_TV

// MARK: - GLFMOpenGLESView (iOS, tvOS)

@interface GLFMOpenGLESView : UIView <GLFMView>

@property(nonatomic, assign) GLFMDisplay *glfmDisplay;
@property(nonatomic, assign) GLFMRenderingAPI renderingAPI;
@property(nonatomic, strong) CADisplayLink *displayLink;
@property(nonatomic, strong) EAGLContext *context;
@property(nonatomic, strong) NSString *colorFormat;
@property(nonatomic, assign) BOOL preserveBackbuffer;
@property(nonatomic, assign) NSUInteger depthBits;
@property(nonatomic, assign) NSUInteger stencilBits;
@property(nonatomic, assign) BOOL multisampling;
@property(nonatomic, assign) BOOL surfaceCreatedNotified;
@property(nonatomic, assign) BOOL surfaceSizeChanged;
@property(nonatomic, assign) BOOL refreshRequested;
@property(nonatomic, assign) BOOL isDrawing;

@end

@implementation GLFMOpenGLESView {
    GLint _drawableWidth;
    GLint _drawableHeight;
    GLuint _defaultFramebuffer;
    GLuint _colorRenderbuffer;
    GLuint _attachmentRenderbuffer;
    GLuint _msaaFramebuffer;
    GLuint _msaaRenderbuffer;
}

@synthesize renderingAPI, displayLink, context, colorFormat, preserveBackbuffer;
@synthesize depthBits, stencilBits, multisampling;
@synthesize surfaceCreatedNotified, surfaceSizeChanged, refreshRequested, isDrawing;
@synthesize glfmDisplay = _glfmDisplay, preRenderCallback = _preRenderCallback;
@dynamic drawableWidth, drawableHeight, animating;

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame contentScaleFactor:(CGFloat)contentScaleFactor
                  glfmDisplay:(GLFMDisplay *)glfmDisplay {
    if ((self = [super initWithFrame:frame])) {
        
        self.contentScaleFactor = contentScaleFactor;
        self.glfmDisplay = glfmDisplay;
        [self requestRefresh];
        
        if (glfmDisplay->preferredAPI >= GLFMRenderingAPIOpenGLES3) {
            self.context = GLFM_AUTORELEASE([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3]);
            self.renderingAPI = GLFMRenderingAPIOpenGLES3;
        }
        if (!self.context) {
            self.context = GLFM_AUTORELEASE([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2]);
            self.renderingAPI = GLFMRenderingAPIOpenGLES2;
        }
        
        if (!self.context) {
            GLFM_LOG("Failed to create ES context");
            glfm__reportSurfaceError(glfmDisplay, "Failed to create ES context");
            GLFM_RELEASE(self);
            return nil;
        }
        
        switch (glfmDisplay->colorFormat) {
            case GLFMColorFormatRGB565:
                self.colorFormat = kEAGLColorFormatRGB565;
                break;
            case GLFMColorFormatRGBA8888:
            default:
                self.colorFormat = kEAGLColorFormatRGBA8;
                break;
        }
        
        switch (glfmDisplay->depthFormat) {
            case GLFMDepthFormatNone:
            default:
                self.depthBits = 0;
                break;
            case GLFMDepthFormat16:
                self.depthBits = 16;
                break;
            case GLFMDepthFormat24:
                self.depthBits = 24;
                break;
        }
        
        switch (glfmDisplay->stencilFormat) {
            case GLFMStencilFormatNone:
            default:
                self.stencilBits = 0;
                break;
            case GLFMStencilFormat8:
                self.stencilBits = 8;
                break;
        }
        
        self.multisampling = glfmDisplay->multisample != GLFMMultisampleNone;
        
        [self createDrawable];
    }
    return self;
}

- (void)dealloc {
    self.animating = NO;
    [self deleteDrawable];
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
    self.context = nil;
    self.colorFormat = nil;
    GLFM_RELEASE(_preRenderCallback);
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

- (int)drawableWidth {
    return (int)_drawableWidth;
}

- (int)drawableHeight {
    return (int)_drawableHeight;
}

- (BOOL)animating {
    return (self.displayLink != nil);
}

- (void)setAnimating:(BOOL)animating {
    if (self.animating != animating) {
        [self requestRefresh];
        if (!animating) {
            [self.displayLink invalidate];
            self.displayLink = nil;
        } else {
            self.displayLink = [CADisplayLink displayLinkWithTarget:self
                                                           selector:@selector(render:)];
            [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
        }
    }
}

- (void)createDrawable {
    if (_defaultFramebuffer != 0 || !self.context) {
        return;
    }

    if (!self.colorFormat) {
        self.colorFormat = kEAGLColorFormatRGBA8;
    }

    [EAGLContext setCurrentContext:self.context];

    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties =
        @{ kEAGLDrawablePropertyRetainedBacking : @(self.preserveBackbuffer),
           kEAGLDrawablePropertyColorFormat : self.colorFormat };

    glGenFramebuffers(1, &_defaultFramebuffer);
    glGenRenderbuffers(1, &_colorRenderbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);

    // iPhone 6 Display Zoom hack - use a modified bounds so that the renderbufferStorage method
    // creates the correct size renderbuffer.
    CGRect oldBounds = eaglLayer.bounds;
    if (glfm__isCGFloatEqual(eaglLayer.contentsScale, (CGFloat)2.343750)) {
        if (glfm__isCGFloatEqual(eaglLayer.bounds.size.width, (CGFloat)320.0) &&
            glfm__isCGFloatEqual(eaglLayer.bounds.size.height, (CGFloat)568.0)) {
            eaglLayer.bounds = CGRectMake(eaglLayer.bounds.origin.x, eaglLayer.bounds.origin.y,
                                          eaglLayer.bounds.size.width,
                                          1334 / eaglLayer.contentsScale);
        } else if (glfm__isCGFloatEqual(eaglLayer.bounds.size.width, (CGFloat)568.0) &&
                   glfm__isCGFloatEqual(eaglLayer.bounds.size.height, (CGFloat)320.0)) {
            eaglLayer.bounds = CGRectMake(eaglLayer.bounds.origin.x, eaglLayer.bounds.origin.y,
                                          1334 / eaglLayer.contentsScale,
                                          eaglLayer.bounds.size.height);
        }
    }

    if (![self.context renderbufferStorage:GL_RENDERBUFFER fromDrawable:eaglLayer]) {
        GLFM_LOG("Error: Call to renderbufferStorage failed");
    }

    eaglLayer.bounds = oldBounds;

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                              _colorRenderbuffer);

    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_drawableWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_drawableHeight);

    if (self.multisampling) {
        glGenFramebuffers(1, &_msaaFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, _msaaFramebuffer);

        glGenRenderbuffers(1, &_msaaRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _msaaRenderbuffer);

        GLenum internalFormat = GL_RGBA8_OES;
        if ([kEAGLColorFormatRGB565 isEqualToString:self.colorFormat]) {
            internalFormat = GL_RGB565;
        }

        glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 4, internalFormat,
                                              _drawableWidth, _drawableHeight);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                  _msaaRenderbuffer);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            GLFM_LOG("Error: Couldn't create multisample framebuffer: 0x%04x", status);
        }
    }

    if (self.depthBits > 0 || self.stencilBits > 0) {
        glGenRenderbuffers(1, &_attachmentRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _attachmentRenderbuffer);

        GLenum internalFormat;
        if (self.depthBits > 0 && self.stencilBits > 0) {
            internalFormat = GL_DEPTH24_STENCIL8_OES;
        } else if (self.depthBits >= 24) {
            internalFormat = GL_DEPTH_COMPONENT24_OES;
        } else if (self.depthBits > 0) {
            internalFormat = GL_DEPTH_COMPONENT16;
        } else {
            internalFormat = GL_STENCIL_INDEX8;
        }

        if (self.multisampling) {
            glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 4, internalFormat,
                                                  _drawableWidth, _drawableHeight);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, _drawableWidth, _drawableHeight);
        }

        if (self.depthBits > 0) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                      _attachmentRenderbuffer);
        }
        if (self.stencilBits > 0) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                      _attachmentRenderbuffer);
        }
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        GLFM_LOG("Error: Framebuffer incomplete: 0x%04x", status);
    }

    GLFM_CHECK_GL_ERROR();
}

- (void)deleteDrawable {
    [EAGLContext setCurrentContext:self.context];
    if (_defaultFramebuffer) {
        glDeleteFramebuffers(1, &_defaultFramebuffer);
        _defaultFramebuffer = 0;
    }
    if (_colorRenderbuffer) {
        glDeleteRenderbuffers(1, &_colorRenderbuffer);
        _colorRenderbuffer = 0;
    }
    if (_attachmentRenderbuffer) {
        glDeleteRenderbuffers(1, &_attachmentRenderbuffer);
        _attachmentRenderbuffer = 0;
    }
    if (_msaaRenderbuffer) {
        glDeleteRenderbuffers(1, &_msaaRenderbuffer);
        _msaaRenderbuffer = 0;
    }
    if (_msaaFramebuffer) {
        glDeleteFramebuffers(1, &_msaaFramebuffer);
        _msaaFramebuffer = 0;
    }
}

- (void)prepareRender {
    [EAGLContext setCurrentContext:self.context];
    if (self.multisampling) {
        glBindFramebuffer(GL_FRAMEBUFFER, _msaaFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _msaaRenderbuffer);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
    }
    GLFM_CHECK_GL_ERROR();
}

- (void)swapBuffers {
    [EAGLContext setCurrentContext:self.context];
    if (self.multisampling) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, _msaaFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, _defaultFramebuffer);
        glResolveMultisampleFramebufferAPPLE();
    }

    static bool checked_GL_EXT_discard_framebuffer = false;
    static bool has_GL_EXT_discard_framebuffer = false;
    if (!checked_GL_EXT_discard_framebuffer) {
        checked_GL_EXT_discard_framebuffer = true;
        const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
        if (extensions) {
            has_GL_EXT_discard_framebuffer = strstr(extensions, "GL_EXT_discard_framebuffer") != NULL;
        }
    }
    if (has_GL_EXT_discard_framebuffer) {
        GLenum target = GL_FRAMEBUFFER;
        GLenum attachments[3];
        GLsizei numAttachments = 0;
        if (self.multisampling) {
            target = GL_READ_FRAMEBUFFER_APPLE;
            attachments[numAttachments++] = GL_COLOR_ATTACHMENT0;
        }
        if (self.depthBits > 0) {
            attachments[numAttachments++] = GL_DEPTH_ATTACHMENT;
        }
        if (self.stencilBits > 0) {
            attachments[numAttachments++] = GL_STENCIL_ATTACHMENT;
        }
        if (numAttachments > 0) {
            if (self.multisampling) {
                glBindFramebuffer(GL_FRAMEBUFFER, _msaaFramebuffer);
            } else {
                glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
            }
            glDiscardFramebufferEXT(target, numAttachments, attachments);
        }
    }

    glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
    [self.context presentRenderbuffer:GL_RENDERBUFFER];

    GLFM_CHECK_GL_ERROR();
}

- (void)render:(CADisplayLink *)displayLink {
    if (self.isDrawing) {
        return;
    }
    self.isDrawing = YES;
    
    [EAGLContext setCurrentContext:self.context];
    
    if (!self.surfaceCreatedNotified) {
        self.surfaceCreatedNotified = YES;
        [self requestRefresh];
        if (self.glfmDisplay->surfaceCreatedFunc) {
            self.glfmDisplay->surfaceCreatedFunc(self.glfmDisplay,
                                                 self.drawableWidth, self.drawableHeight);
        }
    }
    
    if (self.surfaceSizeChanged) {
        self.surfaceSizeChanged = NO;
        [self requestRefresh];
        if (self.glfmDisplay->surfaceResizedFunc) {
            self.glfmDisplay->surfaceResizedFunc(self.glfmDisplay,
                                                 self.drawableWidth, self.drawableHeight);
        }
    }
    
    if (_preRenderCallback) {
        _preRenderCallback();
    }
    
    if (self.refreshRequested) {
        self.refreshRequested = NO;
        if (self.glfmDisplay->surfaceRefreshFunc) {
            self.glfmDisplay->surfaceRefreshFunc(self.glfmDisplay);
        }
    }
    if (self.glfmDisplay->renderFunc) {
        [self prepareRender];
        self.glfmDisplay->renderFunc(self.glfmDisplay);
    }

    self.isDrawing = NO;
}

- (void)draw {
    if (self.displayLink && !self.isDrawing) {
        [self render:self.displayLink];
    }
}

- (void)requestRefresh {
    self.refreshRequested = YES;
}

- (void)layoutSubviews {
    int newDrawableWidth;
    int newDrawableHeight;
    glfm__getDrawableSize((double)self.bounds.size.width, (double)self.bounds.size.height,
                          (double)self.contentScaleFactor, &newDrawableWidth, &newDrawableHeight);

    if (self.drawableWidth != newDrawableWidth || self.drawableHeight != newDrawableHeight) {
        [self deleteDrawable];
        [self createDrawable];
        self.surfaceSizeChanged = self.surfaceCreatedNotified;
    }
    
    // First render as soon as safeAreaInsets are set
    if (!self.surfaceCreatedNotified) {
        [self requestRefresh];
        [self draw];
    }
}

@end

#endif // TARGET_OS_IOS || TARGET_OS_TV

#if TARGET_OS_OSX

// MARK: - GLFMOpenGLView (macOS)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

@interface GLFMOpenGLView : NSOpenGLView <GLFMView>

@property(nonatomic, assign) GLFMDisplay *glfmDisplay;
@property(nonatomic, assign) int drawableWidth;
@property(nonatomic, assign) int drawableHeight;
@property(nonatomic, assign) BOOL surfaceCreatedNotified;
@property(nonatomic, assign) BOOL refreshRequested;
@property(nonatomic, assign) BOOL isDrawing;

@end

@implementation GLFMOpenGLView {
    CVDisplayLinkRef _displayLink;
    dispatch_source_t _displaySource;
}

@synthesize glfmDisplay = _glfmDisplay, preRenderCallback = _preRenderCallback;
@synthesize drawableWidth, drawableHeight;
@synthesize surfaceCreatedNotified, refreshRequested, isDrawing;
@dynamic renderingAPI, animating;

- (instancetype)initWithFrame:(CGRect)frame
                  glfmDisplay:(GLFMDisplay *)glfmDisplay {
    
    uint32_t colorBits = (glfmDisplay->colorFormat == GLFMColorFormatRGB565) ? 16 : 32;
    uint32_t alphaBits = (glfmDisplay->colorFormat == GLFMColorFormatRGB565) ? 0 : 8;
    uint32_t stencilBits = (glfmDisplay->stencilFormat == GLFMStencilFormat8) ? 8 : 0;
    uint32_t sampleCount = (glfmDisplay->multisample == GLFMMultisample4X) ? 4 : 1;
    uint32_t depthBits;
    switch (glfmDisplay->depthFormat) {
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
    
    // Set up attributes and create pixel format
    NSOpenGLPixelFormatAttribute attributes[32];
    size_t i = 0;
    attributes[i++] = kCGLPFASupportsAutomaticGraphicsSwitching;
    attributes[i++] = NSOpenGLPFAAccelerated;
    attributes[i++] = NSOpenGLPFAAllowOfflineRenderers;
    attributes[i++] = NSOpenGLPFAClosestPolicy;
    attributes[i++] = NSOpenGLPFADoubleBuffer;
    
    if (glfmDisplay->swapBehavior == GLFMSwapBehaviorBufferPreserved) {
        attributes[i++] = NSOpenGLPFABackingStore;
    } else if (glfmDisplay->swapBehavior == GLFMSwapBehaviorBufferDestroyed) {
        attributes[i++] = kCGLPFABackingVolatile;
    }
    
    attributes[i++] = NSOpenGLPFAOpenGLProfile;
    attributes[i++] = NSOpenGLProfileVersion3_2Core;
    
    attributes[i++] = NSOpenGLPFAColorSize;
    attributes[i++] = colorBits;
    if (alphaBits > 0) {
        attributes[i++] = NSOpenGLPFAAlphaSize;
        attributes[i++] = alphaBits;
    }
    if (depthBits > 0) {
        attributes[i++] = NSOpenGLPFADepthSize;
        attributes[i++] = depthBits;
    }
    if (stencilBits > 0) {
        attributes[i++] = NSOpenGLPFAStencilSize;
        attributes[i++] = stencilBits;
    }
    if (sampleCount > 1) {
        attributes[i++] = NSOpenGLPFASampleBuffers;
        attributes[i++] = 1;
        attributes[i++] = NSOpenGLPFASamples;
        attributes[i++] = sampleCount;
    }
    attributes[i] = 0;
    assert(i < sizeof(attributes) / sizeof(attributes[0]));
    
    NSOpenGLPixelFormat *pixelFormat = GLFM_AUTORELEASE([[NSOpenGLPixelFormat alloc]
                                                         initWithAttributes:attributes]);
    if (!pixelFormat) {
        GLFM_LOG("Failed to create GL pixel format");
        glfm__reportSurfaceError(glfmDisplay, "Failed to create GL pixel format");
        return nil;
    }
    
    // Initialize view
    self = [super initWithFrame:frame pixelFormat:pixelFormat];
    if (!self) {
        GLFM_LOG("Failed to create GL context");
        glfm__reportSurfaceError(glfmDisplay, "Failed to create GL context");
        return nil;
    }
    GLint swapInterval = 1;
    [self.openGLContext setValues:&swapInterval forParameter:NSOpenGLContextParameterSwapInterval];
    self.wantsBestResolutionOpenGLSurface = YES;
    self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
    self.glfmDisplay = glfmDisplay;

#if 0
    // Print attributes
    {
#define printAttribute(attr) do { \
    GLint virtualScreen = [self.openGLContext currentVirtualScreen]; \
    [pixelFormat getValues:&value forAttribute:(attr) forVirtualScreen:virtualScreen]; \
    printf("  " #attr ": %i\n", value); \
} while (0)
    
        GLint value;
        printAttribute(NSOpenGLPFAColorSize);
        printAttribute(NSOpenGLPFAAlphaSize);
        printAttribute(NSOpenGLPFADepthSize);
        printAttribute(NSOpenGLPFAStencilSize);
        printAttribute(NSOpenGLPFASampleBuffers);
        printAttribute(NSOpenGLPFASamples);
#undef printAttribute
    }
#endif

    // Initialize display link
    GLFM_WEAK __typeof(self) weakSelf = self;
    _displaySource = dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_ADD, 0, 0,
                                            dispatch_get_main_queue());
    dispatch_source_set_event_handler(_displaySource, ^{
        [weakSelf draw];
    });
    dispatch_resume(_displaySource);
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputHandler(_displayLink, ^CVReturn(CVDisplayLinkRef displayLink,
                                                          const CVTimeStamp *now,
                                                          const CVTimeStamp *outputTime,
                                                          CVOptionFlags flags,
                                                          CVOptionFlags *flagsOut) {
        (void)displayLink;
        (void)now;
        (void)outputTime;
        (void)flags;
        (void)flagsOut;
        __typeof(self) strongSelf = weakSelf;
        if (strongSelf) {
            dispatch_source_merge_data(strongSelf->_displaySource, 1);
        }
        return kCVReturnSuccess;
    });
    CGLContextObj cglContext = self.openGLContext.CGLContextObj;
    CGLPixelFormatObj cglPixelFormat = self.pixelFormat.CGLPixelFormatObj;
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(_displayLink, cglContext, cglPixelFormat);
    return self;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)viewDidChangeBackingProperties {
    [self requestRefresh];
    [self update];
}

- (GLFMRenderingAPI)renderingAPI {
    // TODO: Return correct API
    return GLFMRenderingAPIOpenGLES2;
}

- (BOOL)animating {
    return CVDisplayLinkIsRunning(_displayLink) == TRUE;
}

- (void)setAnimating:(BOOL)animating {
    if (self.animating != animating) {
        if (animating) {
            CVDisplayLinkStart(_displayLink);
        } else {
            CVDisplayLinkStop(_displayLink);
        }
        [self requestRefresh];
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    // For live resizing
    NSRect viewRectPixels = [self convertRectToBacking:self.bounds];
    int newDrawableWidth = (int)viewRectPixels.size.width;
    int newDrawableHeight = (int)viewRectPixels.size.height;
    if (self.surfaceCreatedNotified &&
        (self.drawableWidth != newDrawableWidth || self.drawableHeight != newDrawableHeight)) {
        [self requestRefresh];
        if (self.animating) {
            CVDisplayLinkStop(_displayLink);
            [self draw];
            CVDisplayLinkStart(_displayLink);
        } else {
            [self draw];
        }
    }
}

- (void)draw {
    if (self.isDrawing) {
        return;
    }
    self.isDrawing = YES;
    NSRect viewRectPixels = [self convertRectToBacking:self.bounds];
    int newDrawableWidth = (int)viewRectPixels.size.width;
    int newDrawableHeight = (int)viewRectPixels.size.height;
    
    assert([NSThread isMainThread]);

    [self.openGLContext makeCurrentContext];

    if (!self.surfaceCreatedNotified) {
        self.surfaceCreatedNotified = YES;
        [self requestRefresh];
        self.drawableWidth = newDrawableWidth;
        self.drawableHeight = newDrawableHeight;
        if (self.glfmDisplay->surfaceCreatedFunc) {
            self.glfmDisplay->surfaceCreatedFunc(self.glfmDisplay,
                                                 self.drawableWidth, self.drawableHeight);
        }
    } else if (self.drawableWidth != newDrawableWidth || self.drawableHeight != newDrawableHeight) {
        self.drawableWidth = newDrawableWidth;
        self.drawableHeight = newDrawableHeight;
        [self requestRefresh];
        if (self.glfmDisplay->surfaceResizedFunc) {
            self.glfmDisplay->surfaceResizedFunc(self.glfmDisplay,
                                                 self.drawableWidth, self.drawableHeight);
        }
    }
    
    if (_preRenderCallback) {
        _preRenderCallback();
    }
        
    if (self.refreshRequested) {
        self.refreshRequested = NO;
        if (self.glfmDisplay->surfaceRefreshFunc) {
            self.glfmDisplay->surfaceRefreshFunc(self.glfmDisplay);
        }
    }
    
    if (self.glfmDisplay->renderFunc) {
        self.glfmDisplay->renderFunc(self.glfmDisplay);
    }

    self.isDrawing = NO;
}

- (void)swapBuffers {
    [self.openGLContext flushBuffer];
}

- (void)requestRefresh {
    self.refreshRequested = YES;
}

- (void)dealloc {
    self.animating = NO;
    if ([NSOpenGLContext currentContext] == self.openGLContext) {
        [NSOpenGLContext clearCurrentContext];
    }
    dispatch_source_cancel(_displaySource);
    CVDisplayLinkRelease(_displayLink);
    GLFM_RELEASE(_preRenderCallback);
#if !__has_feature(objc_arc)
    dispatch_release(_displaySource);
    [super dealloc];
#endif
}

@end

#pragma clang diagnostic pop // "-Wdeprecated-declarations"

#endif // TARGET_OS_OSX

// MARK: - GLFMWindow interface

@interface GLFMWindow : UIWindow

@property(nonatomic, assign) BOOL active;

@end

// MARK: - GLFMViewController

@interface GLFMViewController : UIViewController

@property(nonatomic, assign) GLFMDisplay *glfmDisplay;
@property(nonatomic, assign) CGRect defaultFrame;
@property(nonatomic, assign) CGFloat defaultContentScale;
#if GLFM_INCLUDE_METAL
@property(nonatomic, strong) id<MTLDevice> metalDevice;
#endif
@end

#if TARGET_OS_IOS

@interface GLFMViewController () <UIKeyInput, UITextInputTraits, UIPointerInteractionDelegate>

@property(nonatomic, assign) BOOL keyboardRequested;
@property(nonatomic, strong) UIView *noSoftKeyboardView;
@property(nonatomic, strong) CMMotionManager *motionManager;
@property(nonatomic, assign) UIInterfaceOrientation orientation;
@property(nonatomic, assign) BOOL multipleTouchEnabled;
@property(nonatomic, assign) GLFMMouseCursor mouseCursor;

@end

#endif // TARGET_OS_IOS

#if TARGET_OS_OSX

@interface GLFMViewController () <NSTextInputClient>

@property(nonatomic, assign) NSEdgeInsets insets;
@property(nonatomic, strong) NSCursor *transparentCursor;
@property(nonatomic, strong) NSCursor *currentCursor;
@property(nonatomic, assign) BOOL hideMouseCursorWhileTyping;
@property(nonatomic, assign) BOOL mouseInside;
@property(nonatomic, assign) BOOL fnModifier;
@property(nonatomic, assign) UInt32 deadKeyState;
@property(nonatomic, assign) TISInputSourceRef currentKeyboard;
@property(nonatomic, assign) const UCKeyboardLayout *keyboardLayout;
@property(nonatomic, unsafe_unretained) id keyEventMonitor;
@property(nonatomic, strong) NSTextInputContext *textInputContext;

@end

#endif // TARGET_OS_OSX

@implementation GLFMViewController {
#if TARGET_OS_IOS || TARGET_OS_TV
    const void *activeTouches[GLFM_MAX_SIMULTANEOUS_TOUCHES];
#endif
}

@synthesize glfmDisplay, defaultFrame, defaultContentScale;

#if GLFM_INCLUDE_METAL
@synthesize metalDevice = _metalDevice;
#endif
#if TARGET_OS_IOS
@synthesize keyboardRequested, noSoftKeyboardView, motionManager = _motionManager, orientation;
@synthesize multipleTouchEnabled, mouseCursor;
#endif
#if TARGET_OS_OSX
@synthesize insets;
@synthesize transparentCursor, currentCursor, hideMouseCursorWhileTyping, mouseInside;
@synthesize fnModifier, deadKeyState = _deadKeyState, currentKeyboard, keyboardLayout, keyEventMonitor, textInputContext;
#endif

- (id)initWithDefaultFrame:(CGRect)frame contentScale:(CGFloat)contentScale {
    if ((self = [super init])) {
        self.glfmDisplay = calloc(1, sizeof(GLFMDisplay));
        self.glfmDisplay->platformData = (__bridge void *)self;
        self.glfmDisplay->supportedOrientations = GLFMInterfaceOrientationAll;
        self.defaultFrame = frame;
        self.defaultContentScale = contentScale;

#if TARGET_OS_IOS
        self.noSoftKeyboardView = GLFM_AUTORELEASE([UIView new]);
        self.mouseCursor = GLFMMouseCursorDefault;
#endif
        
#if TARGET_OS_OSX
        // Use a transparent image for the hidden cursor
        NSImage *transparentImage = [NSImage imageWithSize:NSMakeSize(16, 16) flipped:NO
                                            drawingHandler:^BOOL(NSRect dstRect) {
            NSRectFillUsingOperation(dstRect, NSCompositingOperationClear);
            return YES;
        }];
        self.transparentCursor = GLFM_AUTORELEASE([[NSCursor alloc] initWithImage:transparentImage
                                                                          hotSpot:NSZeroPoint]);
        self.currentCursor = NSCursor.arrowCursor;
        self.hideMouseCursorWhileTyping = YES;

        // Keyboard
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardChanged:) name:NSTextInputContextKeyboardSelectionDidChangeNotification object:nil];
        [self setCurrentKeyboard];

        // Send key release events when the command key is down
        GLFM_WEAK __typeof(self) weakSelf = self;
        self.keyEventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:^NSEvent *(NSEvent *event) {
            if (event.modifierFlags & NSEventModifierFlagCommand) {
                [weakSelf.glfmViewIfLoaded.window sendEvent:event];
            }
            return event;
        }];
        
        // Capture events from the Character Palette
        self.textInputContext = GLFM_AUTORELEASE([[NSTextInputContext alloc] initWithClient:self]);
        [self.textInputContext activate];
#else
        [self clearTouches];
#endif
    }
    return self;
}

- (void)dealloc {
    if (self.glfmViewIfLoaded.surfaceCreatedNotified && self.glfmDisplay->surfaceDestroyedFunc) {
        self.glfmDisplay->surfaceDestroyedFunc(self.glfmDisplay);
    }
    free(self.glfmDisplay);
    self.glfmViewIfLoaded.preRenderCallback = nil;
#if TARGET_OS_IOS
    self.motionManager = nil;
    self.noSoftKeyboardView = nil;
#endif
#if TARGET_OS_OSX
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    self.currentCursor = nil;
    self.transparentCursor = nil;
    if (self.currentKeyboard) {
        CFRelease(self.currentKeyboard);
        self.currentKeyboard = NULL;
    }
    [NSEvent removeMonitor:self.keyEventMonitor];
    self.keyEventMonitor = nil;
    [self.textInputContext deactivate];
    self.textInputContext = nil;
#endif
#if GLFM_INCLUDE_METAL
    self.metalDevice = nil;
#endif
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

#if GLFM_INCLUDE_METAL

- (id<MTLDevice>)metalDevice {
    if (!_metalDevice) {
        self.metalDevice = GLFM_AUTORELEASE(MTLCreateSystemDefaultDevice());
    }
    return _metalDevice;
}

#endif

- (UIView<GLFMView> *)glfmView {
    return (UIView<GLFMView> *)self.view;
}

- (UIView<GLFMView> *)glfmViewIfLoaded {
    if (self.isViewLoaded) {
        return (UIView<GLFMView> *)self.view;
    } else {
        return nil;
    }
}

- (void)loadView {
    glfmMain(self.glfmDisplay);

    UIView<GLFMView> *glfmView = nil;
    
#if GLFM_INCLUDE_METAL
    if (self.glfmDisplay->preferredAPI == GLFMRenderingAPIMetal && self.metalDevice) {
        glfmView = GLFM_AUTORELEASE([[GLFMMetalView alloc] initWithFrame:self.defaultFrame
                                                      contentScaleFactor:self.defaultContentScale
                                                                  device:self.metalDevice
                                                             glfmDisplay:self.glfmDisplay]);
    }
#endif
#if TARGET_OS_IOS || TARGET_OS_TV
    if (!glfmView) {
        glfmView = GLFM_AUTORELEASE([[GLFMOpenGLESView alloc] initWithFrame:self.defaultFrame
                                                         contentScaleFactor:self.defaultContentScale
                                                                glfmDisplay:self.glfmDisplay]);
    }
#endif
#if TARGET_OS_OSX
    if (!glfmView) {
        glfmView = GLFM_AUTORELEASE([[GLFMOpenGLView alloc] initWithFrame:self.defaultFrame
                                                              glfmDisplay:self.glfmDisplay]);
    }
#endif
    if (!glfmView) {
        assert(glfmView != nil);
        glfmView = GLFM_AUTORELEASE([[GLFMNullView alloc] initWithFrame:self.defaultFrame]);
    }
    GLFM_WEAK __typeof(self) weakSelf = self;
    glfmView.preRenderCallback = ^{
        [weakSelf preRenderCallback];
    };
    self.view = glfmView;
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    
#if TARGET_OS_OSX
    // Get mouse-move events and cursor events
    NSTrackingAreaOptions trackingOptions = (NSTrackingMouseMoved |
                                             NSTrackingMouseEnteredAndExited |
                                             NSTrackingCursorUpdate |
                                             NSTrackingInVisibleRect |
                                             NSTrackingActiveInKeyWindow);
    [self.view addTrackingArea:GLFM_AUTORELEASE([[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                                             options:trackingOptions
                                                                               owner:self
                                                                            userInfo:nil])];
#endif
}

- (void)preRenderCallback {
#if TARGET_OS_IOS
    [self handleMotionEvents];
#endif
}

- (void)viewDidLoad {
    [super viewDidLoad];

#if TARGET_OS_IOS
    self.view.multipleTouchEnabled = self.multipleTouchEnabled;
    self.orientation = [[UIApplication sharedApplication] statusBarOrientation];

    [self setNeedsStatusBarAppearanceUpdate];

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(keyboardFrameWillChange:)
                                               name:UIKeyboardWillChangeFrameNotification
                                             object:self.view.window];
    
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(deviceOrientationChanged:)
                                               name:UIDeviceOrientationDidChangeNotification
                                             object:self.view.window];

    if (@available(iOS 13.4, *)) {
        UIHoverGestureRecognizer *hover = [[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(hover:)];
        [self.view addGestureRecognizer:hover];
        GLFM_RELEASE(hover);

        UIPointerInteraction *pointerInteraction = [[UIPointerInteraction alloc] initWithDelegate:self];
        [self.view addInteraction:pointerInteraction];
        GLFM_RELEASE(pointerInteraction);
    }
#endif
}

#if TARGET_OS_OSX

- (void)viewDidLayout {
    [super viewDidLayout];

    // There doesn't appear to be a notification for insets changed on macOS.
    if (@available(macOS 11, *)) {
        double top, right, bottom, left;
        glfmGetDisplayChromeInsets(self.glfmDisplay, &top, &right, &bottom, &left);
        NSEdgeInsets newInsets = NSEdgeInsetsMake((CGFloat)top, (CGFloat)left, (CGFloat)bottom, (CGFloat)right);
        if (!NSEdgeInsetsEqual(self.insets, newInsets)) {
            self.insets = newInsets;
            if (self.glfmDisplay->displayChromeInsetsChangedFunc) {
                self.glfmDisplay->displayChromeInsetsChangedFunc(self.glfmDisplay, top, right, bottom, left);
            }
        }
    }
}

#endif

#if TARGET_OS_IOS || TARGET_OS_TV

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    GLFMWindow *window = (GLFMWindow *)self.view.window;
    self.glfmView.animating = window.active;
    [self becomeFirstResponder];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    if (self.glfmDisplay->lowMemoryFunc) {
        self.glfmDisplay->lowMemoryFunc(self.glfmDisplay);
    }
}

- (void)viewSafeAreaInsetsDidChange {
    [super viewSafeAreaInsetsDidChange];
    if (self.glfmDisplay->displayChromeInsetsChangedFunc) {
        double top, right, bottom, left;
        glfmGetDisplayChromeInsets(self.glfmDisplay, &top, &right, &bottom, &left);
        self.glfmDisplay->displayChromeInsetsChangedFunc(self.glfmDisplay, top, right, bottom, left);
    }
}

#endif

#if TARGET_OS_IOS

- (UIView *)inputView {
    if (self.keyboardRequested) {
        return nil; // System keyboard
    } else {
        return self.noSoftKeyboardView;
    }
}

- (BOOL)prefersStatusBarHidden {
    return self.glfmDisplay->uiChrome != GLFMUserInterfaceChromeNavigationAndStatusBar;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    UIRectEdge edges =  UIRectEdgeLeft | UIRectEdgeRight;
    return (self.glfmDisplay->uiChrome == GLFMUserInterfaceChromeNone ?
            (UIRectEdgeBottom | edges) : edges);
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    GLFMInterfaceOrientation orientations = self.glfmDisplay->supportedOrientations;
    BOOL portraitRequested = (orientations & (GLFMInterfaceOrientationPortrait | GLFMInterfaceOrientationPortraitUpsideDown)) != 0;
    BOOL landscapeRequested = (orientations & GLFMInterfaceOrientationLandscape) != 0;
    BOOL isTablet = [[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad;
    if (portraitRequested && landscapeRequested) {
        if (isTablet) {
            return UIInterfaceOrientationMaskAll;
        } else {
            return UIInterfaceOrientationMaskAllButUpsideDown;
        }
    } else if (landscapeRequested) {
        return UIInterfaceOrientationMaskLandscape;
    } else {
        if (isTablet) {
            return (UIInterfaceOrientationMask)(UIInterfaceOrientationMaskPortrait |
                    UIInterfaceOrientationMaskPortraitUpsideDown);
        } else {
            return UIInterfaceOrientationMaskPortrait;
        }
    }
}

- (void)deviceOrientationChanged:(NSNotification *)notification {
    UIInterfaceOrientation newOrientation = [[UIApplication sharedApplication] statusBarOrientation];
    if (self.orientation != newOrientation) {
        self.orientation = newOrientation;
        [self.glfmViewIfLoaded requestRefresh];
        if (self.glfmDisplay->orientationChangedFunc) {
            self.glfmDisplay->orientationChangedFunc(self.glfmDisplay,
                                                     glfmGetInterfaceOrientation(self.glfmDisplay));
        }
    }
}

- (CMMotionManager *)motionManager {
    if (!_motionManager) {
        self.motionManager = GLFM_AUTORELEASE([CMMotionManager new]);
        self.motionManager.deviceMotionUpdateInterval = 0.01;
    }
    return _motionManager;
}

- (BOOL)isMotionManagerLoaded {
    return _motionManager != nil;
}

- (void)handleMotionEvents {
    if (!self.isMotionManagerLoaded || !self.motionManager.isDeviceMotionActive) {
        return;
    }
    CMDeviceMotion *deviceMotion = self.motionManager.deviceMotion;
    if (!deviceMotion) {
        // No readings yet
        return;
    }
    GLFMSensorFunc accelerometerFunc = self.glfmDisplay->sensorFuncs[GLFMSensorAccelerometer];
    if (accelerometerFunc) {
        GLFMSensorEvent event = { 0 };
        event.sensor = GLFMSensorAccelerometer;
        event.timestamp = deviceMotion.timestamp;
        event.vector.x = deviceMotion.userAcceleration.x + deviceMotion.gravity.x;
        event.vector.y = deviceMotion.userAcceleration.y + deviceMotion.gravity.y;
        event.vector.z = deviceMotion.userAcceleration.z + deviceMotion.gravity.z;
        accelerometerFunc(self.glfmDisplay, event);
    }
    
    GLFMSensorFunc magnetometerFunc = self.glfmDisplay->sensorFuncs[GLFMSensorMagnetometer];
    if (magnetometerFunc) {
        GLFMSensorEvent event = { 0 };
        event.sensor = GLFMSensorMagnetometer;
        event.timestamp = deviceMotion.timestamp;
        event.vector.x = deviceMotion.magneticField.field.x;
        event.vector.y = deviceMotion.magneticField.field.y;
        event.vector.z = deviceMotion.magneticField.field.z;
        magnetometerFunc(self.glfmDisplay, event);
    }
    
    GLFMSensorFunc gyroscopeFunc = self.glfmDisplay->sensorFuncs[GLFMSensorGyroscope];
    if (gyroscopeFunc) {
        GLFMSensorEvent event = { 0 };
        event.sensor = GLFMSensorGyroscope;
        event.timestamp = deviceMotion.timestamp;
        event.vector.x = deviceMotion.rotationRate.x;
        event.vector.y = deviceMotion.rotationRate.y;
        event.vector.z = deviceMotion.rotationRate.z;
        gyroscopeFunc(self.glfmDisplay, event);
    }
    
    GLFMSensorFunc rotationFunc = self.glfmDisplay->sensorFuncs[GLFMSensorRotationMatrix];
    if (rotationFunc) {
        GLFMSensorEvent event = { 0 };
        event.sensor = GLFMSensorRotationMatrix;
        event.timestamp = deviceMotion.timestamp;
        CMRotationMatrix matrix = deviceMotion.attitude.rotationMatrix;
        event.matrix.m00 = matrix.m11; event.matrix.m01 = matrix.m12; event.matrix.m02 = matrix.m13;
        event.matrix.m10 = matrix.m21; event.matrix.m11 = matrix.m22; event.matrix.m12 = matrix.m23;
        event.matrix.m20 = matrix.m31; event.matrix.m21 = matrix.m32; event.matrix.m22 = matrix.m33;
        rotationFunc(self.glfmDisplay, event);
    }
}

- (void)updateMotionManagerActiveState {
    BOOL enable = NO;
    GLFMWindow *window = (GLFMWindow *)self.viewIfLoaded.window;
    if (window.active) {
        for (int i = 0; i < GLFM_NUM_SENSORS; i++) {
            if (self.glfmDisplay->sensorFuncs[i] != NULL) {
                enable = YES;
                break;
            }
        }
    }
    
    if (enable && !self.motionManager.deviceMotionActive) {
        CMAttitudeReferenceFrame referenceFrame;
        CMAttitudeReferenceFrame availableReferenceFrames = [CMMotionManager availableAttitudeReferenceFrames];
        if (availableReferenceFrames & CMAttitudeReferenceFrameXMagneticNorthZVertical) {
            referenceFrame = CMAttitudeReferenceFrameXMagneticNorthZVertical;
        } else if (availableReferenceFrames & CMAttitudeReferenceFrameXArbitraryCorrectedZVertical) {
            referenceFrame = CMAttitudeReferenceFrameXArbitraryCorrectedZVertical;
        } else {
            referenceFrame = CMAttitudeReferenceFrameXArbitraryZVertical;
        }
        [self.motionManager startDeviceMotionUpdatesUsingReferenceFrame:referenceFrame];
    } else if (!enable && self.isMotionManagerLoaded && self.motionManager.deviceMotionActive) {
        [self.motionManager stopDeviceMotionUpdates];
    }
}

#endif // TARGET_OS_IOS

#if TARGET_OS_IOS || TARGET_OS_TV

// MARK: UIResponder

- (BOOL)canBecomeFirstResponder {
    return YES;
}

- (void)clearTouches {
    for (int i = 0; i < GLFM_MAX_SIMULTANEOUS_TOUCHES; i++) {
        activeTouches[i] = NULL;
    }
}

- (void)addTouchEvent:(UITouch *)touch withType:(GLFMTouchPhase)phase {
    int firstNullIndex = -1;
    int index = -1;
    for (int i = 0; i < GLFM_MAX_SIMULTANEOUS_TOUCHES; i++) {
        if (activeTouches[i] == (__bridge const void *)touch) {
            index = i;
            break;
        } else if (firstNullIndex == -1 && activeTouches[i] == NULL) {
            firstNullIndex = i;
        }
    }
    if (index == -1) {
        if (firstNullIndex == -1) {
            // Shouldn't happen
            return;
        }
        index = firstNullIndex;
        activeTouches[index] = (__bridge const void *)touch;
    }

    if (self.glfmDisplay->touchFunc) {
        CGPoint currLocation = [touch locationInView:self.view];
        currLocation.x *= self.view.contentScaleFactor;
        currLocation.y *= self.view.contentScaleFactor;

        self.glfmDisplay->touchFunc(self.glfmDisplay, index, phase,
                                    (double)currLocation.x, (double)currLocation.y);
    }

    if (phase == GLFMTouchPhaseEnded || phase == GLFMTouchPhaseCancelled) {
        activeTouches[index] = NULL;
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self addTouchEvent:touch withType:GLFMTouchPhaseBegan];
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self addTouchEvent:touch withType:GLFMTouchPhaseMoved];
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self addTouchEvent:touch withType:GLFMTouchPhaseEnded];
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self addTouchEvent:touch withType:GLFMTouchPhaseCancelled];
    }
}

#if TARGET_OS_IOS

- (void)hover:(UIHoverGestureRecognizer *)recognizer API_AVAILABLE(ios(13.4)) {
    if (self.glfmDisplay->touchFunc && (recognizer.state == UIGestureRecognizerStateBegan ||
                                        recognizer.state == UIGestureRecognizerStateChanged)) {
        CGPoint currLocation = [recognizer locationInView:self.view];
        currLocation.x *= self.view.contentScaleFactor;
        currLocation.y *= self.view.contentScaleFactor;

        self.glfmDisplay->touchFunc(self.glfmDisplay, 0, GLFMTouchPhaseHover,
                                    (double)currLocation.x, (double)currLocation.y);
    }
}

- (UIPointerStyle *)pointerInteraction:(UIPointerInteraction *)interaction
                        styleForRegion:(UIPointerRegion *)region API_AVAILABLE(ios(13.4)) {
    switch (self.mouseCursor) {
        case GLFMMouseCursorText:
            return [UIPointerStyle styleWithShape:[UIPointerShape beamWithPreferredLength:20 axis:UIAxisVertical]
                                  constrainedAxes:UIAxisNeither];
        case GLFMMouseCursorVerticalText:
            return [UIPointerStyle styleWithShape:[UIPointerShape beamWithPreferredLength:20 axis:UIAxisHorizontal]
                                  constrainedAxes:UIAxisNeither];
        case GLFMMouseCursorNone:
            return [UIPointerStyle hiddenPointerStyle];
        case GLFMMouseCursorCrosshair:
        case GLFMMouseCursorDefault:
        case GLFMMouseCursorAuto:
        case GLFMMouseCursorPointer:
        default:
            return nil;
    }
}

#endif

- (BOOL)handlePress:(UIPress *)press withAction:(GLFMKeyAction)action {
#if TARGET_OS_IOS
    if (!self.glfmDisplay->keyFunc) {
        return NO;
    }
#elif TARGET_OS_TV
    if (!self.glfmDisplay->keyFunc && !self.glfmDisplay->charFunc) {
        return NO;
    }
#endif

    GLFMKeyCode keyCode = GLFMKeyCodeUnknown;
    int modifierFlags = 0;
    BOOL hasKey = NO;
    BOOL isPrintable = NO;
    if (@available(iOS 13.4, tvOS 13.4, *)) {
        static const GLFMKeyCode HID_MAP[] = {
            [UIKeyboardHIDUsageKeyboardReturnOrEnter]        = GLFMKeyCodeEnter,
            [UIKeyboardHIDUsageKeyboardTab]                  = GLFMKeyCodeTab,
            [UIKeyboardHIDUsageKeyboardSpacebar]             = GLFMKeyCodeSpace,
            [UIKeyboardHIDUsageKeyboardDeleteOrBackspace]    = GLFMKeyCodeBackspace,
            [UIKeyboardHIDUsageKeyboardEscape]               = GLFMKeyCodeEscape,

            [UIKeyboardHIDUsageKeyboardCapsLock]             = GLFMKeyCodeCapsLock,
            [UIKeyboardHIDUsageKeyboardLeftGUI]              = GLFMKeyCodeMetaLeft,
            [UIKeyboardHIDUsageKeyboardLeftShift]            = GLFMKeyCodeShiftLeft,
            [UIKeyboardHIDUsageKeyboardLeftAlt]              = GLFMKeyCodeAltLeft,
            [UIKeyboardHIDUsageKeyboardLeftControl]          = GLFMKeyCodeControlLeft,
            [UIKeyboardHIDUsageKeyboardRightGUI]             = GLFMKeyCodeMetaRight,
            [UIKeyboardHIDUsageKeyboardRightShift]           = GLFMKeyCodeShiftRight,
            [UIKeyboardHIDUsageKeyboardRightAlt]             = GLFMKeyCodeAltRight,
            [UIKeyboardHIDUsageKeyboardRightControl]         = GLFMKeyCodeControlRight,
            [UIKeyboardHIDUsageKeyboardApplication]          = GLFMKeyCodeMenu,

            [UIKeyboardHIDUsageKeyboardPrintScreen]          = GLFMKeyCodePrintScreen,
            [UIKeyboardHIDUsageKeyboardScrollLock]           = GLFMKeyCodeScrollLock,
            [UIKeyboardHIDUsageKeyboardPause]                = GLFMKeyCodePause,

            [UIKeyboardHIDUsageKeyboardInsert]               = GLFMKeyCodeInsert,
            [UIKeyboardHIDUsageKeyboardHome]                 = GLFMKeyCodeHome,
            [UIKeyboardHIDUsageKeyboardPageUp]               = GLFMKeyCodePageUp,
            [UIKeyboardHIDUsageKeyboardDeleteForward]        = GLFMKeyCodeDelete,
            [UIKeyboardHIDUsageKeyboardEnd]                  = GLFMKeyCodeEnd,
            [UIKeyboardHIDUsageKeyboardPageDown]             = GLFMKeyCodePageDown,

            [UIKeyboardHIDUsageKeyboardLeftArrow]            = GLFMKeyCodeArrowLeft,
            [UIKeyboardHIDUsageKeyboardRightArrow]           = GLFMKeyCodeArrowRight,
            [UIKeyboardHIDUsageKeyboardDownArrow]            = GLFMKeyCodeArrowDown,
            [UIKeyboardHIDUsageKeyboardUpArrow]              = GLFMKeyCodeArrowUp,

            [UIKeyboardHIDUsageKeyboardEqualSign]            = GLFMKeyCodeEqual,
            [UIKeyboardHIDUsageKeyboardHyphen]               = GLFMKeyCodeMinus,
            [UIKeyboardHIDUsageKeyboardOpenBracket]          = GLFMKeyCodeBracketLeft,
            [UIKeyboardHIDUsageKeyboardCloseBracket]         = GLFMKeyCodeBracketRight,
            [UIKeyboardHIDUsageKeyboardQuote]                = GLFMKeyCodeQuote,
            [UIKeyboardHIDUsageKeyboardSemicolon]            = GLFMKeyCodeSemicolon,
            [UIKeyboardHIDUsageKeyboardBackslash]            = GLFMKeyCodeBackslash,
            [UIKeyboardHIDUsageKeyboardComma]                = GLFMKeyCodeComma,
            [UIKeyboardHIDUsageKeyboardSlash]                = GLFMKeyCodeSlash,
            [UIKeyboardHIDUsageKeyboardPeriod]               = GLFMKeyCodePeriod,
            [UIKeyboardHIDUsageKeyboardGraveAccentAndTilde]  = GLFMKeyCodeBackquote,

            [UIKeyboardHIDUsageKeyboardA]                    = GLFMKeyCodeA,
            [UIKeyboardHIDUsageKeyboardB]                    = GLFMKeyCodeB,
            [UIKeyboardHIDUsageKeyboardC]                    = GLFMKeyCodeC,
            [UIKeyboardHIDUsageKeyboardD]                    = GLFMKeyCodeD,
            [UIKeyboardHIDUsageKeyboardE]                    = GLFMKeyCodeE,
            [UIKeyboardHIDUsageKeyboardF]                    = GLFMKeyCodeF,
            [UIKeyboardHIDUsageKeyboardG]                    = GLFMKeyCodeG,
            [UIKeyboardHIDUsageKeyboardH]                    = GLFMKeyCodeH,
            [UIKeyboardHIDUsageKeyboardI]                    = GLFMKeyCodeI,
            [UIKeyboardHIDUsageKeyboardJ]                    = GLFMKeyCodeJ,
            [UIKeyboardHIDUsageKeyboardK]                    = GLFMKeyCodeK,
            [UIKeyboardHIDUsageKeyboardL]                    = GLFMKeyCodeL,
            [UIKeyboardHIDUsageKeyboardN]                    = GLFMKeyCodeN,
            [UIKeyboardHIDUsageKeyboardM]                    = GLFMKeyCodeM,
            [UIKeyboardHIDUsageKeyboardO]                    = GLFMKeyCodeO,
            [UIKeyboardHIDUsageKeyboardP]                    = GLFMKeyCodeP,
            [UIKeyboardHIDUsageKeyboardQ]                    = GLFMKeyCodeQ,
            [UIKeyboardHIDUsageKeyboardR]                    = GLFMKeyCodeR,
            [UIKeyboardHIDUsageKeyboardS]                    = GLFMKeyCodeS,
            [UIKeyboardHIDUsageKeyboardT]                    = GLFMKeyCodeT,
            [UIKeyboardHIDUsageKeyboardU]                    = GLFMKeyCodeU,
            [UIKeyboardHIDUsageKeyboardV]                    = GLFMKeyCodeV,
            [UIKeyboardHIDUsageKeyboardW]                    = GLFMKeyCodeW,
            [UIKeyboardHIDUsageKeyboardX]                    = GLFMKeyCodeX,
            [UIKeyboardHIDUsageKeyboardY]                    = GLFMKeyCodeY,
            [UIKeyboardHIDUsageKeyboardZ]                    = GLFMKeyCodeZ,
            [UIKeyboardHIDUsageKeyboard0]                    = GLFMKeyCode0,
            [UIKeyboardHIDUsageKeyboard1]                    = GLFMKeyCode1,
            [UIKeyboardHIDUsageKeyboard2]                    = GLFMKeyCode2,
            [UIKeyboardHIDUsageKeyboard3]                    = GLFMKeyCode3,
            [UIKeyboardHIDUsageKeyboard4]                    = GLFMKeyCode4,
            [UIKeyboardHIDUsageKeyboard5]                    = GLFMKeyCode5,
            [UIKeyboardHIDUsageKeyboard6]                    = GLFMKeyCode6,
            [UIKeyboardHIDUsageKeyboard7]                    = GLFMKeyCode7,
            [UIKeyboardHIDUsageKeyboard8]                    = GLFMKeyCode8,
            [UIKeyboardHIDUsageKeyboard9]                    = GLFMKeyCode9,

            [UIKeyboardHIDUsageKeyboardPower]                = GLFMKeyCodePower,

            [UIKeyboardHIDUsageKeypadNumLock]                = GLFMKeyCodeNumLock,
            [UIKeyboardHIDUsageKeypadPeriod]                 = GLFMKeyCodeNumpadDecimal,
            [UIKeyboardHIDUsageKeypadAsterisk]               = GLFMKeyCodeNumpadMultiply,
            [UIKeyboardHIDUsageKeypadPlus]                   = GLFMKeyCodeNumpadAdd,
            [UIKeyboardHIDUsageKeypadSlash]                  = GLFMKeyCodeNumpadDivide,
            [UIKeyboardHIDUsageKeypadEnter]                  = GLFMKeyCodeNumpadEnter,
            [UIKeyboardHIDUsageKeypadHyphen]                 = GLFMKeyCodeNumpadSubtract,
            [UIKeyboardHIDUsageKeypadEqualSign]              = GLFMKeyCodeNumpadEqual,
            [UIKeyboardHIDUsageKeypad0]                      = GLFMKeyCodeNumpad0,
            [UIKeyboardHIDUsageKeypad1]                      = GLFMKeyCodeNumpad1,
            [UIKeyboardHIDUsageKeypad2]                      = GLFMKeyCodeNumpad2,
            [UIKeyboardHIDUsageKeypad3]                      = GLFMKeyCodeNumpad3,
            [UIKeyboardHIDUsageKeypad4]                      = GLFMKeyCodeNumpad4,
            [UIKeyboardHIDUsageKeypad5]                      = GLFMKeyCodeNumpad5,
            [UIKeyboardHIDUsageKeypad6]                      = GLFMKeyCodeNumpad6,
            [UIKeyboardHIDUsageKeypad7]                      = GLFMKeyCodeNumpad7,
            [UIKeyboardHIDUsageKeypad8]                      = GLFMKeyCodeNumpad8,
            [UIKeyboardHIDUsageKeypad9]                      = GLFMKeyCodeNumpad9,

            [UIKeyboardHIDUsageKeyboardF1]                   = GLFMKeyCodeF1,
            [UIKeyboardHIDUsageKeyboardF2]                   = GLFMKeyCodeF2,
            [UIKeyboardHIDUsageKeyboardF3]                   = GLFMKeyCodeF3,
            [UIKeyboardHIDUsageKeyboardF4]                   = GLFMKeyCodeF4,
            [UIKeyboardHIDUsageKeyboardF5]                   = GLFMKeyCodeF5,
            [UIKeyboardHIDUsageKeyboardF6]                   = GLFMKeyCodeF6,
            [UIKeyboardHIDUsageKeyboardF7]                   = GLFMKeyCodeF7,
            [UIKeyboardHIDUsageKeyboardF8]                   = GLFMKeyCodeF8,
            [UIKeyboardHIDUsageKeyboardF9]                   = GLFMKeyCodeF9,
            [UIKeyboardHIDUsageKeyboardF10]                  = GLFMKeyCodeF10,
            [UIKeyboardHIDUsageKeyboardF11]                  = GLFMKeyCodeF11,
            [UIKeyboardHIDUsageKeyboardF12]                  = GLFMKeyCodeF12,
            [UIKeyboardHIDUsageKeyboardF13]                  = GLFMKeyCodeF13,
            [UIKeyboardHIDUsageKeyboardF14]                  = GLFMKeyCodeF14,
            [UIKeyboardHIDUsageKeyboardF15]                  = GLFMKeyCodeF15,
            [UIKeyboardHIDUsageKeyboardF16]                  = GLFMKeyCodeF16,
            [UIKeyboardHIDUsageKeyboardF17]                  = GLFMKeyCodeF17,
            [UIKeyboardHIDUsageKeyboardF18]                  = GLFMKeyCodeF18,
            [UIKeyboardHIDUsageKeyboardF19]                  = GLFMKeyCodeF19,
            [UIKeyboardHIDUsageKeyboardF20]                  = GLFMKeyCodeF20,
            [UIKeyboardHIDUsageKeyboardF21]                  = GLFMKeyCodeF21,
            [UIKeyboardHIDUsageKeyboardF22]                  = GLFMKeyCodeF22,
            [UIKeyboardHIDUsageKeyboardF23]                  = GLFMKeyCodeF23,
            [UIKeyboardHIDUsageKeyboardF24]                  = GLFMKeyCodeF24,
        };

        UIKey *key = press.key;
        if (key) {
            BOOL isControlKey = NO;
            hasKey = YES;
            if ((key.modifierFlags & UIKeyModifierShift) != 0) {
                modifierFlags |= GLFMKeyModifierShift;
            }
            if ((key.modifierFlags & UIKeyModifierControl) != 0) {
                modifierFlags |= GLFMKeyModifierControl;
                isControlKey = YES;
            }
            if ((key.modifierFlags & UIKeyModifierAlternate) != 0) {
                modifierFlags |= GLFMKeyModifierAlt;
            }
            if ((key.modifierFlags & UIKeyModifierCommand) != 0) {
                modifierFlags |= GLFMKeyModifierMeta;
                isControlKey = YES;
            }
            if (key.keyCode >= 0 && (size_t)key.keyCode < sizeof(HID_MAP) / sizeof(*HID_MAP)) {
                keyCode = HID_MAP[key.keyCode];
            }
            if (self.isFirstResponder && self.glfmDisplay->charFunc != NULL &&
                action != GLFMKeyActionReleased && !isControlKey &&
                keyCode >= GLFMKeyCodeSpace && keyCode != GLFMKeyCodeDelete) {
                NSString *chars = key.charactersIgnoringModifiers;
                isPrintable = (chars.length > 0 && [chars characterAtIndex:0] >= ' ' &&
                               ![chars hasPrefix:@"UIKeyInput"]);
            }
        }
    }

    // NOTE: iOS supports UIKeyInput, but tvOS does not. Handle them separately.

#if TARGET_OS_IOS

    if (self.isFirstResponder &&
        (keyCode == GLFMKeyCodeEnter || keyCode == GLFMKeyCodeTab || keyCode == GLFMKeyCodeBackspace)) {
        // Let UIKeyInput handle these keys via insertText and deleteBackwards (allow key repeating).
        return NO;
    }
    if (keyCode == GLFMKeyCodeUnknown && !hasKey) {
        // The tab key on the Magic Keyboard sends two UIPress events. For the second one, press.key=nil and press.type=0xcb.
        return NO;
    }
    BOOL handled = self.glfmDisplay->keyFunc(self.glfmDisplay, keyCode, action, modifierFlags);
    if (self.isFirstResponder && isPrintable && self.glfmDisplay->charFunc) {
        // Send text via insertText.
        return NO;
    }
    return handled;

#endif // TARGET_OS_IOS

#if TARGET_OS_TV

    if (keyCode == GLFMKeyCodeUnknown) {
        switch (press.type) {
            case UIPressTypeUpArrow:
                keyCode = GLFMKeyCodeArrowUp;
                break;
            case UIPressTypeDownArrow:
                keyCode = GLFMKeyCodeArrowDown;
                break;
            case UIPressTypeLeftArrow:
                keyCode = GLFMKeyCodeArrowLeft;
                break;
            case UIPressTypeRightArrow:
                keyCode = GLFMKeyCodeArrowRight;
                break;
            case UIPressTypeSelect:
                keyCode = GLFMKeyCodeMediaSelect;
                break;
            case UIPressTypeMenu:
                keyCode = GLFMKeyCodeNavigationBack;
                break;
            case UIPressTypePlayPause:
                keyCode = GLFMKeyCodeMediaPlayPause;
                break;
            case UIPressTypePageUp:
                keyCode = GLFMKeyCodePageUp;
                break;
            case UIPressTypePageDown:
                keyCode = GLFMKeyCodePageDown;
                break;
        }
    }

    BOOL handled = self.glfmDisplay->keyFunc(self.glfmDisplay, keyCode, action, modifierFlags);
    if (@available(iOS 13.4, tvOS 13.4, *)) {
        if (self.isFirstResponder && hasKey && isPrintable && self.glfmDisplay->charFunc) {
            self.glfmDisplay->charFunc(self.glfmDisplay, press.key.characters.UTF8String, 0);
        }
    }
    return handled;

#endif // TARGET_OS_TV

}

// Returns set of unhandled presses
- (NSSet<UIPress *> *)handlePresses:(NSSet<UIPress *> *)presses withAction:(GLFMKeyAction)action {
    NSMutableSet<UIPress *> *unhandledPresses = nil;
    for (UIPress *press in presses) {
        BOOL handled = [self handlePress:press withAction:action];
        if (!handled) {
            if (presses.count == 1) {
                return presses; // Likely case
            }
            if (!unhandledPresses) {
                unhandledPresses = [NSMutableSet set];
            }
            [unhandledPresses addObject:press];
        }
    }
    return unhandledPresses;
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    NSSet<UIPress *> *unhandledPresses = [self handlePresses:presses withAction:GLFMKeyActionPressed];
    if (unhandledPresses.count > 0) {
        [super pressesBegan:unhandledPresses withEvent:event];
    }
}

- (void)pressesChanged:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    [super pressesChanged:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    NSSet<UIPress *> *unhandledPresses = [self handlePresses:presses withAction:GLFMKeyActionReleased];
    if (unhandledPresses.count > 0) {
        [super pressesEnded:unhandledPresses withEvent:event];
    }
}

- (void)pressesCancelled:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    NSSet<UIPress *> *unhandledPresses = [self handlePresses:presses withAction:GLFMKeyActionReleased];
    if (unhandledPresses.count > 0) {
        [super pressesCancelled:unhandledPresses withEvent:event];
    }
}

// MARK: UIKeyInput

#if TARGET_OS_IOS

- (void)keyboardFrameWillChange:(NSNotification *)notification {
    NSObject *frameBeginValue = notification.userInfo[UIKeyboardFrameBeginUserInfoKey];
    NSObject *frameEndValue = notification.userInfo[UIKeyboardFrameEndUserInfoKey];
    if ([frameBeginValue isKindOfClass:[NSValue class]] &&
        [frameEndValue isKindOfClass:[NSValue class]]) {
        CGRect keyboardFrameBegin = [(NSValue *)frameBeginValue CGRectValue];
        CGRect keyboardFrameEnd = [(NSValue *)frameEndValue CGRectValue];

        // If height is zero, the keyboard is hidden because a physical keyboard was attached.
        BOOL keyboardWasVisible = CGRectIntersectsRect(self.view.window.frame, keyboardFrameBegin);
        BOOL keyboardIsVisible = CGRectIntersectsRect(self.view.window.frame, keyboardFrameEnd);
        if (keyboardWasVisible && !keyboardIsVisible && keyboardFrameEnd.size.height > (CGFloat)0.0) {
            // User hid keyboard (iPad)
            self.keyboardRequested = NO;
        }
        
        [self.glfmViewIfLoaded requestRefresh];

        if (self.glfmDisplay->keyboardVisibilityChangedFunc) {
            // Convert to view coordinates
            keyboardFrameEnd = [self.view convertRect:keyboardFrameEnd fromView:nil];

            // Convert to pixels
            keyboardFrameEnd.origin.x *= self.view.contentScaleFactor;
            keyboardFrameEnd.origin.y *= self.view.contentScaleFactor;
            keyboardFrameEnd.size.width *= self.view.contentScaleFactor;
            keyboardFrameEnd.size.height *= self.view.contentScaleFactor;

            self.glfmDisplay->keyboardVisibilityChangedFunc(self.glfmDisplay, keyboardIsVisible,
                                                            (double)keyboardFrameEnd.origin.x,
                                                            (double)keyboardFrameEnd.origin.y,
                                                            (double)keyboardFrameEnd.size.width,
                                                            (double)keyboardFrameEnd.size.height);
        }
    }
}

// UITextInputTraits - disable suggestion bar
- (UITextAutocorrectionType)autocorrectionType {
    return UITextAutocorrectionTypeNo;
}

- (BOOL)hasText {
    return YES;
}

- (void)insertText:(NSString *)text {
    if ([text isEqualToString:@"\n"]) {
        if (self.glfmDisplay->keyFunc) {
            self.glfmDisplay->keyFunc(self.glfmDisplay, GLFMKeyCodeEnter, GLFMKeyActionPressed, 0);
        }
        if (self.glfmDisplay->keyFunc) {
            self.glfmDisplay->keyFunc(self.glfmDisplay, GLFMKeyCodeEnter, GLFMKeyActionReleased, 0);
        }
    } else if ([text isEqualToString:@"\t"]) {
        if (self.glfmDisplay->keyFunc) {
            self.glfmDisplay->keyFunc(self.glfmDisplay, GLFMKeyCodeTab, GLFMKeyActionPressed, 0);
        }
        if (self.glfmDisplay->keyFunc) {
            self.glfmDisplay->keyFunc(self.glfmDisplay, GLFMKeyCodeTab, GLFMKeyActionReleased, 0);
        }
    } else if (self.glfmDisplay->charFunc) {
        self.glfmDisplay->charFunc(self.glfmDisplay, text.UTF8String, 0);
    }
}

- (void)deleteBackward {
    // NOTE: This method is called for key repeat events when using a hardware keyboard, but not
    // when using the software keyboard.
    if (self.glfmDisplay->keyFunc) {
        self.glfmDisplay->keyFunc(self.glfmDisplay, GLFMKeyCodeBackspace, GLFMKeyActionPressed, 0);
    }
    if (self.glfmDisplay->keyFunc) {
        self.glfmDisplay->keyFunc(self.glfmDisplay, GLFMKeyCodeBackspace, GLFMKeyActionReleased, 0);
    }
}

#endif // TARGET_OS_IOS

- (NSArray<UIKeyCommand *> *)keyCommands {
    if (@available(iOS 13.4, tvOS 13.4, *)) {
        // Using UIPress events
        return @[];
    }
    static NSArray<UIKeyCommand *> *keyCommands = NULL;
    if (!keyCommands) {
        NSArray<NSString *> *keyInputs = @[
            UIKeyInputUpArrow, UIKeyInputDownArrow, UIKeyInputLeftArrow, UIKeyInputRightArrow,
            UIKeyInputEscape, UIKeyInputPageUp, UIKeyInputPageDown,
        ];
        NSMutableArray *mutableKeyCommands = GLFM_AUTORELEASE([NSMutableArray new]);
        [keyInputs enumerateObjectsUsingBlock:^(NSString *keyInput, NSUInteger idx, BOOL *stop) {
            (void)idx;
            (void)stop;
            [mutableKeyCommands addObject:[UIKeyCommand keyCommandWithInput:keyInput
                                                              modifierFlags:(UIKeyModifierFlags)0
                                                                     action:@selector(keyPressed:)]];
        }];
        keyCommands = [mutableKeyCommands copy];
    }
    return keyCommands;
}

- (void)keyPressed:(UIKeyCommand *)keyCommand {
    // Only invoked on iOS/tvOS 13.3 and older
    NSString *key = keyCommand.input;
    GLFMKeyCode keyCode = GLFMKeyCodeUnknown;
    if (key == UIKeyInputUpArrow) {
        keyCode = GLFMKeyCodeArrowUp;
    } else if (key == UIKeyInputDownArrow) {
        keyCode = GLFMKeyCodeArrowDown;
    } else if (key == UIKeyInputLeftArrow) {
        keyCode = GLFMKeyCodeArrowLeft;
    } else if (key == UIKeyInputRightArrow) {
        keyCode = GLFMKeyCodeArrowRight;
    } else if (key == UIKeyInputEscape) {
        keyCode = GLFMKeyCodeEscape;
    } else if (key == UIKeyInputPageUp) {
        keyCode = GLFMKeyCodePageUp;
    } else if (key == UIKeyInputPageDown) {
        keyCode = GLFMKeyCodePageDown;
    }
    if (self.glfmDisplay->keyFunc) {
        self.glfmDisplay->keyFunc(self.glfmDisplay, keyCode, GLFMKeyActionPressed, 0);
    }
    if (self.glfmDisplay->keyFunc) {
        self.glfmDisplay->keyFunc(self.glfmDisplay, keyCode, GLFMKeyActionReleased, 0);
    }
}

#endif // TARGET_OS_IOS || TARGET_OS_TV

#if TARGET_OS_OSX

// MARK: NSResponder (Mouse)

// Returns NO if location could not be determined
- (BOOL)getLocationForEvent:(NSEvent *)event outX:(double *)outX outY:(double *)outY {
    NSWindow *window = event.window;
    CGPoint locationInWindow;
    if (window) {
        locationInWindow = event.locationInWindow;
    } else {
        window = self.view.window;
        if (!window) {
            return NO;
        }
        // When event.window is nil, the locationInWindow property "contains the
        // event location in screen coordinates."
        CGPoint locationInScreen = event.locationInWindow;
        locationInWindow = [window convertPointFromScreen:locationInScreen];
    }
    CGFloat scale = window.backingScaleFactor;
    CGFloat contentHeight = window.contentLayoutRect.size.height;
    *outX = locationInWindow.x * scale;
    *outY = (contentHeight - locationInWindow.y) * scale; // Flip y
    
    return YES;
}

- (void)sendMouseEvent:(NSEvent *)event withType:(GLFMTouchPhase)phase {
    if (!self.glfmDisplay->touchFunc) {
        return;
    }

    double x, y;
    if (![self getLocationForEvent:event outX:&x outY:&y]) {
        return;
    }

    if (phase == GLFMTouchPhaseEnded) {
        GLFMWindow *window = (GLFMWindow *)self.glfmViewIfLoaded.window;
        if (!window.active) {
            phase = GLFMTouchPhaseCancelled;
        }
    }

    self.glfmDisplay->touchFunc(self.glfmDisplay, (int)event.buttonNumber, phase, x, y);
}

- (void)mouseMoved:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseHover];
}

- (void)mouseDown:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseBegan];
}

- (void)mouseDragged:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseMoved];
}

- (void)mouseUp:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseEnded];
}

- (void)rightMouseDown:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseBegan];
}

- (void)rightMouseDragged:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseMoved];
}

- (void)rightMouseUp:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseEnded];
}

- (void)otherMouseDown:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseBegan];
}

- (void)otherMouseDragged:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseMoved];
}

- (void)otherMouseUp:(NSEvent *)event {
    [self sendMouseEvent:event withType:GLFMTouchPhaseEnded];
}

- (void)scrollWheel:(NSEvent *)event {
    if (!self.glfmDisplay->mouseWheelFunc) {
        return;
    }

    double x, y;
    if (![self getLocationForEvent:event outX:&x outY:&y]) {
        return;
    }
    
    // Invert to match web browser behavior
    double deltaX = -event.scrollingDeltaX;
    double deltaY = -event.scrollingDeltaY;
    GLFMMouseWheelDeltaType deltaType = (event.hasPreciseScrollingDeltas ? GLFMMouseWheelDeltaPixel
                                         : GLFMMouseWheelDeltaLine);

    self.glfmDisplay->mouseWheelFunc(self.glfmDisplay, x, y, deltaType, deltaX, deltaY, 0.0);
}

- (void)cursorUpdate:(NSEvent *)event {
    if (event.trackingArea) {
        [self.currentCursor set];
    } else {
        [super cursorUpdate:event];
    }
}

- (void)mouseEntered:(NSEvent *)event {
    self.mouseInside = YES;
}

- (void)mouseExited:(NSEvent *)event {
    self.mouseInside = NO;
    [NSCursor.arrowCursor set];
}

// MARK: NSTextInputClient

- (void)insertText:(id)text replacementRange:(NSRange)replacementRange {
    // Input from the Character Palette
    if (self.glfmDisplay->charFunc) {
        NSString *string;
        if ([(NSObject *)text isKindOfClass:[NSAttributedString class]]) {
            string = ((NSAttributedString *)text).string;
        } else {
            string = text;
        }
        self.glfmDisplay->charFunc(self.glfmDisplay, string.UTF8String, 0);
    }
}

- (void)doCommandBySelector:(SEL)selector {

}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    
}

- (void)unmarkText {
    
}

- (NSRange)selectedRange {
    return NSMakeRange(0, 0);
}

- (NSRange)markedRange {
    return NSMakeRange(NSNotFound, 0);
}

- (BOOL)hasMarkedText {
    return NO;
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    // This is called for positioning the Character Palette, but seems to be ignored.
    NSRect frame = self.glfmViewIfLoaded.window.frame;
    return CGRectMake(NSMidX(frame), NSMidY(frame), 0, 0);
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return 0;
}

// MARK: NSResponder (Keyboard)

- (void)setCurrentKeyboard {
    if (self.currentKeyboard) {
        CFRelease(self.currentKeyboard);
    }
    self.currentKeyboard = TISCopyCurrentKeyboardInputSource();
    CFDataRef layoutData = TISGetInputSourceProperty(self.currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
    if (layoutData) {
        self.keyboardLayout = (const UCKeyboardLayout *)(const void *)CFDataGetBytePtr(layoutData);
    } else {
        self.keyboardLayout = NULL;
    }
}

- (void)keyboardChanged:(NSObject *)object {
    [self setCurrentKeyboard];
}

- (BOOL)sendKeyEvent:(NSEvent *)event withAction:(GLFMKeyAction)action {
    BOOL handled = NO;

    // Send key event
    if (self.glfmDisplay->keyFunc) {
        static const GLFMKeyCode VK_MAP[] = {
            [kVK_Return]                    = GLFMKeyCodeEnter,
            [kVK_Tab]                       = GLFMKeyCodeTab,
            [kVK_Space]                     = GLFMKeyCodeSpace,
            [kVK_Delete]                    = GLFMKeyCodeBackspace,
            [kVK_Escape]                    = GLFMKeyCodeEscape,
            [kVK_Command]                   = GLFMKeyCodeMetaLeft,
            [kVK_Shift]                     = GLFMKeyCodeShiftLeft,
            [kVK_CapsLock]                  = GLFMKeyCodeCapsLock,
            [kVK_Option]                    = GLFMKeyCodeAltLeft,
            [kVK_Control]                   = GLFMKeyCodeControlLeft,
            [kVK_RightCommand]              = GLFMKeyCodeMetaRight,
            [kVK_RightShift]                = GLFMKeyCodeShiftRight,
            [kVK_RightOption]               = GLFMKeyCodeAltRight,
            [kVK_RightControl]              = GLFMKeyCodeControlRight,
            [kVK_Function]                  = GLFMKeyCodeFunction,
            [kVK_Help]                      = GLFMKeyCodeInsert,
            [kVK_Home]                      = GLFMKeyCodeHome,
            [kVK_PageUp]                    = GLFMKeyCodePageUp,
            [kVK_ForwardDelete]             = GLFMKeyCodeDelete,
            [kVK_End]                       = GLFMKeyCodeEnd,
            [kVK_PageDown]                  = GLFMKeyCodePageDown,
            [kVK_LeftArrow]                 = GLFMKeyCodeArrowLeft,
            [kVK_RightArrow]                = GLFMKeyCodeArrowRight,
            [kVK_DownArrow]                 = GLFMKeyCodeArrowDown,
            [kVK_UpArrow]                   = GLFMKeyCodeArrowUp,
            [kVK_ANSI_A]                    = GLFMKeyCodeA,
            [kVK_ANSI_B]                    = GLFMKeyCodeB,
            [kVK_ANSI_C]                    = GLFMKeyCodeC,
            [kVK_ANSI_D]                    = GLFMKeyCodeD,
            [kVK_ANSI_E]                    = GLFMKeyCodeE,
            [kVK_ANSI_F]                    = GLFMKeyCodeF,
            [kVK_ANSI_G]                    = GLFMKeyCodeG,
            [kVK_ANSI_H]                    = GLFMKeyCodeH,
            [kVK_ANSI_I]                    = GLFMKeyCodeI,
            [kVK_ANSI_J]                    = GLFMKeyCodeJ,
            [kVK_ANSI_K]                    = GLFMKeyCodeK,
            [kVK_ANSI_L]                    = GLFMKeyCodeL,
            [kVK_ANSI_N]                    = GLFMKeyCodeN,
            [kVK_ANSI_M]                    = GLFMKeyCodeM,
            [kVK_ANSI_O]                    = GLFMKeyCodeO,
            [kVK_ANSI_P]                    = GLFMKeyCodeP,
            [kVK_ANSI_Q]                    = GLFMKeyCodeQ,
            [kVK_ANSI_R]                    = GLFMKeyCodeR,
            [kVK_ANSI_S]                    = GLFMKeyCodeS,
            [kVK_ANSI_T]                    = GLFMKeyCodeT,
            [kVK_ANSI_U]                    = GLFMKeyCodeU,
            [kVK_ANSI_V]                    = GLFMKeyCodeV,
            [kVK_ANSI_W]                    = GLFMKeyCodeW,
            [kVK_ANSI_X]                    = GLFMKeyCodeX,
            [kVK_ANSI_Y]                    = GLFMKeyCodeY,
            [kVK_ANSI_Z]                    = GLFMKeyCodeZ,
            [kVK_ANSI_0]                    = GLFMKeyCode0,
            [kVK_ANSI_1]                    = GLFMKeyCode1,
            [kVK_ANSI_2]                    = GLFMKeyCode2,
            [kVK_ANSI_3]                    = GLFMKeyCode3,
            [kVK_ANSI_4]                    = GLFMKeyCode4,
            [kVK_ANSI_5]                    = GLFMKeyCode5,
            [kVK_ANSI_6]                    = GLFMKeyCode6,
            [kVK_ANSI_7]                    = GLFMKeyCode7,
            [kVK_ANSI_8]                    = GLFMKeyCode8,
            [kVK_ANSI_9]                    = GLFMKeyCode9,
            [kVK_ANSI_Equal]                = GLFMKeyCodeEqual,
            [kVK_ANSI_Minus]                = GLFMKeyCodeMinus,
            [kVK_ANSI_RightBracket]         = GLFMKeyCodeBracketRight,
            [kVK_ANSI_LeftBracket]          = GLFMKeyCodeBracketLeft,
            [kVK_ANSI_Quote]                = GLFMKeyCodeQuote,
            [kVK_ANSI_Semicolon]            = GLFMKeyCodeSemicolon,
            [kVK_ANSI_Backslash]            = GLFMKeyCodeBackslash,
            [kVK_ANSI_Comma]                = GLFMKeyCodeComma,
            [kVK_ANSI_Slash]                = GLFMKeyCodeSlash,
            [kVK_ANSI_Period]               = GLFMKeyCodePeriod,
            [kVK_ANSI_Grave]                = GLFMKeyCodeBackquote,
            [kVK_ANSI_KeypadClear]          = GLFMKeyCodeNumLock,
            [kVK_ANSI_KeypadDecimal]        = GLFMKeyCodeNumpadDecimal,
            [kVK_ANSI_KeypadMultiply]       = GLFMKeyCodeNumpadMultiply,
            [kVK_ANSI_KeypadPlus]           = GLFMKeyCodeNumpadAdd,
            [kVK_ANSI_KeypadDivide]         = GLFMKeyCodeNumpadDivide,
            [kVK_ANSI_KeypadEnter]          = GLFMKeyCodeNumpadEnter,
            [kVK_ANSI_KeypadMinus]          = GLFMKeyCodeNumpadSubtract,
            [kVK_ANSI_KeypadEquals]         = GLFMKeyCodeNumpadEqual,
            [kVK_ANSI_Keypad0]              = GLFMKeyCodeNumpad0,
            [kVK_ANSI_Keypad1]              = GLFMKeyCodeNumpad1,
            [kVK_ANSI_Keypad2]              = GLFMKeyCodeNumpad2,
            [kVK_ANSI_Keypad3]              = GLFMKeyCodeNumpad3,
            [kVK_ANSI_Keypad4]              = GLFMKeyCodeNumpad4,
            [kVK_ANSI_Keypad5]              = GLFMKeyCodeNumpad5,
            [kVK_ANSI_Keypad6]              = GLFMKeyCodeNumpad6,
            [kVK_ANSI_Keypad7]              = GLFMKeyCodeNumpad7,
            [kVK_ANSI_Keypad8]              = GLFMKeyCodeNumpad8,
            [kVK_ANSI_Keypad9]              = GLFMKeyCodeNumpad9,
            [kVK_F1]                        = GLFMKeyCodeF1,
            [kVK_F2]                        = GLFMKeyCodeF2,
            [kVK_F3]                        = GLFMKeyCodeF3,
            [kVK_F4]                        = GLFMKeyCodeF4,
            [kVK_F5]                        = GLFMKeyCodeF5,
            [kVK_F6]                        = GLFMKeyCodeF6,
            [kVK_F7]                        = GLFMKeyCodeF7,
            [kVK_F8]                        = GLFMKeyCodeF8,
            [kVK_F9]                        = GLFMKeyCodeF9,
            [kVK_F10]                       = GLFMKeyCodeF10,
            [kVK_F11]                       = GLFMKeyCodeF11,
            [kVK_F12]                       = GLFMKeyCodeF12,
            [kVK_F13]                       = GLFMKeyCodeF13,
            [kVK_F14]                       = GLFMKeyCodeF14,
            [kVK_F15]                       = GLFMKeyCodeF15,
            [kVK_F16]                       = GLFMKeyCodeF16,
            [kVK_F17]                       = GLFMKeyCodeF17,
            [kVK_F18]                       = GLFMKeyCodeF18,
            [kVK_F19]                       = GLFMKeyCodeF19,
            [kVK_F20]                       = GLFMKeyCodeF20,
            [0x6e]                          = GLFMKeyCodeMenu,
        };

        GLFMKeyCode keyCode = GLFMKeyCodeUnknown;
        if (event.keyCode < sizeof(VK_MAP) / sizeof(*VK_MAP)) {
            keyCode = VK_MAP[event.keyCode];
        }

        int modifiers = 0;
        if ((event.modifierFlags & NSEventModifierFlagShift) != 0) {
            modifiers |= GLFMKeyModifierShift;
        }
        if ((event.modifierFlags & NSEventModifierFlagControl) != 0) {
            modifiers |= GLFMKeyModifierControl;
        }
        if ((event.modifierFlags & NSEventModifierFlagOption) != 0) {
            modifiers |= GLFMKeyModifierAlt;
        }
        if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
            modifiers |= GLFMKeyModifierMeta;
        }
        if (self.fnModifier) {
            modifiers |= GLFMKeyModifierFunction;
        }

        handled = self.glfmDisplay->keyFunc(self.glfmDisplay, keyCode, action, modifiers);
    }

    // Send char event
    if (self.glfmDisplay->charFunc &&
        event.type == NSEventTypeKeyDown &&
        (event.modifierFlags & NSEventModifierFlagFunction) == 0 &&
        (event.modifierFlags & NSEventModifierFlagCommand) == 0 &&
        (event.modifierFlags & NSEventModifierFlagControl) == 0) {

        UniChar utf16[4];
        char utf8[4 * 2 + 1] = { 0 };

        // This is like `event.characters`, but considers dead key state and ignores control codes.
        // Sets `utf8` on success.
        if (self.keyboardLayout) {
            UInt32 modifierKeyState = (event.modifierFlags >> 16) & 0xFF;
            UniCharCount utf16Length = 0;
            OSStatus status = UCKeyTranslate(self.keyboardLayout, event.keyCode, kUCKeyActionDown, modifierKeyState,
                                             LMGetKbdType(), 0, &_deadKeyState, sizeof(utf16) / sizeof(*utf16), &utf16Length, utf16);
            if (status == noErr && utf16Length > 0) {
                UniChar ch = utf16[0];
                BOOL isControlCode = (ch < 0x20 || ch == NSDeleteCharacter || ch == NSLineSeparatorCharacter || ch == NSParagraphSeparatorCharacter);
                if (!isControlCode) {
                    // Convert to UTF8
                    CFStringRef string = CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault, utf16, utf16Length, kCFAllocatorNull);
                    CFStringGetCString(string, utf8, sizeof(utf8), kCFStringEncodingUTF8);
                    CFRelease(string);
                }
            }
        }

        if (utf8[0] != '\0') {
            if (self.hideMouseCursorWhileTyping) {
                [NSCursor setHiddenUntilMouseMoves:YES];
            }
            self.glfmDisplay->charFunc(self.glfmDisplay, utf8, 0);
        }
    }
    return handled;
}

- (void)keyDown:(NSEvent *)event {
    GLFMKeyAction action = (event.isARepeat ? GLFMKeyActionRepeated : GLFMKeyActionPressed);
    BOOL handled = [self sendKeyEvent:event withAction:action];
    if (!handled) {
        // Interpret unhandled keys. For now, only interpretation is "Esc" to exit fullscreen.
        if (event.keyCode == kVK_Escape) {
            NSWindow *window = self.glfmViewIfLoaded.window;
            if (window && (window.styleMask & NSWindowStyleMaskFullScreen) != 0) {
                [window toggleFullScreen:nil];
            }
        }
    }
}

- (void)keyUp:(NSEvent *)event {
    [self sendKeyEvent:event withAction:GLFMKeyActionReleased];
}

- (void)flagsChanged:(NSEvent *)event {
    NSEventModifierFlags modifier = 0;
    switch (event.keyCode) {
        case kVK_Command: modifier = NX_DEVICELCMDKEYMASK; break;
        case kVK_Shift: modifier = NX_DEVICELSHIFTKEYMASK; break;
        case kVK_CapsLock: modifier = NSEventModifierFlagCapsLock; break;
        case kVK_Option: modifier = NX_DEVICELALTKEYMASK; break;
        case kVK_Control: modifier = NX_DEVICELCTLKEYMASK; break;
        case kVK_RightCommand: modifier = NX_DEVICERCMDKEYMASK; break;
        case kVK_RightShift: modifier = NX_DEVICERSHIFTKEYMASK; break;
        case kVK_RightOption: modifier = NX_DEVICERALTKEYMASK; break;
        case kVK_RightControl: modifier = NX_DEVICERCTLKEYMASK; break;
        case kVK_Function: modifier = NSEventModifierFlagFunction; break;
    };
    if (modifier != 0) {
        BOOL pressed = ((event.modifierFlags & modifier) != 0);
        if (event.keyCode == kVK_Function) {
            // The fn key cannot be determined from `modifierFlags` because the
            // `NSEventModifierFlagFunction` flag is also used for other keys, like the arrow keys.
            // So, keep track of it's state.
            self.fnModifier = pressed;
        }
        GLFMKeyAction action = pressed ? GLFMKeyActionPressed : GLFMKeyActionReleased;
        [self sendKeyEvent:event withAction:action];
    }
}

- (void)clearActiveKeys {
    self.fnModifier = NO;
    self.deadKeyState = 0;
}

#endif // TARGET_OS_OSX

@end // GLFMViewController

// MARK: - GLFMWindow implementation

@implementation GLFMWindow

@synthesize active = _active;

- (void)setActive:(BOOL)active {
    if (_active != active) {
        _active = active;

#if TARGET_OS_OSX
        GLFMViewController *vc = (GLFMViewController *)self.contentViewController;
#else
        GLFMViewController *vc = (GLFMViewController *)self.rootViewController;
#endif
        if (vc.glfmDisplay && vc.glfmDisplay->focusFunc) {
            vc.glfmDisplay->focusFunc(vc.glfmDisplay, _active);
        }
        if (vc.isViewLoaded) {
            if (!active) {
                // Draw once when entering the background so that a game can show "paused" state.
                [vc.glfmView requestRefresh];
                [vc.glfmView draw];
            }
            vc.glfmView.animating = active;
        }
#if TARGET_OS_IOS
        if (vc.isMotionManagerLoaded) {
            [vc updateMotionManagerActiveState];
        }
#endif
#if TARGET_OS_IOS || TARGET_OS_TV
        [vc clearTouches];
#endif
#if TARGET_OS_OSX
        [vc clearActiveKeys];
#endif
    }
}

@end

#if TARGET_OS_IOS || TARGET_OS_TV

// MARK: - UISceneDelegate (iOS, tvOS)

@interface GLFMSceneDelegate : NSObject<UISceneDelegate>

@property(nonatomic, strong) GLFMWindow *window;

@end

@implementation GLFMSceneDelegate

@synthesize window;

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session
      options:(UISceneConnectionOptions *)connectionOptions API_AVAILABLE(ios(13.0), tvos(13.0)) {
    if ([scene isKindOfClass:[UIWindowScene class]]) {
        UIWindowScene *windowScene = (UIWindowScene *)scene;
        self.window = GLFM_AUTORELEASE([[GLFMWindow alloc] initWithWindowScene:windowScene]);
        self.window.rootViewController = GLFM_AUTORELEASE([[GLFMViewController alloc] initWithDefaultFrame:self.window.bounds
                                                                                              contentScale:windowScene.screen.nativeScale]);
        [self.window makeKeyAndVisible];
    }
}

- (void)sceneDidDisconnect:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0)) {
    self.window.active = NO;
}

- (void)sceneDidBecomeActive:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0)) {
    self.window.active = YES;
}

- (void)sceneWillResignActive:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0)) {
    self.window.active = NO;
}

- (void)sceneWillEnterForeground:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0)) {
    self.window.active = YES;
}

- (void)sceneDidEnterBackground:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0)) {
    self.window.active = NO;
}

- (void)dealloc {
    self.window = nil;
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

@end

// MARK: - GLFMAppDelegate (iOS, tvOS)

@interface GLFMAppDelegate : NSObject<UIApplicationDelegate>

@property(nonatomic, strong) GLFMWindow *window; // window for iOS 12 and older

@end

@implementation GLFMAppDelegate

@synthesize window;

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions {
    if (@available(iOS 13, tvOS 13, *)) {
        // Create the window in GLFMSceneDelegate
    } else {
        self.window = GLFM_AUTORELEASE([[GLFMWindow alloc] init]);
        if (self.window.bounds.size.width <= (CGFloat)0.0 || self.window.bounds.size.height <= (CGFloat)0.0) {
            // Set UIWindow frame for iOS 8.
            // On iOS 9, the UIWindow frame may be different than the UIScreen bounds for iPad's
            // Split View or Slide Over.
            self.window.frame = [[UIScreen mainScreen] bounds];
        }
        self.window.rootViewController = GLFM_AUTORELEASE([[GLFMViewController alloc] initWithDefaultFrame:self.window.bounds
                                                                                              contentScale:[UIScreen mainScreen].nativeScale]);
        [self.window makeKeyAndVisible];
    }
    return YES;
}

- (UISceneConfiguration *)application:(UIApplication *)application
configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
                              options:(UISceneConnectionOptions *)options API_AVAILABLE(ios(13.0), tvos(13.0)) {
    UISceneConfiguration *sceneConfig = GLFM_AUTORELEASE([[UISceneConfiguration alloc] initWithName:@"GLFM Configuration"
                                                                                        sessionRole:connectingSceneSession.role]);
    sceneConfig.delegateClass = [GLFMSceneDelegate class];
    return sceneConfig;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    self.window.active = NO;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    self.window.active = NO;
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    self.window.active = YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    self.window.active = YES;
}

- (void)applicationWillTerminate:(UIApplication *)application {
    self.window.active = NO;
}

- (void)dealloc {
    self.window = nil;
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

@end

#endif // TARGET_OS_IOS || TARGET_OS_TV

#if TARGET_OS_OSX

// MARK: - GLFMAppDelegate (macOS)

@interface GLFMAppDelegate: NSObject <NSApplicationDelegate, NSWindowDelegate>

@property(nonatomic, strong) GLFMWindow *window;

@end

@implementation GLFMAppDelegate

@synthesize window;

- (void)createDefaultMenuWithAppName:(NSString *)appName {
    NSMenu *appMenu = GLFM_AUTORELEASE([NSMenu new]);
    NSApp.mainMenu = GLFM_AUTORELEASE([NSMenu new]);
    NSApp.servicesMenu = GLFM_AUTORELEASE([NSMenu new]);
    NSApp.windowsMenu = GLFM_AUTORELEASE([[NSMenu alloc] initWithTitle:NSLocalizedString(@"Window", nil)]);

    // App Menu
    [NSApp.mainMenu addItemWithTitle:@"" action:nil keyEquivalent:@""].submenu = appMenu;
    [appMenu addItemWithTitle:[NSString stringWithFormat:NSLocalizedString(@"About %@", nil), appName]
                       action:@selector(orderFrontStandardAboutPanel:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:NSLocalizedString(@"Services", nil)
                       action:nil
                keyEquivalent:@""].submenu = NSApp.servicesMenu;
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[NSString stringWithFormat:NSLocalizedString(@"Hide %@", nil), appName]
                       action:@selector(hide:)
                keyEquivalent:@"h"];
    [appMenu addItemWithTitle:NSLocalizedString(@"Hide Others", nil)
                       action:@selector(hideOtherApplications:)
                keyEquivalent:@"h"].keyEquivalentModifierMask = NSEventModifierFlagOption | NSEventModifierFlagCommand;
    [appMenu addItemWithTitle:NSLocalizedString(@"Show All", nil)
                       action:@selector(unhideAllApplications:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[NSString stringWithFormat:NSLocalizedString(@"Quit %@", nil), appName]
                       action:@selector(terminate:)
                keyEquivalent:@"q"];

    // Window Menu
    [NSApp.mainMenu addItemWithTitle:@"" action:nil keyEquivalent:@""].submenu = NSApp.windowsMenu;
    [NSApp.windowsMenu addItemWithTitle:NSLocalizedString(@"Minimize", nil)
                                 action:@selector(performMiniaturize:)
                          keyEquivalent:@"m"];
    [NSApp.windowsMenu addItemWithTitle:NSLocalizedString(@"Zoom", nil)
                                 action:@selector(performZoom:)
                          keyEquivalent:@""];
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    // App name (used for menu bar and window title) is "CFBundleName" or the process name.
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
    NSString *appName = infoDictionary[@"CFBundleName"];
    if (!appName || appName.length == 0) {
        appName = [[NSProcessInfo processInfo] processName];
    }

    // Window style: Closable, miniaturizable, resizable.
    NSWindowStyleMask windowStyle = (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                     NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable);

    // Create NSWindow centered on the main screen, One-half screen size.
    double width, height, scale;
    glfm__getDefaultDisplaySize(NULL, &width, &height, &scale);
    CGRect screenFrame = [NSScreen mainScreen].frame;
    CGRect contentFrame;
    contentFrame.origin.x = screenFrame.origin.x + screenFrame.size.width / 2 - width / 2;
    contentFrame.origin.y = screenFrame.origin.y + screenFrame.size.height / 2 - height / 2;
    contentFrame.size.width = width;
    contentFrame.size.height = height;
    self.window = GLFM_AUTORELEASE([[GLFMWindow alloc] initWithContentRect:contentFrame
                                                                 styleMask:windowStyle
                                                                   backing:NSBackingStoreBuffered
                                                                     defer:NO]);
    self.window.title = appName;
    //self.window.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone; // Make the topmost row of the view visible
    self.window.excludedFromWindowsMenu = YES; // Single-window app
    self.window.tabbingMode = NSWindowTabbingModeDisallowed; // No tabs
    self.window.releasedWhenClosed = NO;
    self.window.acceptsMouseMovedEvents = YES;
    self.window.delegate = self;

    CGRect defaultFrame = [self.window contentRectForFrameRect:self.window.frame];
    defaultFrame.origin = CGPointZero;
    GLFMViewController *glfmViewController = GLFM_AUTORELEASE([[GLFMViewController alloc] initWithDefaultFrame:defaultFrame
                                                                                                  contentScale:(CGFloat)scale]);
    
    // Set the contentViewController and call glfmMain() (in loadView).
    self.window.contentViewController = glfmViewController;
    if (!self.window.contentViewController.isViewLoaded) {
        [self.window.contentViewController loadView];
    }
    
    // Check if window size should change (due to orientation setting)
    glfm__getDefaultDisplaySize(glfmViewController.glfmDisplay, &width, &height, &scale);
    if (!glfm__isCGFloatEqual(width, (double)contentFrame.size.width) ||
        glfm__isCGFloatEqual(height, (double)contentFrame.size.height)) {
        contentFrame.origin.x = screenFrame.origin.x + screenFrame.size.width / 2 - width / 2;
        contentFrame.origin.y = screenFrame.origin.y + screenFrame.size.height / 2 - height / 2;
        contentFrame.size.width = width;
        contentFrame.size.height = height;
        [self.window setFrame:[self.window frameRectForContentRect:contentFrame] display:NO];
    }
    
    // Create default menu if one wasn't created in glfmMain()
    if (!NSApp.mainMenu) {
        [self createDefaultMenuWithAppName:appName];
    }

    // Enter fullscreen if requested
    glfm__displayChromeUpdated(glfmViewController.glfmDisplay);
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Draw first before showing the window
    GLFMViewController *glfmViewController = (GLFMViewController *)self.window.contentViewController;
    [glfmViewController.glfmViewIfLoaded draw];
    
    if (NSApp.activationPolicy == NSApplicationActivationPolicyRegular) {
        [self.window makeKeyAndOrderFront:nil];
    } else {
        // Executable-only (unbundled) app
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [self.window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

- (void)applicationWillResignActive:(NSNotification *)notification {
    self.window.active = NO;
}

- (void)applicationDidBecomeActive:(NSNotification *)notification {
    self.window.active = !self.window.miniaturized;
}

- (void)windowWillMiniaturize:(NSNotification *)notification {
    self.window.active = NO;
}

- (void)windowDidDeminiaturize:(NSNotification *)notification  {
    self.window.active = NSApp.active;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    if (self.window) {
        // Terminate later in windowWillClose:
        [self.window close];
        return NSTerminateCancel;
    } else {
        return NSTerminateNow;
    }
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification {
    if (self.window == notification.object) {
        GLFMViewController *glfmViewController = (GLFMViewController *)self.window.contentViewController;
        self.window.styleMask |= NSWindowStyleMaskFullScreen; // Style isn't set before notification
        glfm__displayChromeUpdated(glfmViewController.glfmDisplay);
    }
}

- (void)windowWillExitFullScreen:(NSNotification *)notification {
    if (self.window == notification.object) {
        GLFMViewController *glfmViewController = (GLFMViewController *)self.window.contentViewController;
        self.window.styleMask &= ~NSWindowStyleMaskFullScreen; // Style isn't set before notification
        glfm__displayChromeUpdated(glfmViewController.glfmDisplay);
    }
}

- (void)windowWillClose:(NSNotification *)notification {
    if (self.window == notification.object) {
        self.window.active = NO;
        self.window = nil;
        // Dispatch later, after surfaceDestroyedFunc is called
        dispatch_async(dispatch_get_main_queue(), ^{
            [NSApp terminate:nil];
        });
    }
}

- (void)dealloc {
    self.window = nil;
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

@end

#endif // TARGET_OS_OSX

// MARK: - Main

#if TARGET_OS_IOS || TARGET_OS_TV

int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([GLFMAppDelegate class]));
    }
}

#else

int main(int argc, const char * argv[]) {
    (void)argc;
    (void)argv;
    @autoreleasepool {
        // Create the sharedApplication instance from "NSPrincipalClass"
        NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
        NSString *principalClassName = infoDictionary[@"NSPrincipalClass"];
        Class principalClass = NSClassFromString(principalClassName);
        if ([principalClass respondsToSelector:@selector(sharedApplication)]) {
            [principalClass sharedApplication];
        } else {
            [NSApplication sharedApplication];
        }
        
        // Set the delegate and run
        GLFMAppDelegate *delegate = [GLFMAppDelegate new];
        [NSApp setDelegate:delegate];
        [NSApp run];
        GLFM_RELEASE(delegate);
    }
    return 0;
}
 
#endif // TARGET_OS_OSX

// MARK: - GLFM private functions

static void glfm__getDefaultDisplaySize(const GLFMDisplay *display,
                                        double *width, double *height, double *scale) {
#if TARGET_OS_IOS || TARGET_OS_TV
    (void)display;
    CGSize size = UIScreen.mainScreen.bounds.size;
    *width = (double)size.width;
    *height = (double)size.height;
    *scale = UIScreen.mainScreen.nativeScale;
#else
    NSScreen *screen = [NSScreen mainScreen];
    *scale = screen.backingScaleFactor;
    
    // Create a window half the size of the screen.
    // If portrait orientation (but not landscape) is specified, use a portrait-sized default window.
    GLFMInterfaceOrientation supportedOrientations = !display ? GLFMInterfaceOrientationUnknown : display->supportedOrientations;
    if (((supportedOrientations & GLFMInterfaceOrientationPortrait) != 0 ||
         (supportedOrientations & GLFMInterfaceOrientationPortraitUpsideDown) != 0) &&
        (supportedOrientations & GLFMInterfaceOrientationLandscapeLeft) == 0 &&
        (supportedOrientations & GLFMInterfaceOrientationLandscapeRight) == 0) {
        *width = screen.frame.size.height / 2;
        *height = screen.frame.size.width / 2;
    } else {
        *width = screen.frame.size.width / 2;
        *height = screen.frame.size.height / 2;
    }
#endif
}

/// Get drawable size in pixels from display dimensions in points.
static void glfm__getDrawableSize(double displayWidth, double displayHeight, double displayScale,
                                  int *width, int *height) {
    int newDrawableWidth = (int)(displayWidth * displayScale);
    int newDrawableHeight = (int)(displayHeight * displayScale);

#if TARGET_OS_IOS
    // On the iPhone 6 when "Display Zoom" is set, the size will be incorrect.
    if (glfm__isCGFloatEqual(displayScale, (CGFloat)2.343750)) {
        if (newDrawableWidth == 750 && newDrawableHeight == 1331) {
            newDrawableHeight = 1334;
        } else if (newDrawableWidth == 1331 && newDrawableHeight == 750) {
            newDrawableWidth = 1334;
        }
    }
#endif

    if (width) *width = newDrawableWidth;
    if (height) *height = newDrawableHeight;
}

static void glfm__displayChromeUpdated(GLFMDisplay *display) {
    if (display && display->platformData) {
#if TARGET_OS_IOS
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        [vc.glfmViewIfLoaded requestRefresh];
        [vc setNeedsStatusBarAppearanceUpdate];
        if (@available(iOS 11, *)) {
            [vc setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
        }
#elif TARGET_OS_OSX
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        NSWindow *window = vc.glfmViewIfLoaded.window;
        if (window) {
            // This might not make sense and will probably change.
            // * GLFMUserInterfaceChromeNavigation: Full content view (title bar overlays content).
            // * GLFMUserInterfaceChromeNavigationAndStatusBar: Title bar outside content view (no overlay).
            // * GLFMUserInterfaceChromeNone: No window decor, but user can drag title bar area to move the window.
            // * GLFMUserInterfaceChromeNone when window is fullscreen: No menu bar.

            // Prevent title bar buttons from being tapped when their alpha is zero
            BOOL hideButtons = ((window.styleMask & NSWindowStyleMaskFullScreen) == 0 &&
                                display->uiChrome == GLFMUserInterfaceChromeNone);
            NSView *titleBarView = [window standardWindowButton:NSWindowCloseButton].superview;
            for (NSView *button in titleBarView.subviews) {
                if ([button isKindOfClass:[NSControl class]]) {
                    [button setHidden:hideButtons];
                }
            }

            // Hide/Show title bar
            if (@available(macOS 11, *)) {
                titleBarView = titleBarView.superview;
            }
            if (window.styleMask & NSWindowStyleMaskFullScreen) {
                titleBarView.alphaValue = (CGFloat)1.0;
                [NSMenu setMenuBarVisible:display->uiChrome != GLFMUserInterfaceChromeNone];
            } else {
                [NSMenu setMenuBarVisible:YES];
                if (display->uiChrome == GLFMUserInterfaceChromeNone) {
                    if ((window.styleMask & NSWindowStyleMaskFullSizeContentView) == 0) {
                        titleBarView.alphaValue = (CGFloat)0.0;
                    } else {
                        [[titleBarView animator] setAlphaValue:(CGFloat)0.0];
                    }
                } else {
                    [[titleBarView animator] setAlphaValue:(CGFloat)1.0];
                }

                // Enable/disable full-size content view (under the title bar)
                if (display->uiChrome == GLFMUserInterfaceChromeNavigationAndStatusBar) {
                    window.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
                } else {
                    window.styleMask |= NSWindowStyleMaskFullSizeContentView;
                }
            }
        }
#endif
    }
}

static void glfm__sensorFuncUpdated(GLFMDisplay *display) {
#if TARGET_OS_IOS
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        [vc updateMotionManagerActiveState];
    }
#else
    (void)display;
#endif
}

// MARK: - GLFM public functions

double glfmGetTime() {
    return CACurrentMediaTime();
}

GLFMProc glfmGetProcAddress(const char *functionName) {
    static void *handle = NULL;
    if (!handle) {
        handle = dlopen(NULL, RTLD_LAZY);
    }
    return handle ? (GLFMProc)dlsym(handle, functionName) : NULL;
}

void glfmSwapBuffers(GLFMDisplay *display) {
    if (display && display->platformData) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        [vc.glfmViewIfLoaded swapBuffers];
    }
}

void glfmSetSupportedInterfaceOrientation(GLFMDisplay *display, GLFMInterfaceOrientation supportedOrientations) {
    if (display) {
        if (display->supportedOrientations != supportedOrientations) {
            display->supportedOrientations = supportedOrientations;
#if TARGET_OS_IOS
            GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
            if (@available(iOS 16, *)) {
                [vc setNeedsUpdateOfSupportedInterfaceOrientations];
            } else if (vc.isViewLoaded && vc.view.window) {
                // HACK: Notify that the value of supportedInterfaceOrientations has changed
                [vc.glfmView requestRefresh];
                UIViewController *dummyVC = GLFM_AUTORELEASE([[UIViewController alloc] init]);
                dummyVC.view = GLFM_AUTORELEASE([[UIView alloc] init]);
                [vc presentViewController:dummyVC animated:NO completion:^{
                    [vc dismissViewControllerAnimated:NO completion:NULL];
                }];
            }
#endif
        }
    }
}

GLFMInterfaceOrientation glfmGetInterfaceOrientation(const GLFMDisplay *display) {
    (void)display;
#if TARGET_OS_IOS
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    switch (orientation) {
        case UIInterfaceOrientationPortrait:
            return GLFMInterfaceOrientationPortrait;
        case UIInterfaceOrientationPortraitUpsideDown:
            return GLFMInterfaceOrientationPortraitUpsideDown;
        case UIInterfaceOrientationLandscapeLeft:
            return GLFMInterfaceOrientationLandscapeLeft;
        case UIInterfaceOrientationLandscapeRight:
            return GLFMInterfaceOrientationLandscapeRight;
        case UIInterfaceOrientationUnknown: default:
            return GLFMInterfaceOrientationUnknown;
    }
#else
    return GLFMInterfaceOrientationUnknown;
#endif
}

void glfmGetDisplaySize(const GLFMDisplay *display, int *width, int *height) {
    if (display && display->platformData) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        if (vc.isViewLoaded) {
            if (width) *width = vc.glfmView.drawableWidth;
            if (height) *height = vc.glfmView.drawableHeight;
        } else {
            double displayWidth, displayHeight, displayScale;
            glfm__getDefaultDisplaySize(display, &displayWidth, &displayHeight, &displayScale);
            glfm__getDrawableSize(displayWidth, displayHeight, displayScale, width, height);
        }
    } else {
        if (width) *width = 0;
        if (height) *height = 0;
    }
}

double glfmGetDisplayScale(const GLFMDisplay *display) {
#if TARGET_OS_OSX
    NSWindow *window = nil;
    if (display && display->platformData) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        window = vc.glfmViewIfLoaded.window;
    }
    return window ? window.backingScaleFactor : [NSScreen mainScreen].backingScaleFactor;
#else
    (void)display;
    return (double)[UIScreen mainScreen].nativeScale;
#endif
}

void glfmGetDisplayChromeInsets(const GLFMDisplay *display, double *top, double *right,
                                double *bottom, double *left) {
    if (display && display->platformData) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        if (!vc.isViewLoaded) {
            if (top) *top = 0.0;
            if (right) *right = 0.0;
            if (bottom) *bottom = 0.0;
            if (left) *left = 0.0;
        } else if (@available(iOS 11, tvOS 11, macOS 11, *)) {
#if TARGET_OS_IOS || TARGET_OS_TV
            UIEdgeInsets insets = vc.view.safeAreaInsets;
            if (top) *top = (double)(insets.top * vc.view.contentScaleFactor);
            if (right) *right = (double)(insets.right * vc.view.contentScaleFactor);
            if (bottom) *bottom = (double)(insets.bottom * vc.view.contentScaleFactor);
            if (left) *left = (double)(insets.left * vc.view.contentScaleFactor);
#else
            // NOTE: This has not been tested.
            // Run glfm_test_pattern fullscreen on a 2021-2022 MacBook Pro/Air with a notch.
            NSEdgeInsets insets = vc.view.safeAreaInsets;
            if (top) *top = (double)(insets.top * vc.view.layer.contentsScale);
            if (right) *right = (double)(insets.right * vc.view.layer.contentsScale);
            if (bottom) *bottom = (double)(insets.bottom * vc.view.layer.contentsScale);
            if (left) *left = (double)(insets.left * vc.view.layer.contentsScale);
#endif
        } else {
            if (top) {
#if TARGET_OS_IOS
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                if (![vc prefersStatusBarHidden]) {
                    *top = (double)([UIApplication sharedApplication].statusBarFrame.size.height *
                                    vc.view.contentScaleFactor);
                } else {
                    *top = 0.0;
                }
#pragma clang diagnostic pop
#else
                *top = 0.0;
#endif
            }
            if (right) *right = 0.0;
            if (bottom) *bottom = 0.0;
            if (left) *left = 0.0;
        }
    } else {
        if (top) *top = 0.0;
        if (right) *right = 0.0;
        if (bottom) *bottom = 0.0;
        if (left) *left = 0.0;
    }
}

GLFMRenderingAPI glfmGetRenderingAPI(const GLFMDisplay *display) {
    if (display && display->platformData) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        if (vc.isViewLoaded) {
            return vc.glfmView.renderingAPI;
        } else {
            return GLFMRenderingAPIOpenGLES2;
        }
    } else {
        return GLFMRenderingAPIOpenGLES2;
    }
}

bool glfmHasTouch(const GLFMDisplay *display) {
    (void)display;
#if TARGET_OS_IOS || TARGET_OS_TV
    return true;
#else
    return false;
#endif
}

void glfmSetMouseCursor(GLFMDisplay *display, GLFMMouseCursor mouseCursor) {
#if TARGET_OS_OSX
    if (display && display->platformData) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        NSCursor *cursor;
        switch (mouseCursor) {
            case GLFMMouseCursorAuto:
            case GLFMMouseCursorDefault:
            default:
                cursor = NSCursor.arrowCursor;
                break;
            case GLFMMouseCursorPointer:
                cursor = NSCursor.pointingHandCursor;
                break;
            case GLFMMouseCursorCrosshair:
                cursor = NSCursor.crosshairCursor;
                break;
            case GLFMMouseCursorText:
                cursor = NSCursor.IBeamCursor;
                break;
            case GLFMMouseCursorVerticalText:
                cursor = NSCursor.IBeamCursorForVerticalLayout;
                break;
            case GLFMMouseCursorNone:
                cursor = vc.transparentCursor;
                break;
        }
        if (vc.currentCursor != cursor) {
            vc.currentCursor = cursor;
            GLFMWindow *window = (GLFMWindow *)vc.glfmViewIfLoaded.window;
            if (vc.mouseInside && window.active) {
                [cursor set];
            }
        }
        [NSCursor setHiddenUntilMouseMoves:NO];
        vc.hideMouseCursorWhileTyping = (mouseCursor == GLFMMouseCursorAuto ||
                                         mouseCursor == GLFMMouseCursorText ||
                                         mouseCursor == GLFMMouseCursorVerticalText);
    }
#elif TARGET_OS_IOS
    if (@available(iOS 13.4, *)) {
        if (display && display->platformData) {
            GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
            if (vc.mouseCursor != mouseCursor) {
                vc.mouseCursor = mouseCursor;
                NSArray<id<UIInteraction>> *interactions = vc.viewIfLoaded.interactions;
                if (interactions) {
                    for (id<UIInteraction> interaction in interactions) {
                        if ([interaction isKindOfClass:[UIPointerInteraction class]]) {
                            UIPointerInteraction *pointerInteraction = (UIPointerInteraction *)interaction;
                            [pointerInteraction invalidate];
                        }
                    }
                }
            }
        }
    }
#else
    (void)display;
    (void)mouseCursor;
    // Do nothing
#endif
}

void glfmSetMultitouchEnabled(GLFMDisplay *display, bool multitouchEnabled) {
#if TARGET_OS_IOS
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        vc.multipleTouchEnabled = (BOOL)multitouchEnabled;
        vc.glfmViewIfLoaded.multipleTouchEnabled = (BOOL)multitouchEnabled;
    }
#else
    (void)display;
    (void)multitouchEnabled;
#endif
}

bool glfmGetMultitouchEnabled(const GLFMDisplay *display) {
#if TARGET_OS_IOS
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        return vc.multipleTouchEnabled;
    } else {
        return false;
    }
#else
    (void)display;
    return false;
#endif
}

bool glfmHasVirtualKeyboard(const GLFMDisplay *display) {
    (void)display;
#if TARGET_OS_IOS
    return true;
#else
    return false;
#endif
}

void glfmSetKeyboardVisible(GLFMDisplay *display, bool visible) {
#if TARGET_OS_IOS
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        [vc resignFirstResponder];
        vc.keyboardRequested = visible;
        [vc becomeFirstResponder];
    }
#else
    (void)display;
    (void)visible;
#endif
}

bool glfmIsKeyboardVisible(const GLFMDisplay *display) {
#if TARGET_OS_IOS
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        return vc.keyboardRequested;
    } else {
        return false;
    }
#else
    (void)display;
    return false;
#endif
}

bool glfmIsSensorAvailable(const GLFMDisplay *display, GLFMSensor sensor) {
#if TARGET_OS_IOS
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        switch (sensor) {
            case GLFMSensorAccelerometer:
                return vc.motionManager.deviceMotionAvailable && vc.motionManager.accelerometerAvailable;
            case GLFMSensorMagnetometer:
                return vc.motionManager.deviceMotionAvailable && vc.motionManager.magnetometerAvailable;
            case GLFMSensorGyroscope:
                return vc.motionManager.deviceMotionAvailable && vc.motionManager.gyroAvailable;
            case GLFMSensorRotationMatrix:
                return (vc.motionManager.deviceMotionAvailable &&
                        ([CMMotionManager availableAttitudeReferenceFrames] & CMAttitudeReferenceFrameXMagneticNorthZVertical));
        }
    }
    return false;
#else
    (void)display;
    (void)sensor;
    return false;
#endif
}

bool glfmIsHapticFeedbackSupported(const GLFMDisplay *display) {
    (void)display;
#if TARGET_OS_IOS
    if (@available(iOS 13, *)) {
        return [CHHapticEngine capabilitiesForHardware].supportsHaptics;
    } else {
        return false;
    }
#else
    return false;
#endif
}

void glfmPerformHapticFeedback(GLFMDisplay *display, GLFMHapticFeedbackStyle style) {
    (void)display;
#if TARGET_OS_IOS
    if (@available(iOS 10, *)) {
        UIImpactFeedbackStyle uiStyle;
        switch (style) {
            case GLFMHapticFeedbackLight: default:
                uiStyle = UIImpactFeedbackStyleLight;
                break;
            case GLFMHapticFeedbackMedium:
                uiStyle = UIImpactFeedbackStyleMedium;
                break;
            case GLFMHapticFeedbackHeavy:
                uiStyle = UIImpactFeedbackStyleHeavy;
                break;
        }
        UIImpactFeedbackGenerator *generator = [[UIImpactFeedbackGenerator alloc] initWithStyle:uiStyle];
        [generator impactOccurred];
        GLFM_RELEASE(generator);
    }
#else
    (void)style;
#endif
}

#if TARGET_OS_TV

bool glfmHasClipboardText(const GLFMDisplay *display) {
    (void)display;
    return false;
}

void glfmRequestClipboardText(GLFMDisplay *display, GLFMClipboardTextFunc clipboardTextFunc) {
    (void)display;
    if (clipboardTextFunc) {
        clipboardTextFunc(display, NULL);
    }
}

bool glfmSetClipboardText(GLFMDisplay *display, const char *string) {
    (void)display;
    (void)string;
    return false;
}

#else

bool glfmHasClipboardText(const GLFMDisplay *display) {
    (void)display;
#if TARGET_OS_OSX
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    return [pasteboard.types containsObject:NSPasteboardTypeString];
#else
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    return [pasteboard hasStrings];
#endif
}

void glfmRequestClipboardText(GLFMDisplay *display, GLFMClipboardTextFunc clipboardTextFunc) {
    if (!clipboardTextFunc) {
        return;
    }

    // Run in a background thread because the text may be retrieved from the network (iCloud clipboard)
    // or show a confirmation dialog on iOS.
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSString *text = nil;
        if (glfmHasClipboardText(display)) {
#if TARGET_OS_OSX
            NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
            text = [pasteboard stringForType:NSPasteboardTypeString];
#else
            UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
            text = pasteboard.string;
#endif
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            clipboardTextFunc(display, text.UTF8String);
        });
    });
}

bool glfmSetClipboardText(GLFMDisplay *display, const char *string) {
    (void)display;
    @autoreleasepool {
        NSString *text = string ? [NSString stringWithUTF8String:string] : nil;
        if (!text) {
            return false;
        }
#if TARGET_OS_OSX
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard declareTypes:@[NSPasteboardTypeString] owner:nil];
        return ([pasteboard setString:text forType:NSPasteboardTypeString] == YES);
#else
        [UIPasteboard generalPasteboard].string = text;
        return true;
#endif
    }
}

#endif // !TARGET_OS_TV

// MARK: - Apple-specific functions

bool glfmIsMetalSupported(const GLFMDisplay *display) {
#if GLFM_INCLUDE_METAL
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        return (vc.metalDevice != nil);
    }
#endif
    return false;
}

void *glfmGetMetalView(const GLFMDisplay *display) {
#if GLFM_INCLUDE_METAL
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        UIView<GLFMView> *view = vc.glfmViewIfLoaded;
        if ([view isKindOfClass:[MTKView class]]) {
            return (__bridge void *)view;
        }
    }
#endif
    return NULL;
}

void *glfmGetViewController(const GLFMDisplay *display) {
    if (display) {
        GLFMViewController *vc = (__bridge GLFMViewController *)display->platformData;
        return (__bridge void *)vc;
    } else {
        return NULL;
    }
}

#endif // __APPLE__
