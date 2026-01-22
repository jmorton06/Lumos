#include "iOSOS.h"
#include "iOSWindow.h"
#include "iOSKeyCodes.h"
#include "Core/OS/FileSystem.h"
#include "Core/OS/Input.h"
#include "Core/CoreSystem.h"
#include "Core/Application.h"

#ifdef LUMOS_RENDER_API_VULKAN
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>
#else
#define LUMOS_RENDER_API_OPENGL
#import <GLKit/GLKit.h>
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

#include <sys/sysctl.h>
#include <SystemConfiguration/SCNetworkReachability.h>
#include <netinet/in.h>

#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioServices.h>

#define MAX_SIMULTANEOUS_TOUCHES 10

namespace Lumos
{
	static CAMetalLayer* s_Layer = nullptr;
	CAMetalLayer* iOSOS::GetStaticLayer() { return s_Layer; }

    iOSOS::iOSOS()
    {
    }

    iOSOS::~iOSOS()
    {
    }

    void AudioInterruptionListenerCallback(void* user_data, UInt32 interruption_state)
    {
       if (kAudioSessionBeginInterruption == interruption_state)
       {
           //alcMakeContextCurrent(NULL);
       }
       else if (kAudioSessionEndInterruption == interruption_state)
       {
           AudioSessionSetActive(true);
           //alcMakeContextCurrent(openALContext);
       }
    }

    void iOSOS::Init()
    {
        Lumos::Internal::CoreSystem::Init(false);
        LINFO("Device : %s",GetModelName().c_str());

        iOSWindow::MakeDefault();

        //TODO: Replace with non depricated functions
        AudioSessionInitialize(NULL, NULL, AudioInterruptionListenerCallback, NULL);

        UInt32 session_category = kAudioSessionCategory_MediaPlayback;
        AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(session_category), &session_category);

        AudioSessionSetActive(true);

