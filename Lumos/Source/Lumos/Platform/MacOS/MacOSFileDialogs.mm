#include "Core/OS/FileDialogs.h"

#ifdef LUMOS_PLATFORM_MACOS
#import <AppKit/AppKit.h>
#include <string>
#include <sstream>

namespace Lumos
{
    static NSArray<UTType*>* ParseFilterToUTTypes(const std::string& filter)
    {
        if(filter.empty())
            return nil;

        NSMutableArray<UTType*>* types = [NSMutableArray array];
        std::istringstream stream(filter);
        std::string ext;
        while(std::getline(stream, ext, ','))
        {
            // Trim
            while(!ext.empty() && ext[0] == ' ') ext.erase(0, 1);
            while(!ext.empty() && ext.back() == ' ') ext.pop_back();

            if(!ext.empty())
            {
                NSString* nsExt = [NSString stringWithUTF8String:ext.c_str()];
                UTType* type = [UTType typeWithFilenameExtension:nsExt];
                if(type)
                    [types addObject:type];
            }
        }

        return types.count > 0 ? types : nil;
    }

    std::string FileDialogs::OpenFile(const std::string& filter, const std::string& defaultPath)
    {
        @autoreleasepool
        {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            [panel setCanChooseFiles:YES];
            [panel setCanChooseDirectories:NO];
            [panel setAllowsMultipleSelection:NO];

            NSArray<UTType*>* types = ParseFilterToUTTypes(filter);
            if(types)
                [panel setAllowedContentTypes:types];

            if(!defaultPath.empty())
            {
                NSString* path = [NSString stringWithUTF8String:defaultPath.c_str()];
                [panel setDirectoryURL:[NSURL fileURLWithPath:path]];
            }

            if([panel runModal] == NSModalResponseOK)
            {
                NSURL* url = [[panel URLs] firstObject];
                if(url)
                    return std::string([[url path] UTF8String]);
            }
        }
        return "";
    }

    std::string FileDialogs::SaveFile(const std::string& filter, const std::string& defaultPath, const std::string& defaultName)
    {
        @autoreleasepool
        {
            NSSavePanel* panel = [NSSavePanel savePanel];

            NSArray<UTType*>* types = ParseFilterToUTTypes(filter);
            if(types)
                [panel setAllowedContentTypes:types];

            if(!defaultPath.empty())
            {
                NSString* path = [NSString stringWithUTF8String:defaultPath.c_str()];
                [panel setDirectoryURL:[NSURL fileURLWithPath:path]];
            }

            if(!defaultName.empty())
            {
                NSString* name = [NSString stringWithUTF8String:defaultName.c_str()];
                [panel setNameFieldStringValue:name];
            }

            if([panel runModal] == NSModalResponseOK)
            {
                NSURL* url = [panel URL];
                if(url)
                    return std::string([[url path] UTF8String]);
            }
        }
        return "";
    }

    std::string FileDialogs::PickFolder(const std::string& defaultPath)
    {
        @autoreleasepool
        {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            [panel setCanChooseFiles:NO];
            [panel setCanChooseDirectories:YES];
            [panel setAllowsMultipleSelection:NO];

            if(!defaultPath.empty())
            {
                NSString* path = [NSString stringWithUTF8String:defaultPath.c_str()];
                [panel setDirectoryURL:[NSURL fileURLWithPath:path]];
            }

            if([panel runModal] == NSModalResponseOK)
            {
                NSURL* url = [[panel URLs] firstObject];
                if(url)
                    return std::string([[url path] UTF8String]);
            }
        }
        return "";
    }
}
#endif
