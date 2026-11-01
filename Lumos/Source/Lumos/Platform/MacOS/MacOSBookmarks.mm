#include "Precompiled.h"
#include "Core/OS/Bookmarks.h"
#import <Foundation/Foundation.h>

namespace Lumos
{
#if defined(LUMOS_PLATFORM_MACOS)
    static const NSURLBookmarkCreationOptions kCreate    = NSURLBookmarkCreationWithSecurityScope;
    static const NSURLBookmarkResolutionOptions kResolve = NSURLBookmarkResolutionWithSecurityScope | NSURLBookmarkResolutionWithoutUI;
#else
    static const NSURLBookmarkCreationOptions kCreate    = 0;
    static const NSURLBookmarkResolutionOptions kResolve = 0;
#endif

    std::string Bookmarks::Create(const std::string& path)
    {
        @autoreleasepool
        {
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()] isDirectory:YES];
            NSError* err = nil;
            NSData* data = [url bookmarkDataWithOptions:kCreate includingResourceValuesForKeys:nil relativeToURL:nil error:&err];
            // outside the sandbox security scope isn't available: a plain one still follows moves
            if(!data && kCreate != 0)
                data = [url bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:&err];
            if(!data)
                return std::string();
            return std::string([[data base64EncodedStringWithOptions:0] UTF8String]);
        }
    }

    std::string Bookmarks::Resolve(const std::string& bookmark, bool* stale)
    {
        if(stale)
            *stale = false;
        @autoreleasepool
        {
            NSData* data = [[NSData alloc] initWithBase64EncodedString:[NSString stringWithUTF8String:bookmark.c_str()] options:0];
            if(!data)
                return std::string();
            BOOL isStale = NO;
            NSError* err = nil;
            NSURL* url = [NSURL URLByResolvingBookmarkData:data options:kResolve relativeToURL:nil bookmarkDataIsStale:&isStale error:&err];
            if(!url && kResolve != 0)
                url = [NSURL URLByResolvingBookmarkData:data options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:&isStale error:&err];
            if(!url)
                return std::string();
            // held for the rest of the run: the library reads these folders any time
            [url startAccessingSecurityScopedResource];
            if(stale)
                *stale = isStale;
            return std::string([[url path] UTF8String]);
        }
    }
}
