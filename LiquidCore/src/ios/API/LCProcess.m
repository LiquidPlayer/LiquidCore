/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <Foundation/Foundation.h>
#import "LCProcess.h"
#import "node_bridge.h"
#import "FileSystem.h"

@interface BlockWrap : NSObject
@property (readonly) LCProcess* process;
@property (readonly) ProcessThreadBlock block;
- (id) init:(LCProcess*) process block:(ProcessThreadBlock)block;
@end

@implementation BlockWrap
- (id) init:(LCProcess *)process block:(ProcessThreadBlock)block
{
    self = [super init];
    if (self) {
        _block = block;
        _process = process;
    }
    return self;
}
@end

@interface LoopPreserver : NSObject<LoopPreserver>
@end

@implementation LoopPreserver {
    void *preserver;
    void *processRef_;
}
- (id) init:(void*)processRef
{
    self = [super init];
    if (self) {
        processRef_ = processRef;
        preserver = process_keep_alive(processRef);
    }
    return self;
}
- (void) letDie
{
    if (preserver) {
        process_let_die(processRef_, preserver);
    }
    preserver = NULL;
}
- (void)dealloc
{
    [self letDie];
}
@end

@interface LCProcess()
@property(atomic) bool active;
@property(atomic) JSContext* context;
@property(nonatomic, strong) JSContext* hold_;
@property(nonatomic, strong) NSHashTable *delegates;
- (void)onNodeStarted:(JSContext*)context group:(JSContextGroupRef)group;
- (void)onNodeExit:(int)code;
- (void)eventOnStart:(JSContext *)context;
- (void)eventOnAboutToExit:(int)code;
- (void)eventOnExit:(int)code;
- (void)eventOnProcessFailed:(JSValue *)exception;
@end

static void onNodeStarted(void *data, JSContextRef ctxRef, JSContextGroupRef ctxGroupRef)
{
    LCProcess *process = (__bridge LCProcess*) data;
    [process onNodeStarted:[JSContext contextWithJSGlobalContextRef:JSContextGetGlobalContext(ctxRef)]
                     group:ctxGroupRef];
}

static void onNodeExit(void *data, int code)
{
    LCProcess *process = (__bridge_transfer LCProcess*) data;
    [process onNodeExit:code];
}

@implementation LCProcess {
    NSString* uniqueID_;
    MediaAccessMask mediaAccessMask_;
    void *processRef_;
    bool notified_exit_;
    FileSystem *fs_;
    NSThread *jsThread_;
}

- (id) initWithDelegate:(id<LCProcessDelegate>)delegate
                     id:(NSString*)uniqueID
        mediaAccessMask:(MediaAccessMask)mediaAccessMask;
{
    self = [super init];
    if (self) {
        uniqueID_ = uniqueID;
        mediaAccessMask_ = mediaAccessMask;
        _active = false;
        _delegates = [NSHashTable weakObjectsHashTable];
        [_delegates addObject:delegate];
        processRef_ = process_start(onNodeStarted, onNodeExit, (__bridge_retained void *)self);
        jsThread_ = [[NSThread alloc] initWithTarget:self
                                            selector:@selector(jsThreadMain:)
                                              object:nil];
        [jsThread_ start];  // Actually create the thread
        _context = nil;
        notified_exit_ = false;
        fs_ = nil;
    }
    return self;
}

- (void) jsThreadMain:(id)arg
{
    process_run_in_thread(self->processRef_);
    self.context = nil;
    self.delegates = nil;
    jsThread_ = nil;
}

- (void) addDelegate:(id<LCProcessDelegate>)delegate
{
    if (![self.delegates containsObject:delegate]) {
        [self.delegates addObject:delegate];
    }
    if (self.active) {
        [self async:^(JSContext* context) {
            [delegate onProcessStart:self context:context];
        }];
    }
}

- (void) removeDelegate:(id<LCProcessDelegate>)delegate
{
    [self.delegates removeObject:delegate];
}