        auto app = Lumos::CreateApplication();
        app->Init();
    }

    bool iOSOS::OnFrame()
    {
        return Application::Get().OnFrame();
    }

    void iOSOS::OnQuit()
    {
        Application::Get().OnQuit();
        Application::Release();
	    Lumos::Internal::CoreSystem::Shutdown();
    }

    std::string iOSOS::GetAssetPath()
    {
        return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/"].UTF8String;
    }

	std::string iOSOS::GetCurrentWorkingDirectory()
	{
		@autoreleasepool {
			NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
			NSString *documentsPath = [paths firstObject];
			return std::string([documentsPath UTF8String]);
		}
	}

    void iOSOS::OnKeyPressed(char keycode, bool down, bool cmd, bool shift, bool alt, bool ctrl)
    {
        Lumos::InputCode::Key lumosKey = Lumos::iOSKeyCodes::iOSKeyToLumos(keycode);

        iOSWindow* window = (iOSWindow*)Application::Get().GetWindow();
        if (cmd) window->OnKeyEvent(Lumos::InputCode::Key::LeftSuper, down);
        if (shift) window->OnKeyEvent(Lumos::InputCode::Key::LeftShift, down);
        if (alt) window->OnKeyEvent(Lumos::InputCode::Key::LeftAlt, down);
        if (ctrl) window->OnKeyEvent(Lumos::InputCode::Key::LeftControl, down);

        window->OnKeyEvent(lumosKey, down);
    }

    void iOSOS::OnScreenPressed(uint32_t x, uint32_t y, uint32_t count, bool down)
    {
        ((iOSWindow*)Application::Get().GetWindow())->OnTouchEvent(x,y,count, down);
    }

    void iOSOS::OnMouseMovedEvent(uint32_t xPos, uint32_t yPos)
    {
        ((iOSWindow*)Application::Get().GetWindow())->OnMouseMovedEvent(xPos,yPos);
    }

    void iOSOS::OnScreenResize(uint32_t width, uint32_t height)
    {
        LINFO("Resizing Window %u, %u", width, height);
        ((iOSWindow*)Application::Get().GetWindow())->OnResizeEvent(width, height);
    }

    void iOSOS::OnGesturePinch(float scale, float velocity, uint32_t x, uint32_t y, GestureState state)
    {
        ((iOSWindow*)Application::Get().GetWindow())->OnGesturePinchEvent(scale, velocity, x, y, state);
    }

    void iOSOS::OnGesturePan(float tx, float ty, float vx, float vy, uint32_t touches, GestureState state)
    {
        ((iOSWindow*)Application::Get().GetWindow())->OnGesturePanEvent(tx, ty, vx, vy, touches, state);
    }

    void iOSOS::OnGestureSwipe(SwipeDirection direction, uint32_t touches)
    {
        ((iOSWindow*)Application::Get().GetWindow())->OnGestureSwipeEvent(direction, touches);
    }

    void iOSOS::OnGestureLongPress(uint32_t x, uint32_t y, GestureState state)
    {
        ((iOSWindow*)Application::Get().GetWindow())->OnGestureLongPressEvent(x, y, state);
    }

    std::string iOSOS::GetExecutablePath()
    {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectory = [paths objectAtIndex:0];
        return [documentsDirectory stringByAppendingString: @"/"].UTF8String;
    }

    void iOSOS::Vibrate() const
    {
        @autoreleasepool
        {
            AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
        }
    }

    iOSOS::iOSDeviceType iOSOS::GetDeviceType() const
    {
        @autoreleasepool
        {
            if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
            {
                return iOSDeviceType::iPad;
            }
            return iOSDeviceType::iPhone;
        }
    }

    bool iOSOS::IsLandscape() const
    {
        @autoreleasepool
        {
            UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
            return UIInterfaceOrientationIsLandscape(orientation);
        }
    }

    void iOSOS::Alert(const char *message, const char *title)
    {
        NSString* nsmsg = [[NSString alloc] initWithUTF8String:message];
        UIAlertController* alert = [UIAlertController
                                      alertControllerWithTitle:@"Error"
                                      message:nsmsg
                                      preferredStyle:UIAlertControllerStyleAlert];

        UIAlertAction* okButton = [UIAlertAction
                                     actionWithTitle:@"OK"
                                     style:UIAlertActionStyleDefault
                                     handler:^(UIAlertAction* action) {
                                          // Handle your ok button action here
                                     }];

        [alert addAction:okButton];

        UIViewController* viewController = [[[[UIApplication sharedApplication] delegate] window] rootViewController];
        [viewController presentViewController:alert animated:YES completion:nil];
    }

	iOSOS* iOSOS::Get()
	{
		Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();
		return (iOSOS*)os;
	}

    std::string iOSOS::GetModelName() const
    {
        size_t size;
        sysctlbyname("hw.machine", NULL, &size, NULL, 0);
        char *model = (char *)malloc(size);
        if (model == NULL) {
            return "";
        }
        sysctlbyname("hw.machine", model, &size, NULL, 0);
        NSString *platform = [NSString stringWithCString:model encoding:NSUTF8StringEncoding];
        free(model);
        const char *str = [platform UTF8String];
        return std::string(str != NULL ? str : "");
    }

    void iOSOS::ShowKeyboard(bool bShow)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            UIViewController *rootVC = [[[UIApplication sharedApplication] delegate] window].rootViewController;
            if ([rootVC respondsToSelector:@selector(setKeyboardRequested:)]) {
                [(id)rootVC setKeyboardRequested:bShow];
                if (bShow) {
                    [rootVC becomeFirstResponder];
                } else {
                    [rootVC resignFirstResponder];
                }
            }
        });
    }

    bool iOSOS::HasWifiConnection()
    {
        struct sockaddr_in zeroAddress;
        memset(&zeroAddress, 0, sizeof(zeroAddress));
        zeroAddress.sin_len = sizeof(zeroAddress);
        zeroAddress.sin_family = AF_INET;

        SCNetworkReachabilityRef reachabilityRef = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr *)&zeroAddress);
        SCNetworkReachabilityFlags reachabilityFlags;
        bool isFlagsAvailable = SCNetworkReachabilityGetFlags(reachabilityRef, &reachabilityFlags);
        CFRelease(reachabilityRef);

        bool hasActiveWiFiConnection = false;
        if (isFlagsAvailable)
        {
           bool isReachable = (reachabilityFlags & kSCNetworkReachabilityFlagsReachable) != 0 &&
                              (reachabilityFlags & kSCNetworkReachabilityFlagsConnectionRequired) == 0 &&
                              // in case kSCNetworkReachabilityFlagsConnectionOnDemand || kSCNetworkReachabilityFlagsConnectionOnTraffic
                              (reachabilityFlags & kSCNetworkReachabilityFlagsInterventionRequired) == 0;

           hasActiveWiFiConnection = isReachable && (reachabilityFlags & kSCNetworkReachabilityFlagsIsWWAN) == 0;
        }

        return hasActiveWiFiConnection;
    }
}

#pragma mark - LumosView

@protocol LumosView

@property(nonatomic, readonly) int drawableWidth;
@property(nonatomic, readonly) int drawableHeight;
@property(nonatomic, assign) BOOL animating;

- (void)draw;

@end

#pragma mark - LumosMetalView

@interface LumosMetalView : MTKView <LumosView, MTKViewDelegate>

@property(nonatomic, assign) int drawableWidth;
@property(nonatomic, assign) int drawableHeight;
@property(nonatomic, assign) BOOL surfaceCreatedNotified;
@property(nonatomic, strong) NSMutableArray<NSValue*> *resizeQueue;

@end

@implementation LumosMetalView

+ (Class)layerClass {
#if TARGET_OS_SIMULATOR
	if (@available(iOS 13.0, *)) {
		return [CAMetalLayer class];
	}
	return [CALayer class];
#else
	return [CAMetalLayer class];
#endif
}

- (void)dealloc
{
	NSLog(@"LumosMetalView is being deallocated.");
	[super dealloc]; // Use only if ARC is disabled
}

@dynamic animating;

