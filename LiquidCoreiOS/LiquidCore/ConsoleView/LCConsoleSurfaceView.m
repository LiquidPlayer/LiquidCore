//
//  LCConsoleSurfaceView.m
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

#import "LCConsoleSurfaceView.h"
#import "Process.h"

@interface ConsoleSession : NSObject <ProcessDelegate>
@property (nonatomic) LCConsoleSurfaceView *currentView;

- (id) initWithMicroService:(LCMicroService*)service onAttached:(LCOnSuccessHandler)onAttached;
- (void) processCommand:(NSString*)cmd;
- (void) resize:(int)rows columns:(int)cols;
@end

@implementation ConsoleSession {
    LCOnSuccessHandler onAttached_;
    Process* process_;
    NSString* uuid_;
    NSInteger rows_;
    NSInteger columns_;
    BOOL processedException_;
}

- (id) initWithMicroService:(LCMicroService*)service onAttached:(LCOnSuccessHandler)onAttached
{
    self = [super init];
    if (self) {
        onAttached_ = onAttached;
        process_ = service.process;
        [process_ addDelegate:self];
        uuid_ = service.instanceId;
    }
    return self;
}

- (void) onProcessStart:(Process*)process context:(JSContext*)context
{
    JSValue *stdout = context[@"process"][@"stdout"];
    JSValue *stderr = context[@"process"][@"stderr"];
    [self setupStream:stdout onWrite:^(NSString *string) {
        if (self.currentView != nil) {
            [self.currentView.console print:string];
        }
    }];
    [self setupStream:stderr onWrite:^(NSString *string) {
        if (self.currentView != nil) {
            // Make it red!
            NSString *display = [NSString stringWithFormat:@"%C[31m%@", 0x001B, string];
            [self.currentView.console print:display];
        }
    }];
    context.exceptionHandler = ^(JSContext* context, JSValue *exception) {
        self->processedException_ = YES;
        if (self.currentView != nil) {
            NSString *display = [NSString stringWithFormat:@"%C[31m%@\n%@", 0x001B, [exception toString], exception[@"stack"]];
            [self.currentView.console println:display];
        }
    };
    if (onAttached_ != nil) {
        onAttached_();
        onAttached_ = nil;
    }
}

- (void) processCommand:(NSString*)cmd
{
    processedException_ = NO;
    if (process_ != nil) {
        [process_ async:^(JSContext *context) {
            JSValue *output = [context evaluateScript:cmd];
            if (!self->processedException_) {
                [context[@"console"] invokeMethod:@"log" withArguments:@[output]];
            }
        }];
    }
}

- (void) resize:(int)rows columns:(int)cols
{
    if (process_ != nil && (rows != rows_ || columns_ != cols)) {
        [process_ async:^(JSContext *context) {
            self->rows_ = rows;
            self->columns_ = cols;
            context[@"process"][@"stdout"][@"rows"] = @(rows);
            context[@"process"][@"stderr"][@"rows"] = @(rows);
            context[@"process"][@"stdout"][@"columns"] = @(cols);
            context[@"process"][@"stderr"][@"columns"] = @(cols);
        }];
    }
}

- (void) detach
{
    if (process_ != nil && [process_ isActive]) {
        [process_ async:^(JSContext *context){
            JSValue *stdout = context[@"process"][@"stdout"];
            JSValue *stderr = context[@"process"][@"stderr"];
            [self teardownStream:stdout];
            [self teardownStream:stderr];
        }];
    }

    if (process_ != nil) {
        [process_ removeDelegate:self];
    }
    process_ = nil;
}

- (void) setupStream:(JSValue*)stream onWrite:(id)onWrite
{
    JSContext* context = [stream context];
    stream[@"write"] = onWrite;
    stream[@"clearScreenDown"] = [context evaluateScript:@"new Function(\"this.write('\x1b[0J');\");"];
    stream[@"moveCursor"] = [context evaluateScript:
                             @"new Function('c', 'r', \""
                             @"var out = ''; c = c || 0; r = r || 0;"
                             @"if (c>0) out += '\\x1b['+c+'C'; else if (c<0) out+='\\x1b['+(-c)+'D';"
                             @"if (r>0) out += '\\x1b['+r+'B'; else if (r<0) out+='\\x1b['+(-r)+'A';"
                             @"this.write(out);"
                             @"\");"];
    stream[@"rows"] = @(rows_);
    stream[@"columns"] = @(columns_);
}

- (void) teardownStream:(JSValue*)stream
{
    // FIXME: restore previous
    JSContext* context = [stream context];
    stream[@"write"] = [context evaluateScript:@"new Function()"];
    [stream deleteProperty:@"clearScreenDown"];
    [stream deleteProperty:@"moveCursor"];
}

- (void) onProcessAboutToExit:(Process*)process exitCode:(int)code
{
    if (self.currentView != nil) {
        NSString *display = [NSString stringWithFormat:@"%C[31mProcess about to exit with code %d", 0x001B, code];
        [self.currentView.console println:display];
    }
    NSLog(@"onProcessAboutToExit: Process about to exit with code %d", code);
    [self detach];
}

- (void) onProcessExit:(Process*)process exitCode:(int)code
{
    NSLog(@"onProcessExit: Process exited with code %d", code);
    if (self.currentView != nil) {
        NSString *display = [NSString stringWithFormat:@"%C[31mProcess exited with code %d", 0x001B, code];
        [self.currentView.console println:display];
    }
    process_ = nil;
}

- (void) onProcessFailed:(Process*)process exception:(NSException*)exception
{
    
}

@end

@implementation LCConsoleSurfaceView {
    ConsoleSession *session_;
    NSString *uuid_;
}

+ (NSString*) SURFACE_CANONICAL_NAME
{
    return @"org.liquidplayer.surface.console.ConsoleSurface";
}

+ (NSString*) SURFACE_VERSION
{
    NSDictionary *infoDictionary = [[NSBundle bundleForClass:LCConsoleSurfaceView.class]infoDictionary];
    NSString *version = [infoDictionary objectForKey:@"CFBundleShortVersionString"];

    return version;
}

- (id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
}

#pragma - LCSurface

- (void) bind:(LCMicroService*)service
       export:(JSValue *)exportObject
       config:(JSValue *)config
      onBound:(LCOnSuccessHandler)onBound
      onError:(LCOnFailHandler)onError;
{
    // Nothing to do -- everything happens during attach:onAttached:onError:
    onBound();
}

- (UIView<LCSurface>*) attach:(LCMicroService*)service
                   onAttached:(LCOnSuccessHandler)onAttachedHandler
                      onError:(LCOnFailHandler)onError;
{
    session_ = [[ConsoleSession alloc] initWithMicroService:service onAttached:onAttachedHandler];
    session_.currentView = self;
    uuid_ = service.instanceId;
    return self;
}

- (void) detach
{
    if (session_ != nil) {
        [session_ detach];
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        self.command.enabled = NO;
        self.upHistory.enabled = NO;
        self.downHistory.enabled = NO;
    });
}

// override
- (void) processCommand:(NSString *)cmd
{
    [session_ processCommand:cmd];
}

// override
- (void) resize:(int)rows columns:(int)cols
{
    [session_ resize:rows columns:cols];
}

- (void) reset
{
    
}

@end

__attribute__((constructor))
void myStaticInitMethod()
{
    [LCSurfaceRegistration registerSurface:LCConsoleSurfaceView.class];
}