- (void)onNodeStarted:(JSContext*)context
                group:(JSContextGroupRef)group
{
    [self setActive:true];
    [self setContext:context];
    
    [self setHold_:context];
    
    context[@"__nodedroid_onLoad"] = ^() {
        if ([self active]) {
            [[self.context globalObject] deleteProperty:@"__nodedroid_onLoad"];
            
            // set the filesystem
            self->fs_ = [FileSystem createInContext:self.context uniqueID:self->uniqueID_ mediaAccessMask:self->mediaAccessMask_];
            JSValue *o = [JSValue valueWithNewObjectInContext:self.context];
            o[@"fs"] = self->fs_;
            process_set_filesystem([self.context JSGlobalContextRef], (JSObjectRef)[o[@"fs"] JSValueRef]);
            self->_modulePath = self->fs_.modulePath;
            self->_node_modulesPath = self->fs_.node_modulesPath;
            
            // set the exit handler
            __weak LCProcess* weakSelf = self;
            self.context[@"__tmp"] = [JSValue valueWithObject:^(int code) {
                [weakSelf eventOnAboutToExit:code];
            } inContext:[self context]];
            [[self context] evaluateScript:@"process.on('exit',__tmp);"];
            
            // set unhandled exception handler
            [self context][@"__tmp"] = [JSValue valueWithObject:^(JSValue *error) {
                NSLog(@"There is an unhandled exception!");
                NSLog(@"Error: %@", [error toString]);
                NSLog(@"Stack: %@", [error[@"stack"] toString]);
                [weakSelf eventOnProcessFailed:error];
            } inContext:[self context]];
            [[self context] evaluateScript:@"process.on('uncaughtException',__tmp);"];
            [[[self context] globalObject] deleteProperty:@"__tmp"];
            
            // intercept stdout and stderr
            JSValue *stdout = [self context][@"process"][@"stdout"];
            JSValue *stderr = [self context][@"process"][@"stderr"];
            stdout[@"write"] = ^(NSString *msg) {
                NSLog(@"stdout: %@", msg);
            };
            stderr[@"write"] = ^(NSString *msg) {
                NSLog(@"stderr: %@", msg);
            };
            
            // Remove global.v8 --> V8 doesn't actually exist on this platform
            [[self.context globalObject] deleteProperty:@"v8"];
            
            // Set correct 'process.config' values
#if TARGET_OS_SIMULATOR
            const NSString *arch = @"x64";
#else
#if __LP64__
            const NSString *arch = @"arm64";
#else
            const NSString *arch = @"arm";
            [self context][@"process"][@"config"][@"variables"][@"arm_float_abi"] = @"default";
            [self context][@"process"][@"config"][@"variables"][@"arm_fpu"] = @"vfpv3";
            [self context][@"process"][@"config"][@"variables"][@"arm_thumb"] = @(0);
            [self context][@"process"][@"config"][@"variables"][@"arm_version"] = @"7";

#endif
#endif
            [self context][@"process"][@"config"][@"variables"][@"host_arch"] = arch;
            [self context][@"process"][@"config"][@"variables"][@"target_arch"] = arch;
            [[self context][@"process"] deleteProperty:@"arch"];
            [self context][@"process"][@"arch"] = arch;

            // Remove v8 version, since it is only the V8 API
            [[self context][@"process"][@"versions"] deleteProperty:@"v8"];
            
            // Expose JavaScriptCore version
            NSString *version = [[[NSBundle bundleForClass:JSValue.class
                                   ] infoDictionary] objectForKey:@"CFBundleVersion"];
            [self context][@"process"][@"versions"][@"jsc"] = version;

            // Expose LiquidCore version
            NSString *lcversion = [[[NSBundle bundleForClass:LCProcess.class] infoDictionary]
                                   objectForKey:@"CFBundleShortVersionString"];
            [self context][@"process"][@"versions"][@"liquidcore"] = lcversion;

            [self eventOnStart:[self context]];
            [self setHold_:nil];
        }
    };
}