- (instancetype)initWithFrame:(CGRect)frame contentScaleFactor:(CGFloat)contentScaleFactor
                       device:(id<MTLDevice>)device {
    if ((self = [super initWithFrame:frame device:device]))
    {
		NSLog(@"LumosMetalView is being initialised.");
        self.contentScaleFactor = contentScaleFactor;
        self.resizeQueue = [NSMutableArray array];
        self.delegate = self;
        self.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
        self.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
        self.sampleCount = 1;

		// Configure the Metal layer
		CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
		metalLayer.device = device;
		metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
		metalLayer.framebufferOnly = YES; // Optimize for rendering performance.
    }
    return self;
}

- (BOOL)animating {
    return !self.paused;
}

- (void)setAnimating:(BOOL)animating {
    self.paused = !animating;
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    if (size.width > 0 && size.height > 0) {
        self.drawableWidth = (int)size.width;
        self.drawableHeight = (int)size.height;
        [self.resizeQueue addObject:[NSValue valueWithCGSize:size]];
    }
}

- (void)drawInMTKView:(MTKView *)view {
    int newDrawableWidth = (int)self.drawableSize.width;
    int newDrawableHeight = (int)self.drawableSize.height;
    if (!self.surfaceCreatedNotified)
    {
        self.surfaceCreatedNotified = YES;

        self.drawableWidth = newDrawableWidth;
        self.drawableHeight = newDrawableHeight;

    }
    else if (newDrawableWidth != self.drawableWidth || newDrawableHeight != self.drawableHeight)
    {
        self.drawableWidth = newDrawableWidth;
        self.drawableHeight = newDrawableHeight;
        [self.resizeQueue addObject:[NSValue valueWithCGSize:CGSizeMake(newDrawableWidth, newDrawableHeight)]];
    }

   Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();
   if(!os->OnFrame())
   {
	   os->OnQuit();
   }

   if(self.resizeQueue.count > 0)
   {
	Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();
    for (NSValue *sizeValue in self.resizeQueue) {
        CGSize size = [sizeValue CGSizeValue];
        os->OnScreenResize((int)size.width, (int)size.height);
    }
    [self.resizeQueue removeAllObjects];
    }
}

- (void)layoutSubviews
{
    [super layoutSubviews];

    CGSize size = self.drawableSize;
    if (size.width > 0 && size.height > 0 && !self.surfaceCreatedNotified)
    {
        self.surfaceCreatedNotified = YES;
        self.drawableWidth = (int)size.width;
        self.drawableHeight = (int)size.height;

        Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();
        os->OnScreenResize(self.drawableWidth, self.drawableHeight);

        [self draw];
    }
}
@end

@interface LumosAppDelegate : UIResponder <UIApplicationDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, assign) BOOL active;
@end

UIView<LumosView> *lumosView = nil;
CALayer* layer;

#pragma mark - LumosViewController

@interface LumosViewController : UIViewController<UIKeyInput, UITextInputTraits, UIPointerInteractionDelegate, UIGestureRecognizerDelegate> {
    const void *activeTouches[MAX_SIMULTANEOUS_TOUCHES];
}

@property(nonatomic, strong) id<MTLDevice> metalDevice;
@property(nonatomic, assign) BOOL multipleTouchEnabled;
@property(nonatomic, assign) BOOL keyboardRequested;
@property(nonatomic, assign) BOOL keyboardVisible;
@property(nonatomic, strong) UIPinchGestureRecognizer *pinchGesture;
@property(nonatomic, strong) UIPanGestureRecognizer *panGesture;

// Single-finger selection tracking
@property(nonatomic, assign) BOOL singleTouchValid;         // True if single touch hasn't become multi-touch
@property(nonatomic, assign) CGPoint singleTouchStartPos;   // Position where single touch began
@property(nonatomic, assign) BOOL wasMultiTouch;            // True if we've had multi-touch this gesture

// Pinch gesture tracking
@property(nonatomic, assign) CGFloat lastPinchScale;        // Previous frame's pinch scale
@end

@implementation LumosViewController

- (id)init {
    if ((self = [super init])) {
        [self clearTouches];
    }

    return self;
}

- (id<MTLDevice>)metalDevice {
    if (!_metalDevice) {
        self.metalDevice = [MTLCreateSystemDefaultDevice() autorelease];
    }
    return _metalDevice;
}

- (BOOL)prefersStatusBarHidden {
    return true;//UserInterfaceChromeNavigationAndStatusBar;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    return UIRectEdgeNone;//_Display->uiChrome == UserInterfaceChromeFullscreen ? UIRectEdgeBottom : UIRectEdgeNone;
}

- (UIView<LumosView> *)lumosView {
    return (UIView<LumosView> *)self.view;
}

- (void)loadView
{
	//[super loadView];
	auto *delegate = UIApplication.sharedApplication.delegate;
	CGRect frame = delegate.window.bounds;
	CGFloat scale = [UIScreen mainScreen].nativeScale;

	if (self.metalDevice)
	{
		lumosView = [[[LumosMetalView alloc] initWithFrame:frame
										contentScaleFactor:scale
													device:self.metalDevice
					 ] autorelease];
	}

	self.view = lumosView;

	CAMetalLayer *metalLayer = (CAMetalLayer *)self.view.layer;

	if (![metalLayer isKindOfClass:[CAMetalLayer class]]) {
		NSLog(@"Error: The layer is not a CAMetalLayer.");
		return;
	}

	// Assign view as delegate if needed (for custom drawing logic, not normally required)
	if ([metalLayer respondsToSelector:@selector(setDelegate:)]) {
		[metalLayer performSelector:@selector(setDelegate:) withObject:lumosView];
	}

	CFRetain((__bridge CFTypeRef)metalLayer); // Retain to prevent deallocation

	Lumos::s_Layer = metalLayer;
	Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();
	os->SetLayerPtr((__bridge CAMetalLayer *)metalLayer);  // Ensure correct bridging
	os->SetWindowSize(frame.size.width * scale, frame.size.height * scale);
	os->Init();
}

