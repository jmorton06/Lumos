#import "ViewController.h"

#include "../iOSOS.h"
#include "../iOSWindow.h"
#include "../iOSKeyCodes.h"

#include "UIWindow.h"

#import <QuartzCore/CAMetalLayer.h>
#include <MoltenVK/vk_mvk_moltenvk.h>

#pragma mark -
#pragma mark ViewController

@implementation ViewController {
    CADisplayLink* _displayLink;
    CAMetalLayer* _metalLayer;
    BOOL _viewHasAppeared;
    Lumos::iOSOS* os;
    LumosUIWindow* uiWindow;
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
	[super viewDidLoad];

    self.view.contentScaleFactor = [[UIScreen mainScreen] scale];
    self.view.contentMode = UIViewContentModeScaleAspectFill;

    if (@available(iOS 11.0, *)) {
        [self setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
    }
    
    assert([self.view isKindOfClass:[UIView class]]);
    
    self.view.autoresizesSubviews = true;
    self.modalPresentationStyle = UIModalPresentationFullScreen;
    _viewHasAppeared = NO;

}

-(void) viewDidAppear: (BOOL) animated {
    [super viewDidAppear: animated];
    
    _viewHasAppeared = YES;
}

- (void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    NSLog(@"bounds = %@", NSStringFromCGRect(self.view.bounds));
    
    CGSize drawableSize = self.view.bounds.size;
          
    // Since drawable size is in pixels, we need to multiply by the scale to move from points to pixels
    drawableSize.width *= self.view.contentScaleFactor;
    drawableSize.height *= self.view.contentScaleFactor;

    _metalLayer = [CAMetalLayer new];
    _metalLayer.frame = self.view.frame;
    _metalLayer.opaque = true;
    _metalLayer.framebufferOnly = true;
    _metalLayer.drawableSize = drawableSize;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.contentsScale = self.view.contentScaleFactor;

    [self.view.layer addSublayer:_metalLayer];

    Lumos::iOSWindow::SetIOSView((__bridge void *)_metalLayer);
    Lumos::iOSOS::SetWindowSize(self.view.bounds.size.width * self.view.contentScaleFactor, self.view.bounds.size.height * self.view.contentScaleFactor);
    Lumos::iOSOS::Create();
    os = (Lumos::iOSOS*)Lumos::iOSOS::Instance();
    os->SetIOSView((__bridge void *)_metalLayer);
     
    uint32_t fps = 60;
    _displayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderFrame)];
    _displayLink.preferredFramesPerSecond = fps;
    [_displayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];

    // Setup tap gesture to toggle virtual keyboard
    UITapGestureRecognizer* tapSelector = [[UITapGestureRecognizer alloc]
                                        initWithTarget: self action: @selector(handleTapGesture:)];
    tapSelector.numberOfTapsRequired = 1;
    tapSelector.cancelsTouchesInView = YES;
    [self.view addGestureRecognizer: tapSelector];
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
    os->OnKeyPressed(Lumos::iOSKeyCodes::iOSKeyToLumos(keycode));
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
