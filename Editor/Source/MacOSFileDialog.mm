#ifdef LUMOS_PLATFORM_MACOS

#import <Cocoa/Cocoa.h>
#include "MacOSFileDialog.h"

namespace Lumos
{
    void OpenNativeFileDialog(bool selectDirectory,
                              const std::vector<std::string>& filters,
                              const std::string& startPath,
                              const std::function<void(const std::string&)>& callback)
    {
        @autoreleasepool
        {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            [panel setCanChooseFiles:!selectDirectory];
            [panel setCanChooseDirectories:selectDirectory];
            [panel setAllowsMultipleSelection:NO];
            panel.canCreateDirectories = YES;
            
            if(!startPath.empty())
            {
                NSString* path = [NSString stringWithUTF8String:startPath.c_str()];
                [panel setDirectoryURL:[NSURL fileURLWithPath:path]];
            }

            if(!selectDirectory && !filters.empty())
            {
                NSMutableArray* types = [NSMutableArray array];
                for(auto& f : filters)
                {
                    std::string ext = f;
                    if(!ext.empty() && ext[0] == '.')
                        ext = ext.substr(1);
                    [types addObject:[NSString stringWithUTF8String:ext.c_str()]];
                }

                NSMutableArray* utTypes = [NSMutableArray array];
                for(NSString* ext in types)
                {
                    UTType* utType = [UTType typeWithFilenameExtension:ext];
                    if(utType)
                        [utTypes addObject:utType];
                }
                if([utTypes count] > 0)
                    [panel setAllowedContentTypes:utTypes];
            }

            NSModalResponse result = [panel runModal];
            if(result == NSModalResponseOK)
            {
                NSURL* url = [[panel URLs] firstObject];
                if(url && callback)
                    callback([[url path] UTF8String]);
            }
        }
    }
}

#endif
