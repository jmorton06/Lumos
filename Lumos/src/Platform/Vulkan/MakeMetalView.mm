#import <Cocoa/Cocoa.h>

#include <QuartzCore/CAMetalLayer.h>

//static CALayer* orilayer;

extern "C" void* makeViewMetalCompatible(void* handle) {
    //NSView* view = (NSView*)handle;
    //assert([view isKindOfClass:[NSView class]]);
    
    NSWindow* window = (NSWindow*)handle;
    NSView* view = window.contentView;
    
    if (![view.layer isKindOfClass:[CAMetalLayer class]]) {
        //orilayer = [view layer];
        [view setLayer:[CAMetalLayer layer]];
        [view setWantsLayer:YES];
    }
    
    return (void*)view;
}