- (void)viewDidLoad
{
    [super viewDidLoad];

	layer = [self.view layer];		// SRS - When creating a Vulkan Metal surface, need the layer backing the view

    auto *delegate = (LumosAppDelegate*)UIApplication.sharedApplication.delegate;
    self.lumosView.animating = delegate.active;

    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.multipleTouchEnabled = true;
    
#if TARGET_OS_IOS
    self.view.multipleTouchEnabled = self.multipleTouchEnabled;
    [self setNeedsStatusBarAppearanceUpdate];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(keyboardFrameChanged:)
                                               name:UIKeyboardWillChangeFrameNotification
                                             object:self.view.window];

    // Add pointer interaction for iPad editor (iOS 13.4+)
    if (@available(iOS 13.4, *)) {
        if (Lumos::Application::Get().GetAppType() == Lumos::AppType::Editor) {
            UIPointerInteraction *pointerInteraction = [[UIPointerInteraction alloc] initWithDelegate:self];
            [self.view addInteraction:pointerInteraction];
        }
    }

    // Add hover gesture for cursor movement (iOS 13.0+) - only for trackpad/pencil
    if (@available(iOS 13.0, *)) {
        if (Lumos::Application::Get().GetAppType() == Lumos::AppType::Editor) {
            UIHoverGestureRecognizer *hoverGesture = [[UIHoverGestureRecognizer alloc]
                initWithTarget:self action:@selector(handleHover:)];
            // Only allow indirect pointer (trackpad), not direct touches
            if (@available(iOS 13.4, *)) {
                hoverGesture.allowedTouchTypes = @[];  // No touch input
            }
            [self.view addGestureRecognizer:hoverGesture];
        }
    }

    // Add pinch gesture for zoom
    self.pinchGesture = [[UIPinchGestureRecognizer alloc]
        initWithTarget:self action:@selector(handlePinch:)];
    self.pinchGesture.delegate = self;
    self.pinchGesture.delaysTouchesBegan = NO;
    self.pinchGesture.delaysTouchesEnded = NO;
    self.pinchGesture.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:self.pinchGesture];

    // Add pan gesture (2 fingers only)
    self.panGesture = [[UIPanGestureRecognizer alloc]
        initWithTarget:self action:@selector(handlePan:)];
    self.panGesture.delegate = self;
    self.panGesture.minimumNumberOfTouches = 2;
    self.panGesture.maximumNumberOfTouches = 2;
    self.panGesture.delaysTouchesBegan = NO;
    self.panGesture.delaysTouchesEnded = NO;
    self.panGesture.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:self.panGesture];

    // Add swipe gestures (3 fingers) for undo/redo
    NSArray *swipeDirections = @[@(UISwipeGestureRecognizerDirectionLeft),
                                  @(UISwipeGestureRecognizerDirectionRight)];
    for (NSNumber *dirNum in swipeDirections) {
        UISwipeGestureRecognizer *swipe = [[UISwipeGestureRecognizer alloc]
            initWithTarget:self action:@selector(handleSwipe:)];
        swipe.direction = [dirNum integerValue];
        swipe.numberOfTouchesRequired = 3;
        swipe.delaysTouchesBegan = NO;
        [self.view addGestureRecognizer:swipe];
    }

    // Add long-press gesture for context menus
    UILongPressGestureRecognizer *longPress = [[UILongPressGestureRecognizer alloc]
        initWithTarget:self action:@selector(handleLongPress:)];
    longPress.minimumPressDuration = 0.5;
    longPress.delaysTouchesBegan = NO;
    [self.view addGestureRecognizer:longPress];
#endif

	if (![self.view.layer isKindOfClass:[CAMetalLayer class]]) {
		NSLog(@"Error: The layer is not a CAMetalLayer.");
	}
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    Lumos::iOSOS* iosOS = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();
    BOOL isEditor = Lumos::Application::Get().GetAppType() == Lumos::AppType::Editor;

    if (iosOS->GetDeviceType() == Lumos::iOSOS::iOSDeviceType::iPad)
    {
        // iPad editor: allow all orientations, runtime: prefer landscape
        return isEditor ? UIInterfaceOrientationMaskAll : UIInterfaceOrientationMaskLandscape;
    }
    else
    {
        // iPhone editor: landscape for more space, runtime: flexible
        return isEditor ? UIInterfaceOrientationMaskLandscape : UIInterfaceOrientationMaskAllButUpsideDown;
    }
}

