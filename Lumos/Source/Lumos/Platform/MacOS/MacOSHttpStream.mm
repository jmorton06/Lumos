#include "Precompiled.h"
#include "Core/OS/HttpStream.h"
#import <Foundation/Foundation.h>

// One session per stream so its delegate gets the body as it arrives
@interface LumosHttpStreamDelegate : NSObject <NSURLSessionDataDelegate>
{
@public
    Lumos::HttpStream* owner;
}
@end

@implementation LumosHttpStreamDelegate
- (void)URLSession:(NSURLSession*)session dataTask:(NSURLSessionDataTask*)task didReceiveResponse:(NSURLResponse*)response completionHandler:(void (^)(NSURLSessionResponseDisposition))completionHandler
{
    if(owner)
    {
        int status = 0;
        __block Lumos::Http::Headers headers;
        if([response isKindOfClass:[NSHTTPURLResponse class]])
        {
            NSHTTPURLResponse* http = (NSHTTPURLResponse*)response;
            status                  = (int)http.statusCode;
            [http.allHeaderFields enumerateKeysAndObjectsUsingBlock:^(id key, id value, BOOL*) {
                headers.push_back({ std::string([[key description] UTF8String]), std::string([[value description] UTF8String]) });
            }];
        }
        owner->OnResponse(status, std::move(headers));
    }
    completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession*)session dataTask:(NSURLSessionDataTask*)task didReceiveData:(NSData*)data
{
    if(owner)
    {
        [data enumerateByteRangesUsingBlock:^(const void* bytes, NSRange range, BOOL*) {
            owner->OnData((const u8*)bytes, range.length);
        }];
    }
}

- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didCompleteWithError:(NSError*)error
{
    if(owner)
        owner->OnComplete(error ? std::string(error.localizedDescription.UTF8String) : std::string());
}
@end

namespace Lumos
{
    namespace
    {
        struct MacStream
        {
            NSURLSession* session;
            LumosHttpStreamDelegate* delegate;
            NSURLSessionDataTask* task;
            NSOperationQueue* queue;
        };
    }

    void HttpStreamPlatform::Start(HttpStream* stream, const std::string& url, const Http::Headers& headers)
    {
        @autoreleasepool
        {
            NSURL* nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
            if(!nsurl)
            {
                stream->OnComplete("bad url");
                return;
            }
            NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:nsurl cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:20.0];
            for(auto& h : headers)
                [request setValue:[NSString stringWithUTF8String:h.second.c_str()] forHTTPHeaderField:[NSString stringWithUTF8String:h.first.c_str()]];

            MacStream* ms  = new MacStream();
            ms->delegate   = [[LumosHttpStreamDelegate alloc] init];
            ms->delegate->owner = stream;
            NSURLSessionConfiguration* config = [NSURLSessionConfiguration ephemeralSessionConfiguration];
            config.timeoutIntervalForResource = 60 * 60 * 24; // live streams run for hours
            NSOperationQueue* queue = [[NSOperationQueue alloc] init];
            queue.maxConcurrentOperationCount = 1;
            ms->session = [[NSURLSession sessionWithConfiguration:config delegate:ms->delegate delegateQueue:queue] retain];
            ms->queue   = queue; // owned: Stop waits on it
            ms->task = [[ms->session dataTaskWithRequest:request] retain];
            stream->Platform = ms;
            [ms->task resume];
        }
    }

    void HttpStreamPlatform::Stop(HttpStream* stream)
    {
        MacStream* ms = (MacStream*)stream->Platform;
        if(!ms)
            return;
        @autoreleasepool
        {
            ms->delegate->owner = nullptr; // callbacks already queued see nothing
            [ms->task cancel];
            [ms->session invalidateAndCancel]; // releases the delegate
            [ms->queue waitUntilAllOperationsAreFinished]; // one may be mid-call on the stream
            [ms->queue release];
            [ms->task release];
            [ms->session release];
            [ms->delegate release];
        }
        delete ms;
        stream->Platform = nullptr;
    }
}
