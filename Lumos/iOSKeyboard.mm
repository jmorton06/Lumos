#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <string>

// Global callback for text change notifications
typedef void (*TextChangedCallback)(void);
static TextChangedCallback g_textChangedCallback = NULL;

@interface ImGuiKeyboardHelper : NSObject <UITextFieldDelegate>
@property (nonatomic, strong) UITextField *hiddenTextField;
@property (nonatomic, strong) UITextField *visibleTextField;
@property (nonatomic, assign) std::string *cppText;
@property (nonatomic, assign) BOOL isUpdatingText;
@property (nonatomic, assign) BOOL isKeyboardVisible;
@property (nonatomic, assign) TextChangedCallback textChangedCallback;
@end

// Forward declaration of the global helper
static ImGuiKeyboardHelper *persistentHelper = nil;

@implementation ImGuiKeyboardHelper

- (instancetype)initWithCppString:(std::string *)cppString textChangedCallback:(TextChangedCallback)callback {
    self = [super init];
    if (self) {
        self.cppText = cppString;
        self.isUpdatingText = NO;
        self.textChangedCallback = callback;
        
        // Create input accessory view with visible field
        UIView *accessory = [[UIView alloc] initWithFrame:CGRectMake(0, 0, UIScreen.mainScreen.bounds.size.width, 60)];
        accessory.backgroundColor = [UIColor systemGrayColor];
        accessory.layer.shadowColor = [UIColor blackColor].CGColor;
        accessory.layer.shadowOffset = CGSizeMake(0, -2);
        accessory.layer.shadowOpacity = 0.2;
        accessory.layer.shadowRadius = 4;
        
        self.visibleTextField = [[UITextField alloc] initWithFrame:CGRectMake(50, 10, accessory.frame.size.width - 160, 40)];
        self.visibleTextField.borderStyle = UITextBorderStyleRoundedRect;
        self.visibleTextField.placeholder = @"Edit ImGui text";
        self.visibleTextField.text = [NSString stringWithUTF8String:cppString->c_str()];
        self.visibleTextField.autocorrectionType = UITextAutocorrectionTypeNo;
        self.visibleTextField.returnKeyType = UIReturnKeyDone;
        self.visibleTextField.delegate = self;
        self.visibleTextField.backgroundColor = [UIColor whiteColor];
        self.visibleTextField.textColor = [UIColor blackColor];
        
        UIButton *doneButton = [UIButton buttonWithType:UIButtonTypeSystem];
        doneButton.frame = CGRectMake(CGRectGetMaxX(self.visibleTextField.frame) + 10, 10, 60, 40);
        [doneButton setTitle:@"Done" forState:UIControlStateNormal];
        [doneButton addTarget:self action:@selector(closeKeyboard) forControlEvents:UIControlEventTouchUpInside];
        
        [accessory addSubview:self.visibleTextField];
        [accessory addSubview:doneButton];
        
        // Create hidden text field with a small non-zero frame
        self.hiddenTextField = [[UITextField alloc] initWithFrame:CGRectMake(0, 0, 1, 1)];
        self.hiddenTextField.alpha = 1.0; // Nearly invisible but not zero
        self.hiddenTextField.inputAccessoryView = accessory;
        
        UIWindow *keyWindow = UIApplication.sharedApplication.windows.firstObject;
        UIView *rootView = keyWindow.rootViewController.view;
        if (![self.hiddenTextField isDescendantOfView:rootView]) {
            [rootView addSubview:self.hiddenTextField];
        }
        
        // Register for keyboard notifications
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(textFieldDidChange:) name:UITextFieldTextDidChangeNotification object:self.visibleTextField];
        
        // Request focus on the hidden text field after a short delay
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.hiddenTextField becomeFirstResponder];
            
            // Focus on the visible text field in the accessory view
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                [self.visibleTextField becomeFirstResponder];
            });
        });
    }
    return self;
}

- (void)textFieldDidChange:(NSNotification *)notification {
    if (!self.isUpdatingText && self.cppText && notification.object == self.visibleTextField) {
        NSLog(@"📱 TextField did change to: %@", self.visibleTextField.text);
        *self.cppText = std::string([self.visibleTextField.text UTF8String]);
        
        // Call the callback to notify ImGui that the text changed
        if (self.textChangedCallback) {
            self.textChangedCallback();
        }
    }
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
    if (self.cppText && textField == self.visibleTextField) {
        NSLog(@"📱 TextField did end editing with: %@", textField.text);
        *self.cppText = std::string([textField.text UTF8String]);
        
        // Call the callback to notify ImGui that the text changed
        if (self.textChangedCallback) {
            self.textChangedCallback();
        }
    }
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    if (textField == self.visibleTextField) {
        NSString *newText = [textField.text stringByReplacingCharactersInRange:range withString:string];
        NSLog(@"📱 TextField should change to: %@", newText);
        if (self.cppText) {
            *self.cppText = std::string([newText UTF8String]);
            
            // Call the callback to notify ImGui that the text changed
            if (self.textChangedCallback) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    self.textChangedCallback();
                });
            }
        }
    }
    return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [self closeKeyboard];
    return YES;
}