- (void)viewWillTransition:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransition:size withTransitionCoordinator:coordinator];
    [coordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        CGSize newSize = self.view.bounds.size;
        CGFloat scale = self.view.contentScaleFactor;
        Lumos::iOSOS::Get()->OnScreenResize(newSize.width * scale, newSize.height * scale);
    }];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    printf("Receive memory warning!\n");
}

- (void)dealloc
{
#if !__has_feature(objc_arc)
    self.metalDevice = nil;
    [super dealloc];
#endif
}

#pragma mark - UIPointerInteractionDelegate

- (UIPointerStyle *)pointerInteraction:(UIPointerInteraction *)interaction
                        styleForRegion:(UIPointerRegion *)region API_AVAILABLE(ios(13.4)) {
    if (Lumos::Application::Get().GetAppType() == Lumos::AppType::Editor) {
        return [UIPointerStyle hiddenPointerStyle]; // Let ImGui draw cursor
    }
    return nil; // Default pointer
}

- (UIPointerRegion *)pointerInteraction:(UIPointerInteraction *)interaction
                      regionForRequest:(UIPointerRegionRequest *)request
                         defaultRegion:(UIPointerRegion *)defaultRegion API_AVAILABLE(ios(13.4)) {
    return defaultRegion;
}

- (void)handleHover:(UIHoverGestureRecognizer *)gesture API_AVAILABLE(ios(13.0)) {
    if (Lumos::Application::Get().GetAppType() != Lumos::AppType::Editor) return;

    CGPoint location = [gesture locationInView:self.view];
    location.x *= self.view.contentScaleFactor;
    location.y *= self.view.contentScaleFactor;

    if (gesture.state == UIGestureRecognizerStateBegan ||
        gesture.state == UIGestureRecognizerStateChanged) {
        Lumos::iOSOS::Get()->OnMouseMovedEvent(location.x, location.y);
    }
}

#pragma mark - Gesture Handlers

- (Lumos::GestureState)gestureStateFromUIKit:(UIGestureRecognizerState)state {
    switch (state) {
        case UIGestureRecognizerStateBegan: return Lumos::GestureState::Began;
        case UIGestureRecognizerStateChanged: return Lumos::GestureState::Changed;
        case UIGestureRecognizerStateEnded: return Lumos::GestureState::Ended;
        case UIGestureRecognizerStateCancelled: return Lumos::GestureState::Cancelled;
        default: return Lumos::GestureState::Ended;
    }
}

- (void)handlePinch:(UIPinchGestureRecognizer *)gesture {
    // Haptic feedback on gesture start and reset tracking
    if (gesture.state == UIGestureRecognizerStateBegan) {
        self.lastPinchScale = 1.0f;
        if (@available(iOS 10.0, *)) {
            UIImpactFeedbackGenerator *haptic = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
            [haptic impactOccurred];
        }
    }

    CGPoint location = [gesture locationInView:self.view];
    location.x *= self.view.contentScaleFactor;
    location.y *= self.view.contentScaleFactor;

    // Calculate per-frame scale delta (relative to last frame, not gesture start)
    CGFloat scaleDelta = gesture.scale / self.lastPinchScale;
    self.lastPinchScale = gesture.scale;

    Lumos::GestureState state = [self gestureStateFromUIKit:gesture.state];
    Lumos::iOSOS::Get()->OnGesturePinch(scaleDelta, gesture.velocity,
                                         location.x, location.y, state);
}

- (void)handlePan:(UIPanGestureRecognizer *)gesture {
    // Haptic feedback on gesture start
    if (gesture.state == UIGestureRecognizerStateBegan) {
        if (@available(iOS 10.0, *)) {
            UIImpactFeedbackGenerator *haptic = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
            [haptic impactOccurred];
        }
    }

    CGPoint translation = [gesture translationInView:self.view];
    CGPoint velocity = [gesture velocityInView:self.view];
    translation.x *= self.view.contentScaleFactor;
    translation.y *= self.view.contentScaleFactor;

    // Reset translation after reading so we get per-frame delta
    [gesture setTranslation:CGPointZero inView:self.view];

    Lumos::GestureState state = [self gestureStateFromUIKit:gesture.state];
    Lumos::iOSOS::Get()->OnGesturePan(translation.x, translation.y,
                                       velocity.x, velocity.y,
                                       (uint32_t)gesture.numberOfTouches, state);
}

- (void)handleSwipe:(UISwipeGestureRecognizer *)gesture {
    // Haptic feedback for swipe (medium impact for undo/redo)
    if (@available(iOS 10.0, *)) {
        UIImpactFeedbackGenerator *haptic = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
        [haptic impactOccurred];
    }

    Lumos::SwipeDirection dir = Lumos::SwipeDirection::Left;
    if (gesture.direction == UISwipeGestureRecognizerDirectionRight)
        dir = Lumos::SwipeDirection::Right;
    else if (gesture.direction == UISwipeGestureRecognizerDirectionUp)
        dir = Lumos::SwipeDirection::Up;
    else if (gesture.direction == UISwipeGestureRecognizerDirectionDown)
        dir = Lumos::SwipeDirection::Down;

    Lumos::iOSOS::Get()->OnGestureSwipe(dir, (uint32_t)gesture.numberOfTouches);
}

