#include "iOSOS.h"
#include "iOSWindow.h"
#include "iOSKeyCodes.h"
#include "Application.h"
#include "Core/VFS.h"
#include "Core/OS/Input.h"
#include "Core/CoreSystem.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

#include <sys/sysctl.h>
#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioServices.h>

namespace Lumos
{
    iOSOS::iOSOS()
    {
    }

    iOSOS::~iOSOS()
    {
        Lumos::Application::Release();
    }

    void iOSOS::Init()
    {
        Lumos::Internal::CoreSystem::Init(false);

        String root = GetAssetPath();
        Lumos::VFS::Get()->Mount("CoreShaders", root + "/Assets/shaders");
        Lumos::VFS::Get()->Mount("CoreMeshes", root + "/Assets/meshes");
        Lumos::VFS::Get()->Mount("CoreTextures", root + "/Assets/textures");

        Lumos::VFS::Get()->Mount("Shaders", root + "/Assets/shaders");
        Lumos::VFS::Get()->Mount("Meshes", root + "/Assets/meshes");
        Lumos::VFS::Get()->Mount("Textures", root + "/Assets/textures");
        
        Lumos::Debug::Log::Info("Device : {0}",GetModelName());
        
        iOSWindow::MakeDefault();

        s_Instance = this;

        auto app = Lumos::CreateApplication();
        app->Init();
    }

    void iOSOS::OnFrame()
    {
        Application::Instance()->OnFrame();
    }
    
    void iOSOS::OnQuit()
    {
        delete Application::Instance();
	    Lumos::Internal::CoreSystem::Shutdown();
    }
    
    String iOSOS::GetAssetPath()
    {
        return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/"].UTF8String;
    }
    
