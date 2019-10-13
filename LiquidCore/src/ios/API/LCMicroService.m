/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <LiquidCore/LiquidCore.h>
#import "LCProcess.h"
#import "LCAddOn.h"

@interface LCAddOnFactory()
@property (atomic, readonly, class) NSMutableDictionary *factories;
@end

@interface LCMicroService() <LCProcessDelegate>
@property (atomic, assign, readonly, class) NSMapTable *serviceMap;
@property (nonatomic) JSManagedValue *emitter;
@property (atomic, readwrite) LCProcess *process;
@property (atomic, assign) bool fetched;
@property (atomic) NSMutableArray* eventListeners;
@property (nonatomic, weak) JSContext *context;
@property (nonatomic, weak) NSObject<LCMicroServiceDelegate> *delegate;
- (NSError*) fetchService;
@end

@implementation LCMicroService {
    NSString* serviceId_;
    bool started_;
    NSArray* argv_;
    NSString* module_;
}
static NSMapTable* _serviceMap = nil;
+ (NSMapTable *)serviceMap { return _serviceMap; }
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

    [LCProcess uninstall:serviceId scope:GLOBAL];
}

+ (NSURL *)devServer:(NSString *)fileName port:(NSNumber *)port
{
    if (fileName == nil) {
        fileName = @"liquid.js";
    }
    if (port == nil) {
        port = @(8082);
    }
    if (fileName.length >= 3 && [[fileName substringFromIndex:fileName.length-3] isEqualToString:@".js"])
        fileName = [fileName substringToIndex:fileName.length-3];
    if (fileName.length < 7 || ![[fileName substringFromIndex:fileName.length-7] isEqualToString:@".bundle"]) {
        fileName = [NSString stringWithFormat:@"%@.bundle", fileName];
    }
    NSString* loopback = [NSString stringWithFormat:@"http://localhost:%@/%@?platform=ios&dev=true",
                         port, fileName];
    return [NSURL URLWithString:loopback];
}

+ (NSURL *)devServer
{
    return [LCMicroService devServer:nil port:nil];
}

- (id) initWithURL:(NSURL*)serviceURI
{
    return [self initWithURL:serviceURI delegate:nil];
}

- (id) initWithURL:(NSURL*)serviceURI delegate:(id<LCMicroServiceDelegate>)delegate
{
    self = [super init];
    if (self) {
        _serviceURI = serviceURI;
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
        _delegate = delegate;
        _instanceId = [[NSUUID UUID] UUIDString];
        if (_serviceMap == nil) {
            _serviceMap = [NSMapTable strongToStrongObjectsMapTable];
        }
        _emitter = nil;
        started_ = false;
        _process = nil;
        _fetched = false;
        _eventListeners = [[NSMutableArray alloc] init];
        [LCMicroService.serviceMap setObject:self forKey:_instanceId];
    }
    return self;
}

