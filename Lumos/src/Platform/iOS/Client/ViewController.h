#import <UIKit/UIKit.h>


#pragma mark -
#pragma mark ViewController

/** The main view controller for the demo storyboard. */
@interface ViewController : UIViewController <UIKeyInput>

- (BOOL)prefersStatusBarHidden;

- (BOOL)prefersHomeIndicatorAutoHidden;

- (BOOL)shouldAutorotateToInterfaceOrientation;

- (void)didReceiveMemoryWarning;

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures;

@end
