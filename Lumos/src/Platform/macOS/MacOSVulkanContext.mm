#import <Cocoa/Cocoa.h>

#include <QuartzCore/CAMetalLayer.h>

extern "C" void* MakeViewMetalCompatible(void* handle)
{
    NSWindow* window = (NSWindow*)handle;
    NSView* view = window.contentView;
    
    if (![view.layer isKindOfClass:[CAMetalLayer class]])
    {
        [view setLayer:[CAMetalLayer layer]];
        [view setWantsLayer:YES];
    }
    
    return (void*)view;
}