- (NSError*) fetchService
{
    NSString *localPath = [NSString stringWithFormat:@"%@/%@", self.process.modulePath, module_];
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSError __block *error = nil;
    
    if ([self.serviceURI isFileURL]) {
        // Symlink file for speed
        if ([fileManager fileExistsAtPath:localPath]) {
            [fileManager removeItemAtPath:localPath error:&error];
        }
        if (error == nil) {
            [fileManager createSymbolicLinkAtURL:[NSURL fileURLWithPath:localPath] withDestinationURL:self.serviceURI error:&error];
        }
        self.fetched = true;
    } else {
        NSDate* lastModified = nil;
        if ([fileManager fileExistsAtPath:localPath]) {
            NSDictionary* attr = [fileManager attributesOfItemAtPath:localPath error:&error];
            lastModified = [attr objectForKey:NSFileModificationDate];
        }

        NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:self.serviceURI];
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
        
        NSString *userAgent = [NSString stringWithFormat:@"LiquidCore/%@ (%@)", version, info];
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

- (JSValue*) bindings:(JSContext*)context
               module:(NSString*)module
              require:(JSValue*)require
{
    NSString* fname = [module lastPathComponent];
    NSString* ext = [module pathExtension];
    NSString* moduleName = [fname stringByDeletingPathExtension];
    
    if (![@"node" isEqualToString:ext]) {
        return [require callWithArguments:@[module]];
    }
    
    LCAddOnFactory *factory = LCAddOnFactory.factories[moduleName];
    if (!factory) {
        NSLog(@"%@ has no registered native binding.", moduleName);
    } else {
        id<LCAddOn> addOn = [factory createInstance];
        [addOn register:moduleName];

        [[NSFileManager defaultManager]
         createFileAtPath:[NSString stringWithFormat:@"%@/%@", self.process.node_modulesPath, fname]
         contents:[@"" dataUsingEncoding:NSUTF8StringEncoding]
         attributes:nil];
        JSValue* binding = [require callWithArguments:@[[NSString stringWithFormat:@"/home/node_modules/%@", fname]]];
        [addOn require:binding service:self];
        return binding;
    }
    
    return [JSValue valueWithUndefinedInContext:context];
}

- (void) onProcessStart:(LCProcess*)process context:(JSContext*)context
{
    self.context = context;
    
    // Create LiquidCore EventEmitter
    [context evaluateScript:
     @"class LiquidCore_ extends require('events') {}\n"
     @"global.LiquidCore = new LiquidCore_();"];
    self.emitter = [JSManagedValue managedValueWithValue:context[@"LiquidCore"]];
    
    // Override require() function to handle module binding
    JSManagedValue* require = [JSManagedValue managedValueWithValue:context[@"require"]];
    __weak LCMicroService *weakSelf = self;
    JSValue* bindings = [JSValue valueWithObject:^(NSString* module) {
        return [weakSelf bindings:weakSelf.context module:module require:require.value];
    } inContext:context];
    
    JSObjectSetPrototype([context JSGlobalContextRef],
                         JSValueToObject([context JSGlobalContextRef], [bindings JSValueRef], 0),
                         [require.value JSValueRef]);
    context[@"require"] = bindings;
    
    @try
    {
        NSError* error = [self fetchService];
        if (error) @throw error;
        
        if(self.delegate && [self.delegate respondsToSelector:@selector(onStart:)]) {
            [self.delegate onStart:self];
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
        NSString *script = [NSString stringWithFormat:
        @"(()=>{"
        @"  const fs = require('fs'), vm = require('vm'); "
        @"  (new vm.Script(fs.readFileSync('/home/module/%@'), "
        @"     {filename: '%@'} )).runInThisContext();"
        @"})()", module_, module_];
        
        [context evaluateScript:script];
    }
    @catch (NSError* error)
    {
        [self onProcessFailed:process
                    exception:[NSException exceptionWithName:error.localizedDescription
                                                      reason:error.localizedFailureReason
                                                    userInfo:error.userInfo]];
    }
}

- (void) shutDown
{
    self.delegate = nil;
    self.emitter = nil;
    [LCMicroService.serviceMap removeObjectForKey:self.instanceId];
    self.process = nil;
    self.eventListeners = nil;
}

- (void) onProcessAboutToExit:(LCProcess*)process exitCode:(int)code
{
    [[self.context globalObject] deleteProperty:@"require"];
    
    if(self.delegate && [self.delegate respondsToSelector:@selector(onExit:exitCode:)]) {
        [self.delegate onExit:self exitCode:code];
    }
}

- (void) onProcessExit:(LCProcess*)process exitCode:(int)code
{
    if(self.delegate && [self.delegate respondsToSelector:@selector(onExit:exitCode:)]) {
        [self.delegate onExit:self exitCode:code];
    }
    [self shutDown];
}

- (void) onProcessFailed:(LCProcess*)process exception:(NSException*)exception
{
    if(self.delegate && [self.delegate respondsToSelector:@selector(onError:exception:)]) {
        [self.delegate onError:self exception:exception];
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
            [self.emitter.value invokeMethod:@"on" withArguments:@[event, jsListener]];

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
                    [self.emitter.value invokeMethod:@"removeListener" withArguments:@[event, l[2]]];
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
            [self.emitter.value invokeMethod:@"emit" withArguments:@[event]];
        }];
    }
}

- (void) emitObject:(NSString*)event object:(id)object
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter.value invokeMethod:@"emit" withArguments:@[event, object]];
        }];
    }
}

- (void) emitNumber:(NSString*)event number:(NSNumber*)number
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter.value invokeMethod:@"emit" withArguments:@[event, number]];
        }];
    }
}

- (void) emitString:(NSString*)event string:(NSString*)string
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter.value invokeMethod:@"emit" withArguments:@[event, string]];
        }];
    }
}

- (void) emitBoolean:(NSString*)event boolean:(BOOL)boolean
{
    if (self.emitter) {
        [self.process async:^(JSContext* context) {
            [self.emitter.value invokeMethod:@"emit" withArguments:@[event, [JSValue valueWithBool:boolean
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
        self.process = [[LCProcess alloc] initWithDelegate:self id:serviceId_ mediaAccessMask:PermissionsRW];
    }
}

@end

@implementation LCAddOnFactory

static NSMutableDictionary* _factories = nil;
+ (NSMutableDictionary *)factories {
    if (_factories == nil) {
        _factories = [[NSMutableDictionary alloc] init];
    }
    return _factories;
}

+ (void) registerAddOnFactory:(NSString*)module factory:(LCAddOnFactory *)factory
{
    NSString *moduleName = [[module lastPathComponent] stringByDeletingPathExtension];
    [LCAddOnFactory.factories setObject:factory forKey:moduleName];
}

- (instancetype) init
{
    self = [super init];
    if (self) {
        
    }
    return self;
}

- (id<LCAddOn>) createInstance
{
    @throw ([NSException exceptionWithName:@"This method must be overridden"
                                    reason:@"Override this method to create an LCAddOn instance."
                                  userInfo:nil]);
}

@end
