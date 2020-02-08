#import "ViewController.h"

#include "../iOSOS.h"
#include "../iOSWindow.h"

#include "UIWindow.h"

#pragma mark -
#pragma mark ViewController

@implementation ViewController {
    CADisplayLink* _displayLink;
    BOOL _viewHasAppeared;
    Lumos::iOSOS* os;
    LumosUIWindow* uiWindow;
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

- (BOOL)shouldAutorotate {
        return YES;
};

/** Since this is a single-view app, init Vulkan when the view is loaded. */
-(void) viewDidLoad {
	[super viewDidLoad];

    self.view.contentScaleFactor = UIScreen.mainScreen.nativeScale;
    Lumos::iOSWindow::SetIOSView(self.view);
    Lumos::iOSOS::SetWindowSize(self.view.bounds.size.width * self.view.contentScaleFactor, self.view.bounds.size.height * self.view.contentScaleFactor);
    Lumos::iOSOS::Create();
    os = (Lumos::iOSOS*)Lumos::iOSOS::Instance();
    os->SetIOSView(self.view);
    
//    uiWindow = [ LumosUIWindow alloc ];
//    [ uiWindow initWithFrame:[ [ UIScreen mainScreen ] bounds ] ];
//    uiWindow.backgroundColor = [ UIColor whiteColor ];
//    [ uiWindow makeKeyAndVisible ];
//    [ uiWindow setMultipleTouchEnabled:YES ];

    
    uint32_t fps = 60;
    _displayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderFrame)];
    [_displayLink setFrameInterval: 60 / fps];
    [_displayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];

    // Setup tap gesture to toggle virtual keyboard
    UITapGestureRecognizer* tapSelector = [[UITapGestureRecognizer alloc]
                                           initWithTarget: self action: @selector(handleTapGesture:)];
    tapSelector.numberOfTapsRequired = 1;
    tapSelector.cancelsTouchesInView = YES;
    [self.view addGestureRecognizer: tapSelector];

    _viewHasAppeared = NO;
}

-(void) viewDidAppear: (BOOL) animated {
    [super viewDidAppear: animated];
    _viewHasAppeared = YES;
}

-(BOOL) canBecomeFirstResponder { return _viewHasAppeared; }

-(void) renderFrame {
    os->OnFrame();
}

-(void) dealloc {
    os->OnQuit();
    delete os;
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
    os->OnKeyPressed((u32)keycode);
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


@end


#pragma mark -
#pragma mark View

@implementation View

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

@end

