#include "Core/OS/FileActions.h"
#import <Cocoa/Cocoa.h>

namespace Lumos
{
    namespace FileActions
    {
        bool MoveToTrash(const std::string& path)
        {
            @autoreleasepool
            {
                NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
                NSError* err = nil;
                return [[NSFileManager defaultManager] trashItemAtURL:url resultingItemURL:nil error:&err] == YES;
            }
        }

        bool CopyFilesToClipboard(const TDArray<std::string>& paths)
        {
            @autoreleasepool
            {
                NSMutableArray* urls = [NSMutableArray array];
                for(u32 i = 0; i < paths.Size(); i++)
                    [urls addObject:[NSURL fileURLWithPath:[NSString stringWithUTF8String:paths[i].c_str()]]];
                if(urls.count == 0)
                    return false;
                NSPasteboard* pb = [NSPasteboard generalPasteboard];
                [pb clearContents];
                return [pb writeObjects:urls] == YES;
            }
        }
    }
}
