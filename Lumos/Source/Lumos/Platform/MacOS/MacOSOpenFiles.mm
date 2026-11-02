#include "Core/OS/OpenFiles.h"
#include "Core/LMLog.h"
#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

// GLFW's app delegate doesn't take open-document events: add the handler to its class.
static void OpenURLs(id self, SEL _cmd, NSApplication* app, NSArray<NSURL*>* urls)
{
    for(NSURL* url in urls)
        if(url.isFileURL && url.path)
            Lumos::OpenFiles::Push(url.path.UTF8String);
}

namespace Lumos
{
    namespace OpenFiles
    {
        void Install()
        {
            static bool done = false;
            if(done)
                return;
            id delegate = [NSApp delegate];
            if(!delegate)
                return;
            done = true;
            SEL sel = @selector(application:openURLs:);
            if(!class_addMethod([delegate class], sel, (IMP)OpenURLs, "v@:@@"))
                LWARN("OpenFiles: app delegate already handles open events");
        }
    }
}