- (void)handleLongPress:(UILongPressGestureRecognizer *)gesture {
    // Haptic feedback on long-press recognition
    if (gesture.state == UIGestureRecognizerStateBegan) {
        if (@available(iOS 10.0, *)) {
            UIImpactFeedbackGenerator *haptic = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
            [haptic impactOccurred];
        }
    }

    CGPoint location = [gesture locationInView:self.view];
    location.x *= self.view.contentScaleFactor;
    location.y *= self.view.contentScaleFactor;

    Lumos::GestureState state = [self gestureStateFromUIKit:gesture.state];
    Lumos::iOSOS::Get()->OnGestureLongPress(location.x, location.y, state);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    // Allow pinch + pan simultaneously
    if (([gestureRecognizer isKindOfClass:[UIPinchGestureRecognizer class]] &&
         [otherGestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) ||
        ([gestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]] &&
         [otherGestureRecognizer isKindOfClass:[UIPinchGestureRecognizer class]])) {
        return YES;
    }
    return NO;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch {
    // Always allow single touches through for ImGui (menu bar, popups, etc.)
    // Only pan gesture requires 2 fingers, so single touches should work normally
    if ([gestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) {
        // Pan gesture needs multiple touches
        return YES;
    }
    // Pinch needs multiple touches too
    if ([gestureRecognizer isKindOfClass:[UIPinchGestureRecognizer class]]) {
        return YES;
    }
    // Other gestures can receive all touches
    return YES;
}

#pragma mark - UIResponder

- (void)clearTouches {
    for (int i = 0; i < MAX_SIMULTANEOUS_TOUCHES; i++) {
        activeTouches[i] = NULL;
    }
}

- (int)countActiveTouches {
    int count = 0;
    for (int i = 0; i < MAX_SIMULTANEOUS_TOUCHES; i++) {
        if (activeTouches[i] != NULL) {
            count++;
        }
    }
    return count;
}

- (BOOL)isMultiFingerGestureActive {
    // Check if pinch or pan gestures are currently recognizing
    if (self.pinchGesture &&
        (self.pinchGesture.state == UIGestureRecognizerStateBegan ||
         self.pinchGesture.state == UIGestureRecognizerStateChanged)) {
        return YES;
    }
    if (self.panGesture &&
        (self.panGesture.state == UIGestureRecognizerStateBegan ||
         self.panGesture.state == UIGestureRecognizerStateChanged)) {
        return YES;
    }
    return NO;
}


#pragma mark UIKeyInput methods

//// A key on the keyboard has been pressed.
- (void)insertText:(NSString *)text {
    unichar keycode = (text.length > 0) ? [text characterAtIndex:0] : 0;
    [self handleKeyboardInput:keycode];
}

- (void)deleteBackward {
    [self handleKeyboardInput:0x08]; // ASCII Backspace
}

typedef enum {
    TouchPhaseHover,
    TouchPhaseBegan,
    TouchPhaseMoved,
    TouchPhaseEnded,
    TouchPhaseCancelled,
} TouchPhase;


- (void)addTouchEvent:(UITouch *)touch withType:(TouchPhase)phase {
	int firstNullIndex = -1;
	int index = -1;

		// Try to find an existing touch or the first empty slot
	for (int i = 0; i < MAX_SIMULTANEOUS_TOUCHES; i++) {
		if (activeTouches[i] == (__bridge const void *)touch) {
			index = i; // Touch is already tracked
			break;
		}
		else if (firstNullIndex == -1 && activeTouches[i] == nil) {
			firstNullIndex = i; // First available slot for a new touch
		}
	}

		// If touch is not found, assign it to the first available slot
	if (index == -1) {
		if (firstNullIndex == -1) {
			NSLog(@"Warning: Maximum simultaneous touches exceeded!");
			return;
		}
		index = firstNullIndex;
		activeTouches[index] = (__bridge const void *)touch;
	}

	CGPoint currLocation = [touch locationInView:self.view];
	currLocation.x *= self.view.contentScaleFactor;
	currLocation.y *= self.view.contentScaleFactor;

	switch (phase) {
		case TouchPhaseCancelled:
		case TouchPhaseEnded:
			Lumos::iOSOS::Get()->OnScreenPressed(currLocation.x, currLocation.y, (uint32_t)index, false);
			activeTouches[index] = nil; // Clear the slot for future touches
			break;

		case TouchPhaseBegan:
			Lumos::iOSOS::Get()->OnScreenPressed(currLocation.x, currLocation.y, (uint32_t)index, true);
			break;

		case TouchPhaseMoved:
			Lumos::iOSOS::Get()->OnMouseMovedEvent(currLocation.x, currLocation.y);
			break;
	}
}


- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    NSUInteger totalTouches = event.allTouches.count;

    if(totalTouches == 1 && !self.wasMultiTouch)
    {
        UITouch *touch = [touches anyObject];
        CGPoint location = [touch locationInView:self.view];
        location.x *= self.view.contentScaleFactor;
        location.y *= self.view.contentScaleFactor;

        self.singleTouchValid = YES;
        self.singleTouchStartPos = location;

        // Send mouse move + down for UI interaction
        Lumos::iOSOS::Get()->OnMouseMovedEvent(location.x, location.y);
        Lumos::iOSOS::Get()->OnScreenPressed(location.x, location.y, 0, true);
    }
    else
    {
        // Multi-touch started - release any held mouse button and invalidate
        if(self.singleTouchValid)
        {
            Lumos::iOSOS::Get()->OnScreenPressed(self.singleTouchStartPos.x, self.singleTouchStartPos.y, 0, false);
        }
        self.singleTouchValid = NO;
        self.wasMultiTouch = YES;
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    NSUInteger totalTouches = event.allTouches.count;

    // If multi-touch active, don't send single-finger events
    if(totalTouches > 1 || self.wasMultiTouch || [self isMultiFingerGestureActive])
    {
        if(self.singleTouchValid)
        {
            // Release mouse button when transitioning to multi-touch
            Lumos::iOSOS::Get()->OnScreenPressed(self.singleTouchStartPos.x, self.singleTouchStartPos.y, 0, false);
            self.singleTouchValid = NO;
        }
        self.wasMultiTouch = YES;
        return;
    }

    // Single-finger move - update mouse position
    if(totalTouches == 1 && self.singleTouchValid)
    {
        UITouch *touch = [touches anyObject];
        CGPoint location = [touch locationInView:self.view];
        location.x *= self.view.contentScaleFactor;
        location.y *= self.view.contentScaleFactor;

        Lumos::iOSOS::Get()->OnMouseMovedEvent(location.x, location.y);
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    NSUInteger remainingTouches = 0;
    for(UITouch *touch in event.allTouches)
    {
        if(touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled)
            remainingTouches++;
    }

    // Single-finger ended - send mouse up
    if(self.singleTouchValid && !self.wasMultiTouch && ![self isMultiFingerGestureActive])
    {
        UITouch *touch = [touches anyObject];
        CGPoint location = [touch locationInView:self.view];
        location.x *= self.view.contentScaleFactor;
        location.y *= self.view.contentScaleFactor;

        Lumos::iOSOS::Get()->OnScreenPressed(location.x, location.y, 0, false);
    }

    // Reset when all fingers lifted
    if(remainingTouches == 0)
    {
        self.singleTouchValid = NO;
        self.wasMultiTouch = NO;
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    // Release mouse button and reset state
    if(self.singleTouchValid)
    {
        Lumos::iOSOS::Get()->OnScreenPressed(self.singleTouchStartPos.x, self.singleTouchStartPos.y, 0, false);
    }
    self.singleTouchValid = NO;
    self.wasMultiTouch = NO;
}

#pragma mark - UIKeyInput

- (void)keyboardFrameChanged:(NSNotification *)notification {
    NSObject *value = notification.userInfo[UIKeyboardFrameEndUserInfoKey];
    if ([value isKindOfClass:[NSValue class]]) {
        NSValue *nsValue = (NSValue *)value;
        CGRect keyboardFrame = [nsValue CGRectValue];

        self.keyboardVisible = CGRectIntersectsRect(self.view.frame, keyboardFrame);
    }
}

// UITextInputTraits - disable suggestion bar
- (UITextAutocorrectionType)autocorrectionType {
    return UITextAutocorrectionTypeNo;
}

- (BOOL)hasText {
    return YES;
}

- (BOOL)canBecomeFirstResponder {
    return self.keyboardRequested;
}

- (NSArray<UIKeyCommand *> *)keyCommands {
    static NSArray<UIKeyCommand *> *keyCommands = NULL;
    if (!keyCommands) {
        keyCommands = @[
            // Navigation
            [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow modifierFlags:0 action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow modifierFlags:0 action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow modifierFlags:0 action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow modifierFlags:0 action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:UIKeyInputEscape modifierFlags:0 action:@selector(keyPressed:)],

            // Editor shortcuts (cmd+)
            [UIKeyCommand keyCommandWithInput:@"s" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"z" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"z" modifierFlags:UIKeyModifierCommand|UIKeyModifierShift action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"c" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"v" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"x" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"a" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"d" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"n" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"o" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"w" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"f" modifierFlags:UIKeyModifierCommand action:@selector(keyPressed:)],

            // Tab
            [UIKeyCommand keyCommandWithInput:@"\t" modifierFlags:0 action:@selector(keyPressed:)],
            [UIKeyCommand keyCommandWithInput:@"\t" modifierFlags:UIKeyModifierShift action:@selector(keyPressed:)]
        ];
#if !__has_feature(objc_arc)
        [keyCommands retain];
#endif
    }

    return keyCommands;
}

- (void)keyPressed:(UIKeyCommand *)keyCommand {
    NSString *input = keyCommand.input;
    UIKeyModifierFlags modifiers = keyCommand.modifierFlags;

    char keycode = 0;
    if ([input isEqualToString:UIKeyInputUpArrow]) keycode = 0x7E;
    else if ([input isEqualToString:UIKeyInputDownArrow]) keycode = 0x7D;
    else if ([input isEqualToString:UIKeyInputLeftArrow]) keycode = 0x7B;
    else if ([input isEqualToString:UIKeyInputRightArrow]) keycode = 0x7C;
    else if ([input isEqualToString:UIKeyInputEscape]) keycode = 0x35;
    else if ([input isEqualToString:@"\t"]) keycode = 0x30;
    else if (input.length > 0) keycode = [input characterAtIndex:0];

    BOOL hasCmd = (modifiers & UIKeyModifierCommand) != 0;
    BOOL hasShift = (modifiers & UIKeyModifierShift) != 0;
    BOOL hasAlt = (modifiers & UIKeyModifierAlternate) != 0;
    BOOL hasCtrl = (modifiers & UIKeyModifierControl) != 0;

    Lumos::iOSOS::Get()->OnKeyPressed(keycode, true, hasCmd, hasShift, hasAlt, hasCtrl);
}

// Toggle the display of the virtual keyboard
-(void) toggleKeyboard {
    if (self.isFirstResponder) {
        [self resignFirstResponder];
    } else {
        [self becomeFirstResponder];
    }
}

// Display and hide the keyboard by tapping on the view
-(void) handleTapGesture: (UITapGestureRecognizer*) gestureRecognizer {
    if (gestureRecognizer.state == UIGestureRecognizerStateEnded) {
        [self toggleKeyboard];
    }
}

// Handle keyboard input
-(void) handleKeyboardInput: (unichar) keycode {
    Lumos::iOSOS::Get()->OnKeyPressed(keycode, true);
}

- (BOOL)hasNotch
{
    if(@available(iOS 11.0, *)){
        return [UIApplication sharedApplication].delegate.window.safeAreaInsets.bottom > 0.0f;
    }
    return NO;
}

- (void)KeyboardBridge_ShowKeyboard {
    [self.view becomeFirstResponder];
};

- (void)KeyboardBridge_HideKeyboard {
    [self.view resignFirstResponder];
};

@synthesize hasText;
@end

@implementation LumosAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    _active = YES;

    Lumos::iOSOS::Create();
    Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();

    self.window = [[UIWindow alloc] init];

    if (self.window.bounds.size.width <= 0.0 || self.window.bounds.size.height <= 0.0)
    {
        // Set UIWindow frame for iOS 8.
        // On iOS 9, the UIWindow frame may be different than the UIScreen bounds for iPad's
        // Split View or Slide Over.
        self.window.frame = [[UIScreen mainScreen] bounds];
    }

    UIEdgeInsets safeAreaInsets = self.window.safeAreaInsets;
    CGRect safeAreaFrame = CGRectMake(safeAreaInsets.left,
                                      safeAreaInsets.top,
                                      self.window.frame.size.width - safeAreaInsets.left - safeAreaInsets.right,
                                      self.window.frame.size.height - safeAreaInsets.top - safeAreaInsets.bottom);
    self.window.frame = safeAreaFrame;

    self.window.rootViewController = [[LumosViewController alloc] init];
    [self.window makeKeyAndVisible];

    return YES;
}

- (void)setActive:(BOOL)active {
    if (_active != active) {
        _active = active;

        LumosViewController *vc = (LumosViewController *)[self.window rootViewController];
        //if (vc.Display && vc.Display->focusFunc) {
            //vc.Display->focusFunc(vc.Display, _active);
        //}
        if (vc.isViewLoaded) {
            if (!active) {
                // Draw once when entering the background so that a game can show "paused" state.
                [vc.lumosView draw];
            }
            vc.lumosView.animating = active;
        }
        [vc clearTouches];
    }
}

CGRect ComputeSafeArea(UIView* view)
{
    CGSize screenSize = view.bounds.size;
    CGRect screenRect = CGRectMake(0, 0, screenSize.width, screenSize.height);

    UIEdgeInsets insets = UIEdgeInsetsMake(0, 0, 0, 0);

    if (@available(iOS 11.0, tvOS 11.0, *))
        insets = [view safeAreaInsets];

    screenRect.origin.x += insets.left;
    screenRect.origin.y += insets.bottom;
    screenRect.size.width -= insets.left + insets.right;
    screenRect.size.height -= insets.top + insets.bottom;

    return screenRect;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    self.active = NO;
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    Lumos::Application::Get().GetWindow()->SetWindowFocus(false);
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    self.active = NO;
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    Lumos::Application::Get().GetWindow()->SetWindowFocus(false);
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    self.active = YES;
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
    Lumos::Application::Get().GetWindow()->SetWindowFocus(true);
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    self.active = YES;
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
	if(Lumos::Application::Get().GetWindow())
	Lumos::Application::Get().GetWindow()->SetWindowFocus(true);
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    self.active = NO;
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    ((Lumos::iOSOS*)Lumos::iOSOS::Get())->OnQuit();
    Lumos::iOSOS::Release();
}


- (void)dealloc
{
#if !__has_feature(objc_arc)
    self.window = nil;
    [super dealloc];
#endif
}

@end

#pragma mark - Main

int main(int argc, char * argv[])
{
	@autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([LumosAppDelegate class]));
    }
}
