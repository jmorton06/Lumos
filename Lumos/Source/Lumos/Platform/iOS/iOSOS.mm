#ifdef LUMOS_PLATFORM_IOS
#include "iOSOS.h"
#include "iOSWindow.h"
#include "iOSKeyCodes.h"
#include "Core/OS/FileSystem.h"
#include "Core/OS/Input.h"
#include "Core/CoreSystem.h"
#include "Core/Application.h"
#include "Audio/NowPlaying.h"
#include "Audio/AudioSession.h"
#include "Platform/OpenAL/ALMusicStream.h"

#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

#include <sys/sysctl.h>
#include <SystemConfiguration/SCNetworkReachability.h>
#include <netinet/in.h>

#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioServices.h>
#import <AVFoundation/AVFoundation.h>

#define MAX_SIMULTANEOUS_TOUCHES 10

static void iOSShowKeyboard(bool show);

// 0 = the system keyboard as it comes, 1 = for code (see UIKeyboardStyle)
static int g_KeyboardStyle = 0;
// share of the view's height under the keyboard
static float g_KeyboardCover = 0.0f;

extern "C" float iOSKeyboardCover() { return g_KeyboardCover; }

// Handwriting: the UI's text fields (Graphics/UI.cpp) and the field the keyboard types into (iOSKeyboard.mm)
extern "C" uint32_t UIScribbleTargets(uint64_t* hashes, float* rects, uint32_t maxCount);
extern "C" bool UIScribbleIsFocused(uint64_t hash);
extern "C" void UIScribbleFocus(uint64_t hash);
UITextField *iOSKeyboardActiveField(void);

namespace Lumos
{
	static CAMetalLayer* s_Layer = nullptr;
	CAMetalLayer* iOSOS::GetStaticLayer() { return s_Layer; }

    // UIKit can deliver touches / keys after the app has shut down
    static iOSWindow* LiveWindow()
    {
        return Application::HasInstance() ? (iOSWindow*)Application::Get().GetWindow() : nullptr;
    }


    // scripts' background tick, run on demand while not in the foreground (frames handle it otherwise)
    static void RunBackgroundTick()
    {
        if(iOSOS::IsQuitting() || !Application::HasInstance())
            return;
        if([UIApplication sharedApplication].applicationState == UIApplicationStateActive)
            return;
        Application::Get().OnBackgroundUpdate(0.0f);
    }

    iOSOS::iOSOS()
    {
    }

    iOSOS::~iOSOS()
    {
    }

