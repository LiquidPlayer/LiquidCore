//
//  MicroService.m
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <LiquidCore/LiquidCore.h>
#import "Process.h"

@interface LCMicroService() <ProcessDelegate>
@property (atomic, assign, readonly, class) NSMutableDictionary *serviceMap;
@property (nonatomic) JSValue *emitter;
@property (atomic, readwrite) Process *process;
@property (atomic, assign) bool fetched;
@property (atomic) NSMutableArray* eventListeners;
- (NSError*) fetchService;
@end

@interface LCSynchronizer()
- (id) init;
- (void) blockUntilReady;
@property (atomic) int count;
@end

@implementation LCSynchronizer
- (id) init
{
    self = [super init];
    if (self) {
        _count = 0;
    }
    return self;
}

- (void) enter
{
    self.count ++;
}

- (void) exit
{
    -- self.count;
}

- (bool) isSyncrhonized
{
    return (self.count == 0);
}

- (void) blockUntilReady
{
    volatile int count = self.count;
    while (count > 0) {
        count = self.count;
    }
}
@end

@implementation LCMicroService {
    NSURL* serviceURI_;
    NSString* serviceId_;
    id<LCMicroServiceDelegate> delegate_;
    bool started_;
    NSArray* argv_;
    NSString* module_;
}
static NSMutableDictionary* _serviceMap = nil;
+ (NSMutableDictionary *)serviceMap { return _serviceMap; }
+ (id) serviceFromInstanceId:(NSString*)instanceId
{
    return [LCMicroService.serviceMap objectForKey:instanceId];
}
+ (void) uninstall:(NSURL *)serviceURI
{
    NSString *serviceId;
    NSRange comp = [[serviceURI absoluteString] rangeOfString:@"/" options:NSBackwardsSearch];
    if (comp.location != NSNotFound) {
        serviceId = [[serviceURI absoluteString] substringToIndex:comp.location];
    } else {
        serviceId = [serviceURI absoluteString];
    }
    serviceId = [serviceId stringByAddingPercentEncodingWithAllowedCharacters:
                  [NSCharacterSet URLHostAllowedCharacterSet]];

    [Process uninstall:serviceId scope:GLOBAL];
}

- (id) initWithURL:(NSURL*)serviceURI
{
    return [self initWithURL:serviceURI delegate:nil];
}

- (id) initWithURL:(NSURL*)serviceURI delegate:(id<LCMicroServiceDelegate>)delegate
{
    self = [super init];
    if (self) {
        serviceURI_ = serviceURI;
        NSRange comp = [[serviceURI absoluteString] rangeOfString:@"/" options:NSBackwardsSearch];
        if (comp.location != NSNotFound) {
            serviceId_ = [[serviceURI absoluteString] substringToIndex:comp.location];
            module_ = [[serviceURI absoluteString] substringFromIndex:comp.location+1];
        } else {
            serviceId_ = [serviceURI absoluteString];
            module_ = serviceId_;
        }
        comp = [module_ rangeOfString:@"?"];
        if (comp.location != NSNotFound) {
            module_ = [module_ substringToIndex:comp.location];
        }

        serviceId_ = [serviceId_ stringByAddingPercentEncodingWithAllowedCharacters:
                      [NSCharacterSet URLHostAllowedCharacterSet]];
        if (module_.length < 3 || ![[module_ substringFromIndex:module_.length - 3] isEqualToString:@".js"]) {
            module_ = [NSString stringWithFormat:@"%@.js", module_];
        }
        delegate_ = delegate;
        _instanceId = [[NSUUID UUID] UUIDString];
        if (_serviceMap == nil) {
            _serviceMap = [[NSMutableDictionary alloc] init];
        }
        _emitter = nil;
        started_ = false;
        _process = nil;
        _fetched = false;
        _eventListeners = [[NSMutableArray alloc] init];
        [LCMicroService.serviceMap setObject:self forKey:self.instanceId];
    }
    return self;
}

