#import <UIKit/UIKit.h>


#pragma mark -
#pragma mark ViewController

/** The main view controller for the demo storyboard. */
@interface ViewController : UIViewController <UIKeyInput>

- (BOOL)prefersStatusBarHidden;

- (BOOL)prefersHomeIndicatorAutoHidden;

- (BOOL)shouldAutorotateToInterfaceOrientation;

@end


#pragma mark -
#pragma mark View

/** The Metal-compatibile view for the demo Storyboard. */
@interface View : UIView
@end

