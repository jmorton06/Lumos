#ifdef LUMOS_PLATFORM_IOS

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <objc/runtime.h>
#include "iOSFileDialog.h"
#include <Lumos/Core/OS/OS.h>

@interface LumosEditorDocPickerDelegate : NSObject <UIDocumentPickerDelegate>
@property (nonatomic, copy) void (^completionBlock)(NSString*);
- (instancetype)initWithCompletion:(void (^)(NSString*))completion;
@end

// Prevent delegate from being deallocated while picker is active
static LumosEditorDocPickerDelegate* s_ActiveDelegate = nil;

namespace Lumos
{
    static UIViewController* GetEditorRootViewController()
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

    void OpenNativeFileDialog(bool selectDirectory,
                              const std::vector<std::string>& filters,
                              const std::string& startPath,
                              const std::function<void(const std::string&)>& callback)
    {
        if(selectDirectory)
        {
            std::string docsDir = OS::Get().GetCurrentWorkingDirectory();
            if(callback)
                callback(docsDir);
            return;
        }

        auto cb = callback;

        dispatch_async(dispatch_get_main_queue(), ^{
            UIViewController* vc = GetEditorRootViewController();
            if(!vc)
                return;

            NSMutableArray<UTType*>* types = [NSMutableArray array];
            for(auto& f : filters)
            {
                std::string ext = f;
                if(!ext.empty() && ext[0] == '.')
                    ext = ext.substr(1);
                NSString* nsExt = [NSString stringWithUTF8String:ext.c_str()];
                UTType* type = [UTType typeWithFilenameExtension:nsExt];
                if(type)
                    [types addObject:type];
            }
            if([types count] == 0)
                [types addObject:UTTypeItem];

            UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types];
            picker.allowsMultipleSelection = NO;

            if(!startPath.empty())
            {
                NSString* path = [NSString stringWithUTF8String:startPath.c_str()];
                picker.directoryURL = [NSURL fileURLWithPath:path];
            }

            s_ActiveDelegate = [[LumosEditorDocPickerDelegate alloc] initWithCompletion:^(NSString* path) {
                if(path && cb)
                    cb(std::string([path UTF8String]));
                s_ActiveDelegate = nil;
            }];

            picker.delegate = s_ActiveDelegate;
            [vc presentViewController:picker animated:YES completion:nil];
        });
    }
}

@implementation LumosEditorDocPickerDelegate
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
