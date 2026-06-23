#include "Core/OS/FileDialogs.h"

#ifdef LUMOS_PLATFORM_MACOS
#import <AppKit/AppKit.h>
#include <string>
#include <sstream>

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

namespace Lumos
{
    static NSArray<NSString*>* ParseFilterToExtensions(const std::string& filter)
    {
        if(filter.empty())
            return nil;

        NSMutableArray<NSString*>* exts = [NSMutableArray array];
        std::istringstream stream(filter);
        std::string ext;
        while(std::getline(stream, ext, ','))
        {
            while(!ext.empty() && ext[0] == ' ') ext.erase(0, 1);
            while(!ext.empty() && ext.back() == ' ') ext.pop_back();
            if(!ext.empty())
                [exts addObject:[NSString stringWithUTF8String:ext.c_str()]];
        }
        return exts.count > 0 ? exts : nil;
    }

    static void ApplyFilter(NSSavePanel* panel, const std::string& filter)
    {
        NSArray<NSString*>* exts = ParseFilterToExtensions(filter);
        if(!exts)
            return;

        if(@available(macOS 11.0, *))
        {
            NSMutableArray<UTType*>* types = [NSMutableArray array];
            for(NSString* ext in exts)
            {
                UTType* type = [UTType typeWithFilenameExtension:ext];
                if(type)
                    [types addObject:type];
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

    std::string FileDialogs::OpenFile(const std::string& filter, const std::string& defaultPath)
    {
        @autoreleasepool
        {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            [panel setCanChooseFiles:YES];
            [panel setCanChooseDirectories:NO];
            [panel setAllowsMultipleSelection:NO];

            ApplyFilter(panel, filter);

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
            ApplyFilter(panel, filter);

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