- (NSError*) fetchService
{
    NSString *localPath = [NSString stringWithFormat:@"%@/%@", self.process.modulePath, module_];
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSError __block *error = nil;
    
    if ([serviceURI_ isFileURL]) {
        // Symlink file for speed
        if ([fileManager fileExistsAtPath:localPath]) {
            [fileManager removeItemAtPath:localPath error:&error];
        }
        if (error == nil) {
            [fileManager createSymbolicLinkAtURL:[NSURL fileURLWithPath:localPath] withDestinationURL:serviceURI_ error:&error];
        }
        self.fetched = true;
    } else {
        NSDate* lastModified = nil;
        if ([fileManager fileExistsAtPath:localPath]) {
            NSDictionary* attr = [fileManager attributesOfItemAtPath:localPath error:&error];
            lastModified = [attr objectForKey:NSFileModificationDate];
        }

        NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:serviceURI_];
        [request setHTTPMethod:@"GET"];
        if (lastModified) {
            NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
            [dateFormatter setDateFormat:@"EEE, d MMM yyyy HH:mm:ss"];
            [dateFormatter setTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]];
            NSString *formatted = [NSString stringWithFormat:@"%@ GMT", [dateFormatter stringFromDate:lastModified]];
            [request setValue:formatted forHTTPHeaderField:@"If-Modified-Since"];
        }
        [request setValue:@"gzip" forHTTPHeaderField:@"Accept-Encoding"];
        [request setValue:@"application/javascript" forHTTPHeaderField:@"Accept"];

        NSDictionary *infoDictionary = [[NSBundle bundleForClass:self.class]infoDictionary];        
        NSString *version = [infoDictionary objectForKey:@"CFBundleShortVersionString"];
        NSString *info = [NSString stringWithFormat:@"iOS; API=%@", [[UIDevice currentDevice] systemVersion]];

        NSString *bindings = nil;
        NSBundle* bundle = [NSBundle bundleForClass:[self class]];
        NSString* node_modules = [bundle pathForResource:@"node_modules" ofType:nil];
        NSDirectoryEnumerator * enumerator = [fileManager enumeratorAtPath:node_modules];
        [enumerator skipDescendants];
        NSString* path;
        while (path = [enumerator nextObject])
        {
            if (![path containsString:@"/"]) {
                if (bindings) {
                    bindings = [NSString stringWithFormat:@"%@; %@", bindings, path];
                } else {
                    bindings = [NSString stringWithString:path];
                }
            }
        }
        if (bindings) bindings = [NSString stringWithFormat:@" Binding (%@)", bindings];
        else bindings = @"";
        
        NSString *surfaces = nil;
        if (self.availableSurfaces != nil) {
            for (Class<LCSurface> surface in self.availableSurfaces) {
                NSString *sfc = [NSString stringWithFormat:@"%@/%@",
                                 [[surface class] SURFACE_CANONICAL_NAME],
                                 [[surface class] SURFACE_VERSION]];
                if (surfaces != nil) {
                    surfaces = [NSString stringWithFormat:@"%@; %@", surfaces, sfc];
                } else {
                    surfaces = sfc;
                }
            }
        }
        if (surfaces) surfaces = [NSString stringWithFormat:@" Surface (%@)", surfaces];
        else surfaces = @"";

        NSString *userAgent = [NSString stringWithFormat:@"LiquidCore/%@ (%@)%@%@", version, info, surfaces, bindings];
        NSLog(@"MicroService User-Agent : %@", userAgent);
        [request setValue:userAgent forHTTPHeaderField:@"User-Agent"];
        [request setHTTPMethod:@"GET"];
        
        NSURLSessionConfiguration *configuration = [NSURLSessionConfiguration defaultSessionConfiguration];
        NSURLSession *session = [NSURLSession sessionWithConfiguration:configuration delegate:nil delegateQueue:nil];
        NSURLSessionDownloadTask *downloadTask =
        [session downloadTaskWithRequest:request
                   completionHandler:^(NSURL *location, NSURLResponse *response, NSError *e)
        {
            NSHTTPURLResponse *http = (NSHTTPURLResponse*)response;
            error = nil;
            if (http.statusCode == 200) {
                if ([[NSFileManager defaultManager] fileExistsAtPath:localPath]) {
                    [[NSFileManager defaultManager] removeItemAtPath:localPath error:nil];
                }
                [[NSFileManager defaultManager] moveItemAtURL:location
                                                        toURL:[NSURL fileURLWithPath:localPath]
                                                        error:nil];
            } else if (http.statusCode != 304) { // 304 just means the file has not changed
                error = e;
            }
            self.fetched = true;
        }];
        [downloadTask resume];
    }
    
    // This needs to be synchronous
    volatile bool fetched = self.fetched;
    while (!fetched) {
        fetched = self.fetched;
    }
    return error;
}