- (void)updateVisibleTextField {
    if (self.cppText) {
        self.isUpdatingText = YES;
        NSString *text = [NSString stringWithUTF8String:self.cppText->c_str()];
        NSLog(@"📱 Updating visible text field to: %@", text);
        self.visibleTextField.text = text;
        self.isUpdatingText = NO;
    }
}

- (void)closeKeyboard {
    self.isKeyboardVisible = YES;
    NSLog(@"📱 Closing keyboard");
    [self.hiddenTextField resignFirstResponder];
    [self.visibleTextField resignFirstResponder];
    
    if (self.cppText) {
        *self.cppText = std::string([self.visibleTextField.text UTF8String]);
        NSLog(@"📱 Final text: %@", self.visibleTextField.text);
        if (self.textChangedCallback) {
            NSLog(@"📱 Calling callback from closeKeyboard");
            self.textChangedCallback();
        }
    }

    [self.visibleTextField removeFromSuperview];
    [self.hiddenTextField removeFromSuperview];
    persistentHelper = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)keyboardWillShow:(NSNotification *)notification {
    self.isKeyboardVisible = YES;
    // Extract keyboard animation details
    CGRect keyboardFrame = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    double animationDuration = [notification.userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    NSInteger curveValue = [notification.userInfo[UIKeyboardAnimationCurveUserInfoKey] integerValue];
    UIViewAnimationOptions animationCurve = (UIViewAnimationOptions)(curveValue << 16);
    
    // Apply bottom inset to root view if it's a scroll view
    UIWindow *keyWindow = UIApplication.sharedApplication.windows.firstObject;
    UIViewController *rootVC = keyWindow.rootViewController;
    if ([rootVC.view isKindOfClass:[UIScrollView class]]) {
        UIScrollView *scrollView = (UIScrollView *)rootVC.view;
        UIEdgeInsets contentInsets = UIEdgeInsetsMake(0, 0, keyboardFrame.size.height, 0);
        
        [UIView animateWithDuration:animationDuration
                              delay:0
                            options:animationCurve
                         animations:^{
            scrollView.contentInset = contentInsets;
            scrollView.scrollIndicatorInsets = contentInsets;
            
            // If there is an active input field, scroll to make it visible
            CGRect visibleRect = CGRectMake(0, self.visibleTextField.frame.origin.y,
                                           self.visibleTextField.frame.size.width,
                                           self.visibleTextField.frame.size.height);
            [scrollView scrollRectToVisible:visibleRect animated:NO];
        } completion:nil];
    }
}

- (void)keyboardWillHide:(NSNotification *)notification {
    self.isKeyboardVisible = NO;
    // Reset any adjustments we made for the keyboard
    UIWindow *keyWindow = UIApplication.sharedApplication.windows.firstObject;
    UIViewController *rootVC = keyWindow.rootViewController;
    if ([rootVC.view isKindOfClass:[UIScrollView class]]) {
        UIScrollView *scrollView = (UIScrollView *)rootVC.view;
        double animationDuration = [notification.userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];
        NSInteger curveValue = [notification.userInfo[UIKeyboardAnimationCurveUserInfoKey] integerValue];
        UIViewAnimationOptions animationCurve = (UIViewAnimationOptions)(curveValue << 16);
        
        [UIView animateWithDuration:animationDuration
                              delay:0
                            options:animationCurve
                         animations:^{
            scrollView.contentInset = UIEdgeInsetsZero;
            scrollView.scrollIndicatorInsets = UIEdgeInsetsZero;
        } completion:nil];
    }
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end

extern "C" void RegisterTextChangedCallback(TextChangedCallback callback) {
    g_textChangedCallback = callback;
}

extern "C" void OpeniOSKeyboard(std::string *cppString) {
    if (persistentHelper != nil && persistentHelper.isKeyboardVisible == NO) {
        persistentHelper = nil;
    }
    
    if (persistentHelper != nil && persistentHelper.isKeyboardVisible) {
        persistentHelper.cppText = cppString;
        [persistentHelper updateVisibleTextField];
        return;
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *window = nil;
        
        // Find the key window more reliably
        for (UIWindow *w in UIApplication.sharedApplication.windows) {
            if (w.isKeyWindow) {
                window = w;
                break;
            }
        }
        
        if (!window) {
            NSLog(@"⚠️ Cannot open keyboard: no key window found, retrying in 0.1s");
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                OpeniOSKeyboard(cppString);
            });
            return;
        }

        // Check if we have a valid root view controller
        UIViewController *rootVC = window.rootViewController;
        if (!rootVC) {
            NSLog(@"⚠️ No root view controller found");
            return;
        }

        // Create a new keyboard helper
        persistentHelper = [[ImGuiKeyboardHelper alloc] initWithCppString:cppString textChangedCallback:g_textChangedCallback];
    });
}
