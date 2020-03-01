#import <UIKit/UIKit.h>


#pragma mark -
#pragma mark ViewController

/** The main view controller for the demo storyboard. */
@interface ViewController : UIViewController <UIKeyInput>

- (BOOL)prefersStatusBarHidden;

- (BOOL)prefersHomeIndicatorAutoHidden;

- (BOOL)shouldAutorotateToInterfaceOrientation;

- (BOOL)hasNotch;

- (void)didReceiveMemoryWarning;

- (void)hideKeyboard;

- (void)showKeyboard;

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures;

@end

#pragma mark -
#pragma mark MetalView

@interface MetalView : UIView
@end
