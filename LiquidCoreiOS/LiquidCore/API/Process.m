//
//  Process.m
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
#import "Process.h"
#import "NodeBridge.h"
#import "FileSystem.h"
#import <mach-o/dyld.h>

extern int jsc_get_major_version (void);
extern int jsc_get_minor_version (void);
extern int jsc_get_micro_version (void);

@interface BlockWrap : NSObject
@property (readonly) Process* process;
@property (readonly) ProcessThreadBlock block;
- (id) init:(Process*) process block:(ProcessThreadBlock)block;
@end

@implementation BlockWrap
- (id) init:(Process *)process block:(ProcessThreadBlock)block
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

@interface Process()
@property(atomic) bool active;
@property(atomic, weak) JSContext* context;
@property(nonatomic, strong) JSContext* hold_;
- (void)onNodeStarted:(JSContext*)context group:(JSContextGroupRef)group;
- (void)onNodeExit:(int)code;
- (void)eventOnStart:(JSContext *)context;
- (void)eventOnAboutToExit:(int)code;
- (void)eventOnExit:(int)code;
- (void)eventOnProcessFailed:(JSValue *)exception;
@end

static void onNodeStarted(void *data, JSContextRef ctxRef, JSContextGroupRef ctxGroupRef)
{
    Process *process = (__bridge Process*) data;
    [process onNodeStarted:[JSContext contextWithJSGlobalContextRef:JSContextGetGlobalContext(ctxRef)]
                     group:ctxGroupRef];
}

static void onNodeExit(void *data, int code)
{
    Process *process = (__bridge_transfer Process*) data;
    [process onNodeExit:code];
}

@implementation Process {
    NSString* uniqueID_;
    MediaAccessMask mediaAccessMask_;
    NSMutableArray *delegates_;
    void *processRef_;
    bool notified_exit_;
    FileSystem *fs_;
}

- (id) initWithDelegate:(id<ProcessDelegate>)delegate
                     id:(NSString*)uniqueID
        mediaAccessMask:(MediaAccessMask)mediaAccessMask;
{
    self = [super init];
    if (self) {
        uniqueID_ = uniqueID;
        mediaAccessMask_ = mediaAccessMask;
        _active = false;
        delegates_ = [[NSMutableArray alloc] init];
        [delegates_ addObject:delegate];
        processRef_ = process_start(onNodeStarted, onNodeExit, (__bridge_retained void *)self);
        _context = nil;
        notified_exit_ = false;
        fs_ = nil;
    }
    return self;
}

- (void) addDelegate:(id<ProcessDelegate>)delegate
{
    if (![delegates_ containsObject:delegate]) {
        [delegates_ addObject:delegate];
    }
    if (self.active) {
        [self async:^(JSContext* context) {
            [delegate onProcessStart:self context:context];
        }];
    }
}

- (void) removeDelegate:(id<ProcessDelegate>)delegate
{
    [delegates_ removeObject:delegate];
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
            
            // set the exit handler
            self.context[@"__tmp"] = [JSValue valueWithObject:^(int code) {
                [self eventOnAboutToExit:code];
            } inContext:[self context]];
            [[self context] evaluateScript:@"process.on('exit',__tmp); __tmp = undefined;"];
            
            // set unhandled exception handler
            [self context][@"__tmp"] = [JSValue valueWithObject:^(JSValue *error) {
                NSLog(@"There is an unhandled exception!");
                NSLog(@"Error: %@", [error toString]);
                NSLog(@"Stack: %@", [error[@"stack"] toString]);
                [self eventOnProcessFailed:error];
            } inContext:[self context]];
            [[self context] evaluateScript:@"process.on('uncaughtException',__tmp); __tmp = undefined;"];
            
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
            
            // Remove v8 version, since it is only the V8 API
            [[self context][@"process"][@"versions"] deleteProperty:@"v8"];
            
            // Expose JavaScriptCore version
            NSString *version = [[[NSBundle bundleForClass:JSValue.class
                                   ] infoDictionary] objectForKey:@"CFBundleVersion"];
            [self context][@"process"][@"versions"][@"jsc"] = version;

            // Expose LiquidCore version
            NSString *lcversion = [[[NSBundle bundleForClass:Process.class] infoDictionary]
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
    [self setContext:nil];
    [self eventOnExit:code];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        process_dispose(self->processRef_);
    });
}

- (void)eventOnStart:(JSContext *)context
{
    for (NSObject<ProcessDelegate>* delegate in delegates_) {
        [delegate onProcessStart:self context:context];
    }
}

- (void)eventOnAboutToExit:(int)code
{
    for (NSObject<ProcessDelegate>* delegate in delegates_) {
        [delegate onProcessAboutToExit:self exitCode:code];
    }
}

- (void)eventOnExit:(int)code
{
    if (!notified_exit_) {
        notified_exit_ = true;
        for (NSObject<ProcessDelegate>* delegate in delegates_) {
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
    for (NSObject<ProcessDelegate>* delegate in delegates_) {
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
            if ([self active] && ctx) {
                [ctx evaluateScript:[NSString stringWithFormat:@"process.exit(%d)", code]];
            }
        }];
    }
}

- (id<LoopPreserver>) keepAlive
{
    return [[LoopPreserver alloc] init:processRef_];
}

+ (void) uninstall:(NSString*)uniqueID scope:(UninstallScope)scope
{
    [FileSystem uninstallLocal:uniqueID];
}

@end