    void iOSOS::OnKeyPressed(char keycode, bool down)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnKeyEvent((Lumos::InputCode::Key)Lumos::iOSKeyCodes::iOSKeyToLumos(keycode), down);
    }

    void iOSOS::OnScreenPressed(u32 x, u32 y, u32 count, bool down)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnTouchEvent(x,y,count, down);
    }

    void iOSOS::OnMouseMovedEvent(u32 xPos, u32 yPos)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnMouseMovedEvent(xPos,yPos);
    }
    
    void iOSOS::OnScreenResize(u32 width, u32 height)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnResizeEvent(width, height);
    }

    String iOSOS::GetExecutablePath()
    {
        return GetAssetPath();
        static char path[512] = "";
        if (!path[0]) 
        {
            NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
            strcpy(path, (const char *)[bundlePath cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        return path;
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
        NSString* nsmsg = [[NSString alloc] initWithBytes:message
                            length:strlen(message) * sizeof(message)
                            encoding:NSUTF8StringEncoding];
        
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
    
    String iOSOS::GetModelName() const
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
        return String(str != NULL ? str : "");
    }
}

#import <UIKit/UIKit.h>

#ifdef LUMOS_RENDER_API_VULKAN
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#else
#define LUMOS_RENDER_API_OPENGL
#import <GLKit/GLKit.h>
#endif

#include "iOSKeyCodes.h"
#include "App/Application.h"

#define MAX_TOUCHES 10

@interface LumosViewController : UIViewController
  @property (strong, nonatomic)  CADisplayLink* _displayLink;

- (BOOL)prefersStatusBarHidden;
- (BOOL)prefersHomeIndicatorAutoHidden;
- (BOOL)shouldAutorotateToInterfaceOrientation;
- (BOOL)hasNotch;
- (void)didReceiveMemoryWarning;
- (void)hideKeyboard;
- (void)showKeyboard;
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures;
@end


@interface LumosAppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow*             window;
@property (strong, nonatomic) CAMetalLayer*         m_MetalLayer;
@property (strong, nonatomic) LumosViewController*  m_ViewController;

@end

@implementation LumosAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    @autoreleasepool {
        
    CGRect bounds = [[UIScreen mainScreen] bounds]; // size in "points"
    float scale = [[UIScreen mainScreen] nativeScale]; // scale for retina dimensions

    self.window = [[UIWindow alloc] initWithFrame:bounds];
    [self.window setBackgroundColor:[UIColor blackColor]];
    [self.window makeKeyAndVisible];
    self.window.contentScaleFactor = scale;
        
    id<MTLDevice> mtlDevice = MTLCreateSystemDefaultDevice();

    self.m_ViewController = [[LumosViewController alloc] initWithNibName:nil bundle:nil];

    [self.window setRootViewController:self.m_ViewController];
    self.m_ViewController.view.multipleTouchEnabled = YES;
    self.m_ViewController.view.bounds = bounds;
    self.m_ViewController.view.contentScaleFactor = scale;
        
    self.m_MetalLayer = [CAMetalLayer new];
    self.m_MetalLayer.drawableSize = CGSizeMake(scale * bounds.size.width, scale * bounds.size.height);

    self.m_MetalLayer.frame = self.window.layer.frame;
    self.m_MetalLayer.device = mtlDevice;
    self.m_MetalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    self.m_MetalLayer.framebufferOnly = false;
    self.m_MetalLayer.opaque = YES;
    //[self.window.layer addSublayer:self.m_MetalLayer];
        
    [self.m_ViewController.view.layer addSublayer:self.m_MetalLayer];
    
    Lumos::iOSOS::Create();
    Lumos::iOSOS* os = (Lumos::iOSOS*)Lumos::iOSOS::Instance();
    os->SetIOSView((__bridge void *)self.m_MetalLayer);//self.m_ViewController.view);
    os->SetWindowSize(self.m_ViewController.view.bounds.size.width * self.m_ViewController.view.contentScaleFactor, self.m_ViewController.view.bounds.size.height * self.m_ViewController.view.contentScaleFactor);
    os->Init();
        
    return YES;
    }
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end

@implementation LumosViewController

static UITouch* touches[MAX_TOUCHES];

static int getTouchId(UITouch *touch, bool remove)
{
    int next = -1;
    for (int i = 0; i < MAX_TOUCHES; i++) {
        if (touches[i] == touch){
            if (remove)
                touches[i] = NULL;
            return i;
        }
        if (next == -1 && touches[i] == NULL) {
            next = i;
        }
    }
    
    if (next != -1) {
        touches[next] = touch;
        return next;
    }
    
    return -1;
}

static void clearTouches()
{
    for (int i = 0; i < MAX_TOUCHES; i++) {
        touches[i] = NULL;
    }
}


- (void)didReceiveMemoryWarning {
    printf("Receive memory warning!\n");
};

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    return UIRectEdgeAll;
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

- (BOOL)shouldAutorotateToInterfaceOrientation {
    return YES;
};

/** Since this is a single-view app, init Vulkan when the view is loaded. */
-(void) viewDidLoad {
    
    //self.view.contentScaleFactor = [[UIScreen mainScreen] scale];
    //self.view.contentMode = UIViewContentModeScaleAspectFill;
    self.view.multipleTouchEnabled = true;

    if (@available(iOS 11.0, *)) {
        [self setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
    }
    
    assert([self.view isKindOfClass:[UIView class]]);
    
    //self.view.autoresizesSubviews = true;
    //self.modalPresentationStyle = UIModalPresentationFullScreen;

}

-(void) viewDidAppear: (BOOL) animated {
    [super viewDidAppear: animated];
}

- (void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
     
    uint32_t fps = 120;
    __displayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderFrame)];
    __displayLink.preferredFramesPerSecond = fps;
    [__displayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];

    // Setup tap gesture to toggle virtual keyboard
    UITapGestureRecognizer* tapSelector = [[UITapGestureRecognizer alloc]
                                        initWithTarget: self action: @selector(handleTapGesture:)];
    tapSelector.numberOfTapsRequired = 3;
    tapSelector.cancelsTouchesInView = YES;
    [self.view addGestureRecognizer: tapSelector];
}

-(void) renderFrame {
    ((Lumos::iOSOS*)Lumos::iOSOS::Instance())->OnFrame();
}

-(void) dealloc {
    ((Lumos::iOSOS*)Lumos::iOSOS::Instance())->OnQuit();
    delete (Lumos::iOSOS*)Lumos::iOSOS::Instance();
    [super dealloc];
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

#pragma mark UIKeyInput methods

// Returns whether text is available
-(BOOL) hasText { return YES; }

// A key on the keyboard has been pressed.
-(void) insertText: (NSString*) text {
    unichar keycode = (text.length > 0) ? [text characterAtIndex: 0] : 0;
    [self handleKeyboardInput: keycode];
}

// The delete backward key has been pressed.
-(void) deleteBackward {
    [self handleKeyboardInput: 0x33];
}

-( void )touchesBegan:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self.view ];
        Lumos::iOSOS::Get()->OnScreenPressed(point.x * self.view.contentScaleFactor, point.y * self.view.contentScaleFactor, (u32)getTouchId(touch,false), true);
    }
}

-( void )touchesMoved:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self.view ];
        Lumos::iOSOS::Get()->OnMouseMovedEvent(point.x * self.view.contentScaleFactor, point.y * self.view.contentScaleFactor);
    }
}

-( void )touchesEnded:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self.view ];
        Lumos::iOSOS::Get()->OnScreenPressed(point.x * self.view.contentScaleFactor, point.y * self.view.contentScaleFactor, (u32)getTouchId(touch,false),false);
    }
}

-( void )touchesCancelled:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self.view ];
        Lumos::iOSOS::Get()->OnScreenPressed(point.x * self.view.contentScaleFactor, point.y * self.view.contentScaleFactor, (u32)getTouchId(touch,false),false);
    }

    clearTouches();
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
    [self becomeFirstResponder];
};

- (void)hideKeyboard {
    [self resignFirstResponder];
};

@end

int main(int argc, char * argv[])
{
    NSString* str = NSStringFromClass([LumosAppDelegate class]);
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, str);
    }
}