- (void) onProcessStart:(Process*)process context:(JSContext*)context
{
    // Create LiquidCore EventEmitter
    [context evaluateScript:
     @"class LiquidCore_ extends require('events') {}\n"
     @"global.LiquidCore = new LiquidCore_();"];
    self.emitter = context[@"LiquidCore"];
    
    @try
    {
        NSError* error = [self fetchService];
        if (error) @throw error;
        
        LCSynchronizer *synchronizer = nil;
        if( delegate_ && [delegate_ respondsToSelector:@selector(onStart:synchronizer:)]) {
            synchronizer = [[LCSynchronizer alloc] init];
            [delegate_ onStart:self synchronizer:synchronizer];
        }
        
        // Construct process.argv
        NSMutableArray *argv = [[NSMutableArray alloc] init];
        [argv addObject:@"node"];
        [argv addObject:[NSString stringWithFormat:@"/home/module/%@", module_]];
        if (argv_) {
            [argv addObjectsFromArray:argv_];
        }
        context[@"process"][@"argv"] = argv;
        
        // Execute code
        NSString *script =
        [NSString stringWithFormat:@"eval(String(require('fs').readFileSync('/home/module/%@')))", module_];
        
        if (synchronizer && ![synchronizer isSyncrhonized]) {
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [synchronizer blockUntilReady];
                [process sync:^(JSContext* context) {
                    [context evaluateScript:script];
                }];
            });
        } else {
            [context evaluateScript:script];
        }
    }
    @catch (NSError* error)
    {
        [self onProcessFailed:process exception:[NSException exceptionWithName:error.localizedDescription
                                                                        reason:error.localizedFailureReason
                                                                      userInfo:error.userInfo]];
    }
}

- (void) shutDown
{
    delegate_ = nil;
    self.emitter = nil;
    [LCMicroService.serviceMap removeObjectForKey:serviceId_];
    self.process = nil;
}

- (void) onProcessAboutToExit:(Process*)process exitCode:(int)code
{
    if( delegate_ && [delegate_ respondsToSelector:@selector(onExit:exitCode:)]) {
        [delegate_ onExit:self exitCode:code];
    }
    [self shutDown];
}

- (void) onProcessExit:(Process*)process exitCode:(int)code
{
    if( delegate_ && [delegate_ respondsToSelector:@selector(onExit:exitCode:)]) {
        [delegate_ onExit:self exitCode:code];
    }
    [self shutDown];
}

- (void) onProcessFailed:(Process*)process exception:(NSException*)exception
{
    if( delegate_ && [delegate_ respondsToSelector:@selector(onError:exception:)]) {
        [delegate_ onError:self exception:exception];
    }
    [self shutDown];
}

- (void) addEventListener:(NSString*)event
                 listener:(id<LCMicroServiceEventListener>)listener
{
    if (self.emitter != nil) {
        [self.process sync:^(JSContext *context) {
            JSValue *jsListener = [JSValue valueWithObject:^(JSValue *value) {
                if ([value isBoolean]) {
                    [listener onEvent:self event:event payload:[NSNumber numberWithBool:[value toBool]]];
                } else if ([value isNumber]) {
                    [listener onEvent:self event:event payload:[value toNumber]];
                } else if ([value isString]) {
                    [listener onEvent:self event:event payload:[value toString]];
                } else if ([value isObject]) {
                    [listener onEvent:self event:event payload:[value toObject]];
                } else if ([value isNull] || [value isUndefined]) {
                    [listener onEvent:self event:event payload:nil];
                } else {
                    assert(0);
                }
            } inContext:context];
            [self.emitter invokeMethod:@"on" withArguments:@[event, jsListener]];

            [self.eventListeners addObject:@[event, listener, jsListener]];
        }];
    }
}

- (void) removeEventListener:(NSString*)event
                    listener:(id<LCMicroServiceEventListener>)listener
{
    if (self.emitter != nil) {
        [self.process sync:^(JSContext* context) {
            for (NSArray* l in self.eventListeners) {
                if ([l[0] isEqualToString:event] && [l[1] isEqualToObject:listener]) {
                    [self.emitter invokeMethod:@"removeListener" withArguments:@[event, l[2]]];
                    [self.eventListeners removeObject:l];
                }
            }
        }];
    }
}

- (void) emit:(NSString*)event
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter invokeMethod:@"emit" withArguments:@[event]];
        }];
    }
}

- (void) emitObject:(NSString*)event object:(id)object
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter invokeMethod:@"emit" withArguments:@[event, object]];
        }];
    }
}

- (void) emitNumber:(NSString*)event number:(NSNumber*)number
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter invokeMethod:@"emit" withArguments:@[event, number]];
        }];
    }
}

- (void) emitString:(NSString*)event string:(NSString*)string
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter invokeMethod:@"emit" withArguments:@[event, string]];
        }];
    }
}

- (void) emitBoolean:(NSString*)event boolean:(bool)boolean
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter invokeMethod:@"emit" withArguments:@[event, [JSValue valueWithBool:boolean
                                                                                   inContext:context]]];
        }];
    }
}

- (void) start
{
    [self startWithArguments:nil];
}

- (void) startWithArguments:(NSArray*)argv
{
    if (started_) {
        @throw [[NSException alloc] initWithName:@"ServiceAlreadyStartedError"
                                          reason:@"MicroServices can only be started once"
                                        userInfo:nil];
    } else {
        started_ = true;
        argv_ = argv;
        self.process = [[Process alloc] initWithDelegate:self id:serviceId_ mediaAccessMask:PermissionsRW];
    }
}

@end