    void iOSOS::Init()
    {
        // launch arguments (devicectl / Xcode scheme) reach the command line like on desktop
        static char* s_Argv[64];
        int argc = 0;
        for(NSString* a in [[NSProcessInfo processInfo] arguments])
        {
            if(argc >= 64) break;
            s_Argv[argc++] = strdup(a.UTF8String);
        }
        Lumos::Internal::CoreSystem::Init(argc, s_Argv);
        LINFO("Device : %s",GetModelName().c_str());

        iOSWindow::MakeDefault();

        NSError* audioSessionError = nil;
        AVAudioSession* audioSession = [AVAudioSession sharedInstance];
        [audioSession setCategory:AVAudioSessionCategoryPlayback error:&audioSessionError];
        if (audioSessionError)
            LERROR("Failed to set AVAudioSession category: %s", [[audioSessionError localizedDescription] UTF8String]);

        audioSessionError = nil;
        [audioSession setActive:YES error:&audioSessionError];
        if (audioSessionError)
            LERROR("Failed to activate AVAudioSession: %s", [[audioSessionError localizedDescription] UTF8String]);

        // a suspended app gets no frames or timer ticks: handle the command now so what plays
        // and what Control Center shows stay in step
        Lumos::NowPlaying::SetCommandHook(&RunBackgroundTick);

        // a call / Siri / another app's audio: Apple's OpenAL needs its context dropped and
        // restored around it or it stays silent; the app pauses so its state matches
        [[NSNotificationCenter defaultCenter] addObserverForName:AVAudioSessionInterruptionNotification
                                                           object:audioSession
                                                            queue:[NSOperationQueue mainQueue]
                                                       usingBlock:^(NSNotification* note) {
            NSUInteger type = [note.userInfo[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
            if (type == AVAudioSessionInterruptionTypeBegan)
            {
                Lumos::AudioSession::SetOpenALHold(Lumos::AudioSession::Hold_Interrupted, true);
                Lumos::NowPlaying::Inject(Lumos::NowPlaying::CommandType::Pause);
                RunBackgroundTick();
            }
            else if (type == AVAudioSessionInterruptionTypeEnded)
            {
                NSError* resumeError = nil;
                [[AVAudioSession sharedInstance] setActive:YES error:&resumeError];
                Lumos::AudioSession::SetOpenALHold(Lumos::AudioSession::Hold_Interrupted, false);
                NSUInteger opts = [note.userInfo[AVAudioSessionInterruptionOptionKey] unsignedIntegerValue];
                if (opts & AVAudioSessionInterruptionOptionShouldResume)
                {
                    Lumos::NowPlaying::Inject(Lumos::NowPlaying::CommandType::Play);
                    RunBackgroundTick();
                }
            }
        }];

        auto app = Lumos::CreateApplication();
        app->Init();
    }

    bool iOSOS::OnFrame()
    {
        // UIKit may still ask for a frame (e.g. resigning active) after OnQuit released the app
        if(!Application::HasInstance())
            return false;
        // blocking pickers spin the run loop mid-frame and the display link fires again: skip those
        static bool s_InFrame = false;
        if(s_InFrame)
            return true;
        s_InFrame = true;
        bool ok   = Application::Get().OnFrame();
        s_InFrame = false;
        return ok;
    }

    static bool s_Quitting = false;
    bool iOSOS::IsQuitting() { return s_Quitting; }

    void iOSOS::OnQuit()
    {
        if(!Application::HasInstance())
            return; // already quit
        s_Quitting = true;
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

    bool iOSOS::CopyBundleFolder(const std::string& bundleSrc, const std::string& dest)
    {
        @autoreleasepool {
            NSFileManager* fm = [NSFileManager defaultManager];
            NSString* srcPath = [NSString stringWithUTF8String:bundleSrc.c_str()];
            NSString* dstPath = [NSString stringWithUTF8String:dest.c_str()];

            // Remove trailing slashes for NSFileManager
            while([srcPath hasSuffix:@"/"] && srcPath.length > 1)
                srcPath = [srcPath substringToIndex:srcPath.length - 1];
            while([dstPath hasSuffix:@"/"] && dstPath.length > 1)
                dstPath = [dstPath substringToIndex:dstPath.length - 1];

            if(![fm fileExistsAtPath:srcPath])
            {
                LWARN("CopyBundleFolder: source not found: %s", bundleSrc.c_str());
                return false;
            }

            // Create parent directory
            NSString* parentDir = [dstPath stringByDeletingLastPathComponent];
            NSError* error = nil;
            [fm createDirectoryAtPath:parentDir withIntermediateDirectories:YES attributes:nil error:&error];

            // Remove existing destination so copyItemAtPath succeeds
            if([fm fileExistsAtPath:dstPath])
                [fm removeItemAtPath:dstPath error:nil];

            if(![fm copyItemAtPath:srcPath toPath:dstPath error:&error])
            {
                LWARN("CopyBundleFolder failed: %s", [[error localizedDescription] UTF8String]);
                return false;
            }

            LINFO("Copied bundle folder %s -> %s", bundleSrc.c_str(), dest.c_str());
            return true;
        }
    }

    void iOSOS::OnKeyPressed(char keycode, bool down, bool cmd, bool shift, bool alt, bool ctrl)
    {
        Lumos::InputCode::Key lumosKey = Lumos::iOSKeyCodes::iOSKeyToLumos(keycode);

        iOSWindow* window = LiveWindow();
        if(!window) return;
        if (cmd) window->OnKeyEvent(Lumos::InputCode::Key::LeftSuper, down);
        if (shift) window->OnKeyEvent(Lumos::InputCode::Key::LeftShift, down);
        if (alt) window->OnKeyEvent(Lumos::InputCode::Key::LeftAlt, down);
        if (ctrl) window->OnKeyEvent(Lumos::InputCode::Key::LeftControl, down);

        window->OnKeyEvent(lumosKey, down);
    }

    void iOSOS::OnKeyTyped(char character)
    {
        iOSWindow* window = LiveWindow();
        if(!window) return;
        window->OnKeyTypedEvent(character);
    }

    void iOSOS::OnMouseButtonEvent(int button, bool down)
    {
        iOSWindow* window = LiveWindow();
        if(!window) return;
        window->OnTouchEvent(0, 0, button, down);
    }

    void iOSOS::OnScrollWheelEvent(float xOffset, float yOffset)
    {
        iOSWindow* window = LiveWindow();
        if(!window) return;
        window->OnScrollEvent(xOffset, yOffset);
    }

    void iOSOS::OnScreenPressed(uint32_t x, uint32_t y, uint32_t count, bool down)
    {
        if(iOSWindow* w = LiveWindow()) w->OnTouchEvent(x,y,count, down);
    }

    void iOSOS::OnMouseMovedEvent(uint32_t xPos, uint32_t yPos)
    {
        if(iOSWindow* w = LiveWindow()) w->OnMouseMovedEvent(xPos,yPos);
    }

    void iOSOS::OnScreenResize(uint32_t width, uint32_t height)
    {
        LINFO("Resizing Window %u, %u", width, height);
        if(iOSWindow* w = LiveWindow()) w->OnResizeEvent(width, height);
    }

    void iOSOS::OnGesturePinch(float scale, float velocity, uint32_t x, uint32_t y, GestureState state)
    {
        if(iOSWindow* w = LiveWindow()) w->OnGesturePinchEvent(scale, velocity, x, y, state);
    }

    void iOSOS::OnGesturePan(float tx, float ty, float vx, float vy, uint32_t touches, GestureState state)
    {
        if(iOSWindow* w = LiveWindow()) w->OnGesturePanEvent(tx, ty, vx, vy, touches, state);
    }

    void iOSOS::OnGestureSwipe(SwipeDirection direction, uint32_t touches)
    {
        if(iOSWindow* w = LiveWindow()) w->OnGestureSwipeEvent(direction, touches);
    }

    void iOSOS::OnGestureLongPress(uint32_t x, uint32_t y, GestureState state)
    {
        if(iOSWindow* w = LiveWindow()) w->OnGestureLongPressEvent(x, y, state);
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

    void iOSOS::Haptic(int kind) const
    {
        dispatch_block_t fire = ^{
            // kept alive and prepared, or the first tick of a run lags
            static UISelectionFeedbackGenerator* selection = nil;
            static UIImpactFeedbackGenerator* impact[3]    = { nil, nil, nil };
            if(kind <= 0)
            {
                if(!selection)
                    selection = [[UISelectionFeedbackGenerator alloc] init];
                [selection selectionChanged];
                [selection prepare];
            }
            else
            {
                const int i = kind > 3 ? 2 : kind - 1;
                if(!impact[i])
                {
                    const UIImpactFeedbackStyle styles[3] = { UIImpactFeedbackStyleLight, UIImpactFeedbackStyleMedium, UIImpactFeedbackStyleHeavy };
                    impact[i] = [[UIImpactFeedbackGenerator alloc] initWithStyle:styles[i]];
                }
                [impact[i] impactOccurred];
                [impact[i] prepare];
            }
        };
        if([NSThread isMainThread])
            fire();
        else
            dispatch_async(dispatch_get_main_queue(), fire);
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
            iOSShowKeyboard(bShow);
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
    UIViewController* vc = self.window.rootViewController;
    SEL check = NSSelectorFromString(@"lumosCheckOrientation");
    if (vc && [vc respondsToSelector:check]) ((void (*)(id, SEL))[vc methodForSelector:check])(vc, check);
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
       view.paused = YES;
       os->OnQuit();
       return;
   }

   if(self.resizeQueue.count > 0)
   {
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
@property(nonatomic, strong) NSTimer *backgroundTimer;
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
@property(nonatomic, strong) UIView *emptyInputView;
@property(nonatomic, strong) UIView *codeBar;
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
    return UIRectEdgeAll;
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
    self.keyboardRequested = NO;
    self.emptyInputView = [[UIView alloc] initWithFrame:CGRectZero];

#if TARGET_OS_IOS
    self.view.multipleTouchEnabled = self.multipleTouchEnabled;
    [self setNeedsStatusBarAppearanceUpdate];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(keyboardFrameChanged:)
                                               name:UIKeyboardWillChangeFrameNotification
                                             object:self.view.window];

    // Pencil handwriting into the UI's own text fields (iOS 14+, iPad)
    if (@available(iOS 14.0, *)) {
        UIIndirectScribbleInteraction *scribble = [[UIIndirectScribbleInteraction alloc]
            initWithDelegate:(id<UIIndirectScribbleInteractionDelegate>)self];
        [self.view addInteraction:scribble];
    }

    // Add pointer interaction for trackpad/mouse (iOS 13.4+)
    if (@available(iOS 13.4, *)) {
        UIPointerInteraction *pointerInteraction = [[UIPointerInteraction alloc] initWithDelegate:self];
        [self.view addInteraction:pointerInteraction];
    }

    // Add hover gesture for cursor movement (iOS 13.0+) - trackpad/mouse
    if (@available(iOS 13.0, *)) {
        UIHoverGestureRecognizer *hoverGesture = [[UIHoverGestureRecognizer alloc]
            initWithTarget:self action:@selector(handleHover:)];
        if (@available(iOS 13.4, *)) {
            hoverGesture.allowedTouchTypes = @[];
        }
        [self.view addGestureRecognizer:hoverGesture];
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
    longPress.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:longPress];

    UIScreenEdgePanGestureRecognizer *topEdgePan = [[UIScreenEdgePanGestureRecognizer alloc]
        initWithTarget:self action:@selector(handleTopEdgePan:)];
    topEdgePan.edges = UIRectEdgeTop;
    topEdgePan.delaysTouchesBegan = NO;
    topEdgePan.delaysTouchesEnded = NO;
    topEdgePan.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:topEdgePan];

    // Become first responder to receive external keyboard input
    [self becomeFirstResponder];
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
        return isEditor ? UIInterfaceOrientationMaskAll : UIInterfaceOrientationMaskAllButUpsideDown;
    }
    else
    {
        return UIInterfaceOrientationMaskAllButUpsideDown;
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

// Landscape phones report the notch inset on BOTH sides; only the notch side needs it, the other just
// keeps clear of the rounded corner. Which side depends on which way the phone is turned.
static UIInterfaceOrientation s_SafeAreaOrientation = UIInterfaceOrientationUnknown;
- (void)lumosUpdateSafeArea {
    UIEdgeInsets insets = self.view.safeAreaInsets;
    UIInterfaceOrientation o = UIInterfaceOrientationUnknown;
    if (@available(iOS 13.0, *))
        o = self.view.window.windowScene.interfaceOrientation;
    s_SafeAreaOrientation = o;
    const CGFloat corner = 16.0; // points kept from the rounded corner on the side without the notch
    if (insets.left > 0 && fabs(insets.left - insets.right) < 1.0) {
        // landscape right = home side on the right, the notch on the left
        if (o == UIInterfaceOrientationLandscapeRight) insets.right = MIN(insets.right, corner);
        else if (o == UIInterfaceOrientationLandscapeLeft) insets.left = MIN(insets.left, corner);
    }
    CGFloat scale = self.view.contentScaleFactor;
    Lumos::iOSOS::Get()->SetSafeAreaScale(scale);
    Lumos::iOSOS::Get()->SetSafeAreaInsets(
        insets.top * scale, insets.bottom * scale,
        insets.left * scale, insets.right * scale);
}

// per frame: a 180 degree turn between the two landscapes doesn't lay the view out again
- (void)lumosCheckOrientation {
    if (@available(iOS 13.0, *)) {
        if (self.view.window.windowScene.interfaceOrientation != s_SafeAreaOrientation) [self lumosUpdateSafeArea];
    }
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    [self lumosUpdateSafeArea];
    if (@available(iOS 11.0, *)) {
        [self setNeedsUpdateOfHomeIndicatorAutoHidden];
        [self setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
    }
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
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
    return nil; // Default system pointer
}

- (UIPointerRegion *)pointerInteraction:(UIPointerInteraction *)interaction
                      regionForRequest:(UIPointerRegionRequest *)request
                         defaultRegion:(UIPointerRegion *)defaultRegion API_AVAILABLE(ios(13.4)) {
    return defaultRegion;
}

- (void)handleHover:(UIHoverGestureRecognizer *)gesture API_AVAILABLE(ios(13.0)) {
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

- (void)handleTopEdgePan:(UIScreenEdgePanGestureRecognizer *)gesture {
    // Fire once on Began — the editor toggles the menu bar visible.
    if(gesture.state == UIGestureRecognizerStateBegan)
    {
        Lumos::iOSOS::Get()->OnGestureSwipe(Lumos::SwipeDirection::Down, 1);
    }
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

- (void)insertText:(NSString *)text {
    if (text.length == 0) return;

    if (!Lumos::Application::HasInstance()) return;
    Lumos::iOSWindow* window = (Lumos::iOSWindow*)Lumos::Application::Get().GetWindow();
    for (NSUInteger i = 0; i < text.length; i++) {
        unichar ch = [text characterAtIndex:i];
        if (ch == '\n' || ch == '\r') {
            window->OnKeyEvent(Lumos::InputCode::Key::Enter, true);
            window->OnKeyEvent(Lumos::InputCode::Key::Enter, false);
        } else if (ch < 0x80) {
            Lumos::iOSOS::Get()->OnKeyTyped((char)ch);
        }
    }
}

- (void)deleteBackward {
    if (!Lumos::Application::HasInstance()) return;
    Lumos::iOSWindow* window = (Lumos::iOSWindow*)Lumos::Application::Get().GetWindow();
    window->OnKeyEvent(Lumos::InputCode::Key::Backspace, true);
    window->OnKeyEvent(Lumos::InputCode::Key::Backspace, false);
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



// Publishes every finger still down (native px) for on-screen controls.
static void PublishTouchPoints(UIEvent* event, UIView* view)
{
    Lumos::Input::TouchPoint points[Lumos::Input::MaxTouchPoints];
    uint32_t n = 0;
    for(UITouch* touch in event.allTouches)
    {
        if(n >= Lumos::Input::MaxTouchPoints)
            break;
        if(touch.phase == UITouchPhaseEnded || touch.phase == UITouchPhaseCancelled)
            continue;
        CGPoint p = [touch locationInView:view];
        points[n].Id       = (uint64_t)(uintptr_t)(__bridge void*)touch;
        points[n].Position = Lumos::Vec2(p.x * view.contentScaleFactor, p.y * view.contentScaleFactor);
        n++;
    }
    Lumos::Input::Get().SetTouchPoints(points, n);
}

static uint32_t ActiveTouchCount(UIEvent* event)
{
    uint32_t count = 0;
    for(UITouch* touch in event.allTouches)
    {
        if(touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled)
            count++;
    }
    return count;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    if (@available(iOS 13.4, *)) {
        if (event.type == UIEventTypeHover && (event.buttonMask & UIEventButtonMaskSecondary)) {
            UITouch *touch = [touches anyObject];
            if (touch) {
                CGPoint location = [touch locationInView:self.view];
                location.x *= self.view.contentScaleFactor;
                location.y *= self.view.contentScaleFactor;
                Lumos::iOSOS::Get()->OnMouseMovedEvent(location.x, location.y);
                Lumos::iOSOS::Get()->OnScreenPressed(location.x, location.y, 1, true);
            }
            return;
        }
    }

    NSUInteger totalTouches = event.allTouches.count;
    Lumos::Input::Get().SetTouchCount(ActiveTouchCount(event));
    PublishTouchPoints(event, self.view);

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
    Lumos::Input::Get().SetTouchCount(ActiveTouchCount(event));
    PublishTouchPoints(event, self.view);

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
    if (@available(iOS 13.4, *)) {
        if (event.type == UIEventTypeHover && (event.buttonMask & UIEventButtonMaskSecondary)) {
            UITouch *touch = [touches anyObject];
            if (touch) {
                CGPoint location = [touch locationInView:self.view];
                location.x *= self.view.contentScaleFactor;
                location.y *= self.view.contentScaleFactor;
                Lumos::iOSOS::Get()->OnScreenPressed(location.x, location.y, 1, false);
            }
            return;
        }
    }

    NSUInteger remainingTouches = ActiveTouchCount(event);
    Lumos::Input::Get().SetTouchCount((uint32_t)remainingTouches);
    PublishTouchPoints(event, self.view);

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
    Lumos::Input::Get().SetTouchCount(ActiveTouchCount(event));
    PublishTouchPoints(event, self.view);
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

        // How much of the view it covers (any keyboard: ours or a text field's). Only a keyboard
        // docked along the bottom pushes the layout up: on an iPad it can float, be split, or be
        // just the shortcut bar of a hardware keyboard - those sit over the view and move about.
        CGRect inView  = [self.view convertRect:keyboardFrame fromView:nil];
        CGRect bounds  = self.view.bounds;
        CGFloat height = bounds.size.height;
        CGRect onView  = CGRectIntersection(inView, bounds);
        const BOOL docked = !CGRectIsNull(onView) && CGRectGetMaxY(onView) >= CGRectGetMaxY(bounds) - 1.0
            && onView.size.width >= bounds.size.width * 0.9;
        CGFloat cover  = (docked && height > 0.0) ? onView.size.height / height : 0.0;
        g_KeyboardCover = (float)fmax(0.0, fmin(0.7, cover));
    }
}

// UITextInputTraits: nothing that rewrites what is typed
- (UITextAutocapitalizationType)autocapitalizationType {
    return UITextAutocapitalizationTypeNone;
}

- (UITextSpellCheckingType)spellCheckingType {
    return UITextSpellCheckingTypeNo;
}

- (UITextSmartQuotesType)smartQuotesType {
    return UITextSmartQuotesTypeNo;
}

- (UITextSmartDashesType)smartDashesType {
    return UITextSmartDashesTypeNo;
}

- (UITextSmartInsertDeleteType)smartInsertDeleteType {
    return UITextSmartInsertDeleteTypeNo;
}

- (UIKeyboardType)keyboardType {
    return g_KeyboardStyle == 1 ? UIKeyboardTypeASCIICapable : UIKeyboardTypeDefault;
}

- (UIKeyboardAppearance)keyboardAppearance {
    return UIKeyboardAppearanceDark;
}

#pragma mark - Scribble (UIIndirectScribbleInteractionDelegate)

// The fields are drawn by the engine, so iOS is told where they are; each is known by its widget hash.
- (BOOL)scribbleRect:(CGRect *)outRect forHash:(uint64_t)hash {
    uint64_t hashes[128];
    float rects[128 * 4];
    const uint32_t count = Lumos::Application::HasInstance() ? UIScribbleTargets(hashes, rects, 128) : 0;
    const CGFloat scale  = self.view.contentScaleFactor > 0.0 ? self.view.contentScaleFactor : 1.0;
    for (uint32_t i = 0; i < count; i++) {
        if (hashes[i] == hash) {
            *outRect = CGRectMake(rects[i * 4] / scale, rects[i * 4 + 1] / scale, rects[i * 4 + 2] / scale, rects[i * 4 + 3] / scale);
            return YES;
        }
    }
    return NO;
}

- (void)indirectScribbleInteraction:(id)interaction
              requestElementsInRect:(CGRect)rect
                         completion:(void (^)(NSArray *elements))completion {
    NSMutableArray *elements = [NSMutableArray array];
    uint64_t hashes[128];
    float rects[128 * 4];
    const uint32_t count = Lumos::Application::HasInstance() ? UIScribbleTargets(hashes, rects, 128) : 0;
    const CGFloat scale  = self.view.contentScaleFactor > 0.0 ? self.view.contentScaleFactor : 1.0;
    for (uint32_t i = 0; i < count; i++) {
        CGRect frame = CGRectMake(rects[i * 4] / scale, rects[i * 4 + 1] / scale, rects[i * 4 + 2] / scale, rects[i * 4 + 3] / scale);
        if (CGRectIntersectsRect(frame, rect))
            [elements addObject:@(hashes[i])];
    }
    completion(elements);
}

- (BOOL)indirectScribbleInteraction:(id)interaction isElementFocused:(id)elementIdentifier {
    return UIScribbleIsFocused([(NSNumber *)elementIdentifier unsignedLongLongValue]) && iOSKeyboardActiveField() != nil;
}

- (CGRect)indirectScribbleInteraction:(id)interaction frameForElement:(id)elementIdentifier {
    CGRect frame = CGRectZero;
    [self scribbleRect:&frame forHash:[(NSNumber *)elementIdentifier unsignedLongLongValue]];
    return frame;
}

// The engine focuses its field on the next frame and that opens the (hidden) system text field:
// hand it over once it is there.
- (void)scribbleDeliverField:(void (^)(UIResponder<UITextInput> *focusedInput))completion tries:(int)tries {
    UITextField *field = iOSKeyboardActiveField();
    if (field != nil || tries <= 0) {
        completion(field);
        return;
    }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.05 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self scribbleDeliverField:completion tries:tries - 1];
    });
}

- (void)indirectScribbleInteraction:(id)interaction
               focusElementIfNeeded:(id)elementIdentifier
                     referencePoint:(CGPoint)focusReferencePoint
                         completion:(void (^)(UIResponder<UITextInput> *focusedInput))completion {
    UIScribbleFocus([(NSNumber *)elementIdentifier unsignedLongLongValue]);
    [self scribbleDeliverField:completion tries:30];
}

#pragma mark - Code key bar

// What sits above the keyboard in the code style: the keys a phone keyboard buries.
// Strings are typed as they are; the rest are named below.
static NSArray<NSString *> *LumosCodeKeys(void) {
    static NSArray<NSString *> *keys = nil;
    if (!keys) {
        keys = @[ @"hide", @"tab", @"left", @"right", @"up", @"down", @"undo", @"redo",
                  @"(", @")", @"{", @"}", @"[", @"]", @"=", @"\"", @"'", @".", @",", @":", @";",
                  @"_", @"-", @"+", @"*", @"/", @"<", @">", @"#", @"%", @"~", @"!", @"&", @"|" ];
    }
    return keys;
}

- (UIView *)codeKeyBar {
    if (self.codeBar)
        return self.codeBar;

    const CGFloat height = 46.0;
    UIInputView *bar = [[UIInputView alloc] initWithFrame:CGRectMake(0, 0, self.view.bounds.size.width, height)
                                           inputViewStyle:UIInputViewStyleKeyboard];
    bar.autoresizingMask = UIViewAutoresizingFlexibleWidth;

    UIScrollView *scroll = [[UIScrollView alloc] initWithFrame:bar.bounds];
    scroll.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    scroll.showsHorizontalScrollIndicator = NO;
    scroll.alwaysBounceHorizontal = YES;
    [bar addSubview:scroll];

    NSDictionary<NSString *, NSString *> *symbols = @{
        @"hide" : @"keyboard.chevron.compact.down", @"tab" : @"arrow.right.to.line",
        @"left" : @"arrow.left", @"right" : @"arrow.right", @"up" : @"arrow.up", @"down" : @"arrow.down",
        @"undo" : @"arrow.uturn.backward", @"redo" : @"arrow.uturn.forward"
    };

    NSArray<NSString *> *keys = LumosCodeKeys();
    CGFloat x = 6.0;
    for (NSUInteger i = 0; i < keys.count; i++) {
        NSString *key = keys[i];
        UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
        button.tag = (NSInteger)i;
        button.tintColor = UIColor.whiteColor;
        button.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.14];
        button.layer.cornerRadius = 6.0;
        NSString *symbol = symbols[key];
        UIImage *image = nil;
        if (symbol) {
            if (@available(iOS 13.0, *))
                image = [UIImage systemImageNamed:symbol];
        }
        if (image) {
            [button setImage:image forState:UIControlStateNormal];
        } else {
            [button setTitle:key forState:UIControlStateNormal];
            if (@available(iOS 13.0, *))
                button.titleLabel.font = [UIFont monospacedSystemFontOfSize:19.0 weight:UIFontWeightMedium];
            else
                button.titleLabel.font = [UIFont systemFontOfSize:19.0 weight:UIFontWeightMedium];
        }
        const CGFloat width = symbol ? 44.0 : 36.0;
        button.frame = CGRectMake(x, 5.0, width, height - 10.0);
        [button addTarget:self action:@selector(codeKeyPressed:) forControlEvents:UIControlEventTouchUpInside];
        [scroll addSubview:button];
        x += width + 6.0;
    }
    scroll.contentSize = CGSizeMake(x, height);

    self.codeBar = bar;
    return bar;
}

- (void)codeKeyPressed:(UIButton *)sender {
    NSArray<NSString *> *keys = LumosCodeKeys();
    if (sender.tag < 0 || (NSUInteger)sender.tag >= keys.count)
        return;
    NSString *key = keys[(NSUInteger)sender.tag];
    [[UIDevice currentDevice] playInputClick];

    if (!Lumos::Application::HasInstance()) return;
    Lumos::iOSWindow* window = (Lumos::iOSWindow*)Lumos::Application::Get().GetWindow();
    auto tap = [window](Lumos::InputCode::Key code) {
        window->OnKeyEvent(code, true);
        window->OnKeyEvent(code, false);
    };

    if ([key isEqualToString:@"hide"]) iOSShowKeyboard(false);
    else if ([key isEqualToString:@"tab"]) tap(Lumos::InputCode::Key::Tab);
    else if ([key isEqualToString:@"left"]) tap(Lumos::InputCode::Key::Left);
    else if ([key isEqualToString:@"right"]) tap(Lumos::InputCode::Key::Right);
    else if ([key isEqualToString:@"up"]) tap(Lumos::InputCode::Key::Up);
    else if ([key isEqualToString:@"down"]) tap(Lumos::InputCode::Key::Down);
    else if ([key isEqualToString:@"undo"] || [key isEqualToString:@"redo"]) {
        // Cmd Z / Cmd Shift Z, as a keyboard would send them
        const BOOL redo = [key isEqualToString:@"redo"];
        window->OnKeyEvent(Lumos::InputCode::Key::LeftSuper, true);
        if (redo) window->OnKeyEvent(Lumos::InputCode::Key::LeftShift, true);
        tap(Lumos::InputCode::Key::Z);
        if (redo) window->OnKeyEvent(Lumos::InputCode::Key::LeftShift, false);
        window->OnKeyEvent(Lumos::InputCode::Key::LeftSuper, false);
    }
    else [self insertText:key];
}

// lets the bar's buttons click like keys
- (BOOL)enableInputClicksWhenVisible {
    return YES;
}

- (UIView *)inputAccessoryView {
    return (self.keyboardRequested && g_KeyboardStyle == 1) ? [self codeKeyBar] : nil;
}

// UITextInputTraits - disable suggestion bar
- (UITextAutocorrectionType)autocorrectionType {
    return UITextAutocorrectionTypeNo;
}

- (BOOL)hasText {
    return YES;
}

- (BOOL)canBecomeFirstResponder {
    return YES;
}

- (UIView *)inputView {
    return self.keyboardRequested ? nil : self.emptyInputView;
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
    BOOL isSpecialKey = NO;
    if ([input isEqualToString:UIKeyInputUpArrow]) { keycode = 0x7E; isSpecialKey = YES; }
    else if ([input isEqualToString:UIKeyInputDownArrow]) { keycode = 0x7D; isSpecialKey = YES; }
    else if ([input isEqualToString:UIKeyInputLeftArrow]) { keycode = 0x7B; isSpecialKey = YES; }
    else if ([input isEqualToString:UIKeyInputRightArrow]) { keycode = 0x7C; isSpecialKey = YES; }
    else if ([input isEqualToString:UIKeyInputEscape]) { keycode = 0x35; isSpecialKey = YES; }
    else if ([input isEqualToString:@"\t"]) { keycode = 0x30; isSpecialKey = YES; }
    else if (input.length > 0) keycode = [input characterAtIndex:0];

    BOOL hasCmd = (modifiers & UIKeyModifierCommand) != 0;
    BOOL hasShift = (modifiers & UIKeyModifierShift) != 0;
    BOOL hasAlt = (modifiers & UIKeyModifierAlternate) != 0;
    BOOL hasCtrl = (modifiers & UIKeyModifierControl) != 0;

    // Key down
    Lumos::iOSOS::Get()->OnKeyPressed(keycode, true, hasCmd, hasShift, hasAlt, hasCtrl);

    // Send typed event for printable chars without cmd modifier
    if (!isSpecialKey && !hasCmd && input.length > 0) {
        char ch = [input characterAtIndex:0];
        Lumos::iOSOS::Get()->OnKeyTyped(ch);
    }

    // Key up so keys don't stick
    Lumos::iOSOS::Get()->OnKeyPressed(keycode, false, hasCmd, hasShift, hasAlt, hasCtrl);
}

// Toggle the display of the virtual keyboard
-(void) toggleKeyboard {
    self.keyboardRequested = !self.keyboardRequested;
    [self reloadInputViews];
    if (self.keyboardRequested) {
        [self becomeFirstResponder];
    }
}

// Display and hide the keyboard by tapping on the view
-(void) handleTapGesture: (UITapGestureRecognizer*) gestureRecognizer {
    if (gestureRecognizer.state == UIGestureRecognizerStateEnded) {
        [self toggleKeyboard];
    }
}

// Kept for compatibility — no longer used internally
-(void) handleKeyboardInput: (unichar) keycode {
    Lumos::iOSOS::Get()->OnKeyPressed((char)keycode, true);
    Lumos::iOSOS::Get()->OnKeyTyped((char)keycode);
    Lumos::iOSOS::Get()->OnKeyPressed((char)keycode, false);
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

static void iOSShowKeyboard(bool show) {
    LumosViewController *vc = (LumosViewController *)[[[UIApplication sharedApplication] delegate] window].rootViewController;
    vc.keyboardRequested = show;
    [vc reloadInputViews];
    if (show) {
        [vc becomeFirstResponder];
    }
}

extern "C" void iOSSetKeyboardStyle(int style) {
    if (g_KeyboardStyle == style)
        return;
    g_KeyboardStyle = style;
    dispatch_async(dispatch_get_main_queue(), ^{
        LumosViewController *vc = (LumosViewController *)[[[UIApplication sharedApplication] delegate] window].rootViewController;
        if ([vc isKindOfClass:[LumosViewController class]])
            [vc reloadInputViews];
    });
}

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

        // nothing playing through OpenAL: take its output down so iOS sees the app as paused
        // (a Play lifts this again)
        Lumos::AudioSession::SetOpenALHold(Lumos::AudioSession::Hold_Idle, !active && !Lumos::ALMusicStream::AnyPlaying());

        // background audio keeps the app alive; let scripts tick a few times a second
        [self.backgroundTimer invalidate];
        self.backgroundTimer = nil;
        if (!active) {
            self.backgroundTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 repeats:YES block:^(NSTimer* t) {
                if (Lumos::iOSOS::IsQuitting()) { [t invalidate]; return; }
                if (Lumos::Application::HasInstance())
                    Lumos::Application::Get().OnBackgroundUpdate(0.5f);
            }];
        }
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
    if (Lumos::Application::HasInstance() && Lumos::Application::Get().GetWindow())
        Lumos::Application::Get().GetWindow()->SetWindowFocus(false);
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    self.active = NO;
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    if (Lumos::Application::HasInstance() && Lumos::Application::Get().GetWindow())
        Lumos::Application::Get().GetWindow()->SetWindowFocus(false);
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    self.active = YES;
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
    if (Lumos::Application::HasInstance() && Lumos::Application::Get().GetWindow())
        Lumos::Application::Get().GetWindow()->SetWindowFocus(true);
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    self.active = YES;
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
	if(Lumos::Application::HasInstance() && Lumos::Application::Get().GetWindow())
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

extern "C" void iOSSendReturnKey()
{
    if (!Lumos::Application::HasInstance()) return;
    auto* window = (Lumos::iOSWindow*)Lumos::Application::Get().GetWindow();
    if (!window) return;
    window->OnKeyEvent(Lumos::InputCode::Key::Enter, true);
    window->OnKeyEvent(Lumos::InputCode::Key::Enter, false);
}

int main(int argc, char * argv[])
{
	@autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([LumosAppDelegate class]));
    }
}
#endif // LUMOS_PLATFORM_IOS
