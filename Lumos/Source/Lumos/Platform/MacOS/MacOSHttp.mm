#include "Precompiled.h"
#include "Core/OS/Http.h"
#import <Foundation/Foundation.h>

// Redirects to plain http (old podcast / CDN links) are asked for over https instead: the system
// refuses insecure loads, and nearly every host answers both.
@interface LumosHttpRedirects : NSObject <NSURLSessionTaskDelegate>
@end

@implementation LumosHttpRedirects
- (void)URLSession:(NSURLSession*)session
                          task:(NSURLSessionTask*)task
    willPerformHTTPRedirection:(NSHTTPURLResponse*)response
                    newRequest:(NSURLRequest*)request
             completionHandler:(void (^)(NSURLRequest*))completionHandler
{
    if([request.URL.scheme caseInsensitiveCompare:@"http"] == NSOrderedSame)
    {
        NSURLComponents* parts = [NSURLComponents componentsWithURL:request.URL resolvingAgainstBaseURL:NO];
        parts.scheme           = @"https";
        NSMutableURLRequest* secure = [[request mutableCopy] autorelease];
        secure.URL                  = parts.URL;
        completionHandler(secure);
        return;
    }
    completionHandler(request);
}
@end

namespace Lumos
{
    static NSURLSession* Session()
    {
        static NSURLSession* session = nil;
        static dispatch_once_t once;
        dispatch_once(&once, ^{
            // kept for the life of the app
            session = [[NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]
                                                     delegate:[[[LumosHttpRedirects alloc] init] autorelease]
                                                delegateQueue:nil] retain];
        });
        return session;
    }

    static void Send(u32 id, const std::string& url, const Http::Headers& headers, const std::string* body);

    void Http::Detail::PlatformGet(u32 id, const std::string& url, const Headers& headers) { Send(id, url, headers, nullptr); }
    void Http::Detail::PlatformPost(u32 id, const std::string& url, const std::string& body, const Headers& headers) { Send(id, url, headers, &body); }

    void Http::Detail::PlatformDownload(u32 id, const std::string& url, const std::string& path, const Headers& headers)
    {
        @autoreleasepool
        {
            NSURL* nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
            if(!nsurl)
            {
                Complete(id, 0, std::string(), "bad url");
                return;
            }
            NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:nsurl];
            for(auto& h : headers)
                [request setValue:[NSString stringWithUTF8String:h.second.c_str()] forHTTPHeaderField:[NSString stringWithUTF8String:h.first.c_str()]];
            NSString* dest = [NSString stringWithUTF8String:path.c_str()];

            NSURLSessionDownloadTask* task = [Session()
                downloadTaskWithRequest:request
                      completionHandler:^(NSURL* location, NSURLResponse* response, NSError* err) {
                          int status = 0;
                          if([response isKindOfClass:[NSHTTPURLResponse class]])
                              status = (int)((NSHTTPURLResponse*)response).statusCode;
                          std::string error = err ? std::string(err.localizedDescription.UTF8String) : std::string();
                          // the temporary file is gone once this block returns: move it now
                          if(!err && location && status >= 200 && status < 300)
                          {
                              NSFileManager* fm = [NSFileManager defaultManager];
                              NSError* moveErr  = nil;
                              [fm removeItemAtPath:dest error:nil];
                              if(![fm moveItemAtURL:location toURL:[NSURL fileURLWithPath:dest] error:&moveErr])
                                  error = moveErr ? std::string(moveErr.localizedDescription.UTF8String) : std::string("couldn't save the file");
                          }
                          Complete(id, status, std::string(), std::move(error));
                      }];
            [task resume];
        }
    }

    static void Send(u32 id, const std::string& url, const Http::Headers& headers, const std::string* body)
    {
        using Http::Detail::Complete;
        @autoreleasepool
        {
            NSURL* nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
            if(!nsurl)
            {
                Complete(id, 0, std::string(), "bad url");
                return;
            }
            NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:nsurl
                                                                   cachePolicy:NSURLRequestUseProtocolCachePolicy
                                                               timeoutInterval:30.0];
            for(auto& h : headers)
                [request setValue:[NSString stringWithUTF8String:h.second.c_str()] forHTTPHeaderField:[NSString stringWithUTF8String:h.first.c_str()]];

            if(body)
            {
                request.HTTPMethod = @"POST";
                request.HTTPBody   = [NSData dataWithBytes:body->data() length:body->size()];
            }

            NSURLSessionDataTask* task = [Session()
                dataTaskWithRequest:request
                  completionHandler:^(NSData* data, NSURLResponse* response, NSError* err) {
                      int status = 0;
                      if([response isKindOfClass:[NSHTTPURLResponse class]])
                          status = (int)((NSHTTPURLResponse*)response).statusCode;
                      std::string reply = data ? std::string((const char*)data.bytes, data.length) : std::string();
                      std::string error = err ? std::string(err.localizedDescription.UTF8String) : std::string();
                      Complete(id, status, std::move(reply), std::move(error));
                  }];
            [task resume];
        }
    }
}
