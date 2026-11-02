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
@property (nonatomic, assign) BOOL keepAccess; // folders: the app reads them after the picker closes
- (instancetype)initWithCompletion:(void (^)(NSString*))completion;
@end

@interface LumosImportPickerDelegate : NSObject <UIDocumentPickerDelegate>
@property (nonatomic, copy) void (^completionBlock)(NSArray<NSURL*>*);
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

    // A folder from the Files app (iCloud Drive, On My iPhone, a USB drive...). Access stays open for
    // the run; keep a Bookmarks::Create of the path to get it back next launch.
    std::string FileDialogs::PickFolder(const std::string& defaultPath)
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
            UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeFolder ]];
            picker.allowsMultipleSelection = NO;
            LumosDocPickerDelegate* delegate = [[LumosDocPickerDelegate alloc] initWithCompletion:^(NSString* path) {
                if(path)
                    result = std::string([path UTF8String]);
                done = true;
            }];
            delegate.keepAccess = YES;
            objc_setAssociatedObject(picker, "delegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            picker.delegate = delegate;
            [vc presentViewController:picker animated:YES completion:nil];
        });

        NSDate* timeout = [NSDate dateWithTimeIntervalSinceNow:300.0];
        while(!done && [[NSDate date] compare:timeout] == NSOrderedAscending)
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
        return result;
    }

    TDArray<std::string> FileDialogs::ImportFiles(const std::string& filter, const std::string& destDir)
    {
        __block TDArray<std::string> result;
        __block bool done = false;
        NSString* dest = [NSString stringWithUTF8String:destDir.c_str()];
        NSString* filterStr = [[[NSString stringWithUTF8String:filter.c_str()] lowercaseString] stringByReplacingOccurrencesOfString:@" " withString:@""];
        NSArray<NSString*>* exts = [filterStr componentsSeparatedByString:@","];

        dispatch_async(dispatch_get_main_queue(), ^{
            UIViewController* vc = GetRootViewController();
            if(!vc)
            {
                done = true;
                return;
            }
            NSMutableArray<UTType*>* types = [ParseFilterToUTTypes(filter) mutableCopy];
            [types addObject:UTTypeAudio];
            // asCopy: iOS hands over copies in our tmp inbox, no security-scoped access to keep
            UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types asCopy:YES];
            picker.allowsMultipleSelection = YES;
            LumosImportPickerDelegate* delegate = [[LumosImportPickerDelegate alloc] init];
            delegate.completionBlock = ^(NSArray<NSURL*>* urls) {
                NSFileManager* fm = [NSFileManager defaultManager];
                [fm createDirectoryAtPath:dest withIntermediateDirectories:YES attributes:nil error:nil];
                for(NSURL* url in urls)
                {
                    NSString* name = url.lastPathComponent;
                    // the picker can tack a type extension on ("book.m4b" -> "book.m4b.mp4"): keep the original one
                    NSString* inner = name.stringByDeletingPathExtension;
                    if(inner.pathExtension.length > 0 && [exts containsObject:inner.pathExtension.lowercaseString])
                        name = inner;
                    NSString* target = [dest stringByAppendingPathComponent:name];
                    for(int n = 2; [fm fileExistsAtPath:target]; n++)
                        target = [dest stringByAppendingPathComponent:[NSString stringWithFormat:@"%@ %d.%@", name.stringByDeletingPathExtension, n, name.pathExtension]];
                    NSError* err = nil;
                    if([fm moveItemAtURL:url toURL:[NSURL fileURLWithPath:target] error:&err] ||
                       [fm copyItemAtURL:url toURL:[NSURL fileURLWithPath:target] error:&err])
                        result.PushBack(std::string(target.UTF8String));
                    else
                        NSLog(@"Import failed for %@: %@", name, err);
                }
                done = true;
            };
            objc_setAssociatedObject(picker, "delegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            picker.delegate = delegate;
            [vc presentViewController:picker animated:YES completion:nil];
        });

        NSDate* timeout = [NSDate dateWithTimeIntervalSinceNow:600.0];
        while(!done && [[NSDate date] compare:timeout] == NSOrderedAscending)
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
        return result;
    }
}

@implementation LumosImportPickerDelegate
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    if(self.completionBlock)
        self.completionBlock(urls);
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
    if(self.completionBlock)
        self.completionBlock(@[]);
}
@end

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

    if(url && !self.keepAccess)
        [url stopAccessingSecurityScopedResource];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
    if(self.completionBlock)
        self.completionBlock(nil);
}
@end

#endif
