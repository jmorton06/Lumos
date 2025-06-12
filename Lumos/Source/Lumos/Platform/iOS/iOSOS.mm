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

        std::string root = GetAssetPath();
		/*
        Lumos::VFS::Get().Mount("CoreShaders", root + "Shaders");

       // Lumos::VFS::Get().Mount("Shaders", root + "shaders");
        Lumos::VFS::Get().Mount("Meshes", root + "Meshes");
        Lumos::VFS::Get().Mount("Textures", root + "Textures");
        Lumos::VFS::Get().Mount("Scripts", root + "Scripts");
        Lumos::VFS::Get().Mount("Scenes", root + "Scenes");
        Lumos::VFS::Get().Mount("Sounds", root + "Sounds");
        Lumos::VFS::Get().Mount("Assets", root + "Assets");
*/
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

    void iOSOS::OnKeyPressed(char keycode, bool down)
    {
        ((iOSWindow*)Application::Get().GetWindow())->OnKeyEvent((Lumos::InputCode::Key)Lumos::iOSKeyCodes::iOSKeyToLumos(keycode), down);
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

    std::string iOSOS::GetExecutablePath()
    {
        //Get iOS Documents Directory
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectory = [paths objectAtIndex:0];
        return [documentsDirectory stringByAppendingString: @"/"].UTF8String;

        //return GetAssetPath();
#if 0
        static char path[512] = "";
        if (!path[0])
        {
            NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
            strcpy(path, (const char *)[bundlePath cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        return path;
#endif
    }

    void iOSOS::Vibrate() const
    {
        @autoreleasepool
        {
            AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
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

    void iOSOS::ShowKeyboard(bool open)
    {

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
@property(nonatomic, assign) BOOL requestResize;

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
        self.delegate = self;
        self.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
        self.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
        self.sampleCount = 1;

		// Configure the Metal layer
		CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
		metalLayer.device = device;
		metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
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
        self.requestResize = YES;
    }

   Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();
   if(!os->OnFrame())
   {
	   os->OnQuit();
   }
   
   if(self.requestResize)
   {
	Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::GetPtr();
    os->OnScreenResize(self.drawableWidth, self.drawableHeight);
    self.requestResize = NO;
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

@interface LumosViewController : UIViewController<UIKeyInput, UITextInputTraits> {
    const void *activeTouches[MAX_SIMULTANEOUS_TOUCHES];
}

@property(nonatomic, strong) id<MTLDevice> metalDevice;
@property(nonatomic, assign) BOOL multipleTouchEnabled;
@property(nonatomic, assign) BOOL keyboardRequested;
@property(nonatomic, assign) BOOL keyboardVisible;
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
    //self.lumosView.animating = delegate.active;

#if TARGET_OS_IOS
    self.view.multipleTouchEnabled = self.multipleTouchEnabled;

    [self setNeedsStatusBarAppearanceUpdate];

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(keyboardFrameChanged:)
                                               name:UIKeyboardWillChangeFrameNotification
                                             object:self.view.window];
#endif

	self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	self.multipleTouchEnabled = true;

	if (![self.view.layer isKindOfClass:[CAMetalLayer class]]) {
		NSLog(@"Error: The layer is not a CAMetalLayer.");
	}
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
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

#pragma mark - UIResponder

- (void)clearTouches {
    for (int i = 0; i < MAX_SIMULTANEOUS_TOUCHES; i++) {
        activeTouches[i] = NULL;
    }
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
    for (UITouch *touch in touches) {
        [self addTouchEvent:touch withType:TouchPhaseBegan];
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self addTouchEvent:touch withType:TouchPhaseMoved];
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self addTouchEvent:touch withType:TouchPhaseEnded];
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self addTouchEvent:touch withType:TouchPhaseCancelled];
    }
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
        keyCommands = @[ [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow
                                             modifierFlags:(UIKeyModifierFlags)0
                                                    action:@selector(keyPressed:)],
                         [UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow
                                             modifierFlags:(UIKeyModifierFlags)0
                                                    action:@selector(keyPressed:)],
                         [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow
                                             modifierFlags:(UIKeyModifierFlags)0
                                                    action:@selector(keyPressed:)],
                         [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow
                                             modifierFlags:(UIKeyModifierFlags)0
                                                    action:@selector(keyPressed:)],
                         [UIKeyCommand keyCommandWithInput:UIKeyInputEscape
                                             modifierFlags:(UIKeyModifierFlags)0
                                                    action:@selector(keyPressed:)] ];
#if !__has_feature(objc_arc)
        [keyCommands retain];
#endif
    }

    return keyCommands;
}

- (void)keyPressed:(UIKeyCommand *)keyCommand {
    Lumos::iOSOS::Get()->OnKeyPressed('d', true);
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

-( void )layoutSubviews
{
    Lumos::iOSOS::Get()->OnScreenResize(self.view.bounds.size.width * self.view.contentScaleFactor, self.view.bounds.size.height * self.view.contentScaleFactor);
}

- (BOOL)hasNotch
{
    if(@available(iOS 11.0, *)){
        return [UIApplication sharedApplication].delegate.window.safeAreaInsets.bottom > 0.0f;
    }
    return NO;
}

- (void)showKeyboard {
    [self.view becomeFirstResponder];
};

- (void)hideKeyboard {
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