- (void)onNodeExit:(int)code
{
    [self setActive:false];
    [self eventOnExit:code];
    //process_dispose(self->processRef_);
}

- (void)eventOnStart:(JSContext *)context
{
    for (NSObject<LCProcessDelegate>* delegate in self.delegates) {
        [delegate onProcessStart:self context:context];
    }
}

- (void)eventOnAboutToExit:(int)code
{
    for (NSObject<LCProcessDelegate>* delegate in self.delegates) {
        [delegate onProcessAboutToExit:self exitCode:code];
    }
}

- (void)eventOnExit:(int)code
{
    if (!notified_exit_) {
        notified_exit_ = true;
        for (NSObject<LCProcessDelegate>* delegate in self.delegates) {
            [delegate onProcessExit:self exitCode:code];
        }
        
        // clean up filesystem
        if (fs_) {
            [fs_ cleanUp];
            fs_ = nil;
        }
    }
}

- (void)eventOnProcessFailed:(JSValue *)exception
{
    for (NSObject<LCProcessDelegate>* delegate in self.delegates) {
        [delegate onProcessFailed:self
                        exception:[NSException exceptionWithName:@"JavaScript exception"
                                                          reason:[exception toString]
                                                        userInfo:nil]];
    }
}

- (bool) isActive
{
    return [self active];
}

static void callback_(void *data)
{
    BlockWrap *wrap = (__bridge_transfer BlockWrap *)data;
    ProcessThreadBlock block = [wrap block];
    JSContext *context = [[wrap process] context];
    block(context);
}

- (void) sync:(void (^)(JSContext*))block
{
    BlockWrap *wrap = [[BlockWrap alloc] init:self block:block];
    process_sync(processRef_, callback_, (__bridge_retained void*)wrap);
}

- (void) async:(void (^)(JSContext*))block
{
    BlockWrap *wrap = [[BlockWrap alloc] init:self block:block];
    process_async(processRef_, callback_, (__bridge_retained void*)wrap);
}

- (void) exit:(int)code
{
    if ([self active]) {
        [self async:^(JSContext *ctx) {
            if (ctx) {
                [ctx evaluateScript:[NSString stringWithFormat:@"process.exit(%d)", code]];
            }
        }];
    }
}

- (id<LoopPreserver>) keepAlive
{
    return [[LoopPreserver alloc] init:processRef_];
}

- (void) exposeHostDirectory:(NSString * _Nonnull)dir
             mediaAccessMask:(MediaAccessMask)mediaAccessMask
{
    NSString *trimmed = [dir stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    if (![[trimmed substringWithRange:NSMakeRange(0, 1)] isEqualToString:@"/"]) {
        @throw [NSException exceptionWithName:@"IO Error"
                                       reason:@"Exposed directory must be absolute (starts with '/')"
                                     userInfo:nil];
    }
    if ([trimmed isEqualToString:@"/"]) {
        @throw [NSException exceptionWithName:@"IO Error"
                                       reason:@"Exposed directory must not be root"
                                     userInfo:nil];
    }
    if ([trimmed containsString:@".."]) {
        @throw [NSException exceptionWithName:@"IO Error"
                                       reason:@"Exposed directory must be absolute (no '..')"
                                     userInfo:nil];
    }
    if ([[trimmed substringWithRange:NSMakeRange(0, 5)] isEqualToString:@"/home"]) {
        @throw [NSException exceptionWithName:@"IO Error"
                                       reason:@"Cannot override /home"
                                     userInfo:nil];
    }
    NSString *path = [dir stringByResolvingSymlinksInPath];
    expose_host_directory([self.context JSGlobalContextRef],
                          [path cStringUsingEncoding:NSUTF8StringEncoding],
                          mediaAccessMask);
}


+ (void) uninstall:(NSString*)uniqueID scope:(UninstallScope)scope
{
    [FileSystem uninstallLocal:uniqueID];
}

@end
