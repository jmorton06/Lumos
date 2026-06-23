#ifdef LUMOS_PLATFORM_MACOS

#import <Cocoa/Cocoa.h>
#include "MacOSFileDialog.h"

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

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
                NSMutableArray<NSString*>* exts = [NSMutableArray array];
                for(auto& f : filters)
                {
                    std::string ext = f;
                    if(!ext.empty() && ext[0] == '.')
                        ext = ext.substr(1);
                    [exts addObject:[NSString stringWithUTF8String:ext.c_str()]];
                }

                if(@available(macOS 11.0, *))
                {
                    NSMutableArray<UTType*>* types = [NSMutableArray array];
                    for(NSString* ext in exts)
                    {
                        UTType* utType = [UTType typeWithFilenameExtension:ext];
                        if(utType)
                            [types addObject:utType];
                    }
                    if(types.count > 0)
                        [panel setAllowedContentTypes:types];
                }
                else
                {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                    [panel setAllowedFileTypes:exts];
#pragma clang diagnostic pop
                }
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
