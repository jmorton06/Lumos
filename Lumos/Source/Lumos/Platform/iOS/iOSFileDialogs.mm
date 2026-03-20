#include "Core/OS/FileDialogs.h"

#ifdef LUMOS_PLATFORM_IOS
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <objc/runtime.h>
#include <string>
#include <sstream>

// Forward declaration
@interface LumosDocPickerDelegate : NSObject <UIDocumentPickerDelegate>
@property (nonatomic, copy) void (^completionBlock)(NSString*);
- (instancetype)initWithCompletion:(void (^)(NSString*))completion;
@end

namespace Lumos
{
    static NSArray<UTType*>* ParseFilterToUTTypes(const std::string& filter)
    {
        if(filter.empty())
            return @[UTTypeItem];

        NSMutableArray<UTType*>* types = [NSMutableArray array];
        std::istringstream stream(filter);
        std::string ext;
        while(std::getline(stream, ext, ','))
        {
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

        return types.count > 0 ? types : @[UTTypeItem];
    }

    static UIViewController* GetRootViewController()
    {
        UIWindow* window = nil;
        for(UIScene* scene in [UIApplication sharedApplication].connectedScenes)
        {
            if([scene isKindOfClass:[UIWindowScene class]])
            {
                UIWindowScene* windowScene = (UIWindowScene*)scene;
                for(UIWindow* w in windowScene.windows)
                {
                    if(w.isKeyWindow)
                    {
                        window = w;
                        break;
                    }
                }
            }
            if(window) break;
        }
        return window.rootViewController;
    }

    std::string FileDialogs::OpenFile(const std::string& filter, const std::string& defaultPath)
    {
        __block std::string result = "";
        __block bool done = false;

        dispatch_async(dispatch_get_main_queue(), ^{
            UIViewController* vc = GetRootViewController();
            if(!vc)
            {
                done = true;
                return;
            }

            NSArray<UTType*>* types = ParseFilterToUTTypes(filter);
            UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types];
            picker.allowsMultipleSelection = NO;

            LumosDocPickerDelegate* delegate = [[LumosDocPickerDelegate alloc] initWithCompletion:^(NSString* path) {
                if(path)
                    result = std::string([path UTF8String]);
                done = true;
            }];

            // Prevent delegate from being deallocated by associating with picker
            objc_setAssociatedObject(picker, "delegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            picker.delegate = delegate;

            [vc presentViewController:picker animated:YES completion:nil];
        });

        // Spin run loop until done (with timeout)
        NSDate* timeout = [NSDate dateWithTimeIntervalSinceNow:300.0];
        while(!done && [[NSDate date] compare:timeout] == NSOrderedAscending)
        {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
        }

        return result;
    }

    std::string FileDialogs::SaveFile(const std::string& filter, const std::string& defaultPath, const std::string& defaultName)
    {
        // iOS uses Documents directory for save operations
        if(!defaultName.empty())
        {
            NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString* documentsDir = [paths firstObject];
            NSString* name = [NSString stringWithUTF8String:defaultName.c_str()];
            NSString* fullPath = [documentsDir stringByAppendingPathComponent:name];
            return std::string([fullPath UTF8String]);
        }
        return "";
    }

    std::string FileDialogs::PickFolder(const std::string& defaultPath)
    {
        // Return app Documents directory
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString* documentsDir = [paths firstObject];
        return std::string([documentsDir UTF8String]);
    }
}

@implementation LumosDocPickerDelegate
- (instancetype)initWithCompletion:(void (^)(NSString*))completion
{
    self = [super init];
    if(self)
        _completionBlock = [completion copy];
    return self;
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    NSURL* url = [urls firstObject];
    if(url)
        [url startAccessingSecurityScopedResource];

    if(self.completionBlock)
        self.completionBlock(url ? [url path] : nil);

    if(url)
        [url stopAccessingSecurityScopedResource];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
    if(self.completionBlock)
        self.completionBlock(nil);
}
@end

#endif
