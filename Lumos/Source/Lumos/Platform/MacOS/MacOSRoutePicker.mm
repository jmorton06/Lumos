#include "Precompiled.h"
#include "Audio/AudioOutput.h"
#import <AVKit/AVKit.h>
#if defined(LUMOS_PLATFORM_IOS)
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

// AVRoutePickerView has no "show" call: keep an invisible one where the popover should point
// and press its button.
namespace Lumos
{
    bool ShowAudioRoutePicker(float x, float y)
    {
        @autoreleasepool
        {
#if defined(LUMOS_PLATFORM_IOS)
            static AVRoutePickerView* s_Picker = nil;
            UIWindow* window = nil;
            for(UIWindow* w in UIApplication.sharedApplication.windows)
                if(w.isKeyWindow)
                    window = w;
            UIView* host = window.rootViewController.view;
            if(!host)
                return false;
            const CGFloat scale = host.contentScaleFactor > 0 ? host.contentScaleFactor : 1;
            if(!s_Picker)
            {
                s_Picker       = [[AVRoutePickerView alloc] initWithFrame:CGRectMake(0, 0, 1, 1)];
                s_Picker.alpha = 0.011; // hit-testable but unseen
            }
            if(s_Picker.superview != host)
                [host addSubview:s_Picker];
            s_Picker.frame = CGRectMake(x / scale, y / scale, 1, 1);
            for(UIView* v in s_Picker.subviews)
                if([v isKindOfClass:[UIButton class]])
                {
                    [(UIButton*)v sendActionsForControlEvents:UIControlEventTouchUpInside];
                    return true;
                }
            return false;
#else
            static AVRoutePickerView* s_Picker = nil;
            NSWindow* window = NSApp.keyWindow ? NSApp.keyWindow : NSApp.mainWindow;
            NSView* host     = window.contentView;
            if(!host)
                return false;
            const CGFloat scale = window.backingScaleFactor > 0 ? window.backingScaleFactor : 1;
            if(!s_Picker)
            {
                s_Picker = [[AVRoutePickerView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
                s_Picker.alphaValue = 0.011;
            }
            if(s_Picker.superview != host)
                [host addSubview:s_Picker];
            const CGFloat px = x / scale, py = host.isFlipped ? y / scale : host.bounds.size.height - y / scale;
            s_Picker.frame = NSMakeRect(px, py, 1, 1);
            for(NSView* v in s_Picker.subviews)
                if([v isKindOfClass:[NSButton class]])
                {
                    [(NSButton*)v performClick:nil];
                    return true;
                }
            return false;
#endif
        }
    }
}
