#include "Precompiled.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

namespace Lumos
{
    // Rounded card window: clear window background, content layer clipped to the radius,
    // shadow traced from the visible pixels.
    void MacOSSetWindowShape(GLFWwindow* handle, float cornerRadius, bool shadow)
    {
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(handle);
        if(!window)
            return;
        NSView* view = window.contentView;
        view.wantsLayer = YES;

        if(cornerRadius > 0.0f)
        {
            window.opaque          = NO;
            window.backgroundColor = [NSColor clearColor];
            view.layer.cornerRadius  = cornerRadius;
            view.layer.masksToBounds = YES;
            if(@available(macOS 10.15, *))
                view.layer.cornerCurve = kCACornerCurveContinuous;
        }
        else
        {
            view.layer.cornerRadius  = 0.0;
            view.layer.masksToBounds = NO;
            window.opaque            = YES;
            window.backgroundColor   = [NSColor windowBackgroundColor];
        }
        window.hasShadow = shadow;
        [window invalidateShadow];
        // the shadow is traced from what's on screen; re-trace once the new shape has been drawn
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.15 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
          [window invalidateShadow];
        });
    }

    void MacOSSetWindowTransparent(GLFWwindow* handle, bool transparent)
    {
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(handle);
        if(!window)
            return;
        NSView* view    = window.contentView;
        view.wantsLayer = YES;
        if(transparent)
        {
            window.opaque          = NO;
            window.backgroundColor = [NSColor clearColor];
            view.layer.opaque      = NO;
        }
        else if(view.layer.cornerRadius <= 0.0)
        {
            window.opaque          = YES;
            window.backgroundColor = [NSColor windowBackgroundColor];
        }
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.15 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
          [window invalidateShadow];
        });
    }
}
