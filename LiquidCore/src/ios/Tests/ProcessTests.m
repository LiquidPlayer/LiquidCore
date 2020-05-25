//
//  ProcessTests.m
//  LiquidCoreiOSTests
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

#import <XCTest/XCTest.h>
#import <LiquidCore/LiquidCore.h>
#import <JavaScriptCore/JavaScriptCore.h>

@interface ProcessTests : XCTestCase

@end

@interface MultiLCProcessDelegate : XCTestCase<LCProcessDelegate>
@property (atomic, assign) int count;
@property (atomic, assign) int countdown;
@end
@implementation MultiLCProcessDelegate
- (void) onProcessStart:(LCProcess*)process context:(JSContext*)context
{
    XCTAssertTrue([process isActive]);
    
    JSValue* function = [JSValue valueWithObject:^(int in) {
        return in + 1;
    } inContext:context];
    int mycount = self.count++;
    JSValue* incd = [function callWithArguments:@[[NSNumber numberWithInt:mycount]]];
    XCTAssertEqual([incd toInt32], mycount + 1);
}
- (void) onProcessAboutToExit:(LCProcess*)process exitCode:(int)code {}
- (void) onProcessExit:(LCProcess*)process exitCode:(int)code
{
    XCTAssertFalse([process isActive]);
    self.countdown --;
}
- (void) onProcessFailed:(LCProcess*)process exception:(NSException*)exception
{
    XCTAssertTrue(false);
}
@end

@interface MultiThreadDelegate : XCTestCase<LCProcessDelegate>
@property (atomic, assign) bool done;
@end
@implementation MultiThreadDelegate
- (void) onProcessStart:(LCProcess*)process context:(JSContext*)ctx
{
    // Don't let the process exit until our thread finishes
    id<LoopPreserver> preserver = [process keepAlive];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [process sync:^(JSContext *context) {
            context[@"foo"] = @"bar";
            JSValue *value = [context evaluateScript:@"5 + 10"];
            XCTAssertEqual([value toInt32], 15);
            XCTAssertEqualObjects([context[@"foo"] toString], @"bar");
            context[@"dir_contents"] = ^(JSValue *err, NSArray * files) {
                NSLog(@"dir_contents = %@", files);
            };
            
            XCTAssertEqual(context, process.context);
            
            [context evaluateScript:
                                   @"(function() {"
                                   @"  var fs = require('fs');"
                                   @"  fs.readdir('/home/node_modules/sqlite3',dir_contents);"
                                   @"})();"];
        }];
        // ok, we're done here
        [preserver letDie];
    });
}
- (void) onProcessAboutToExit:(LCProcess*)process exitCode:(int)code {}
- (void) onProcessExit:(LCProcess*)process exitCode:(int)code
{
    self.done = true;
}
- (void) onProcessFailed:(LCProcess*)process exception:(NSException*)exception
{
    XCTAssertTrue(false);
}
@end

@interface ForceExitDelegate : XCTestCase<LCProcessDelegate>
@property (atomic, assign) bool done;
@end
@implementation ForceExitDelegate
- (void) onProcessStart:(LCProcess*)process context:(JSContext*)context
{
    [context evaluateScript:@"setInterval(function(){console.log('tick');},1000);"];
    [context evaluateScript:@"setTimeout(function(){process.exit(2);},500);"];
}
- (void) onProcessAboutToExit:(LCProcess*)process exitCode:(int)code {}
- (void) onProcessExit:(LCProcess*)process exitCode:(int)code
{
    XCTAssertEqual(code, 2);
    self.done = true;
}
- (void) onProcessFailed:(LCProcess*)process exception:(NSException*)exception
{
    XCTAssertTrue(false);
}
@end

@implementation ProcessTests

- (void)testMultiProcess
{
    MultiLCProcessDelegate* delegate = [[MultiLCProcessDelegate alloc] init];
    delegate.countdown = 3;
    XCTAssertNotNil([[LCProcess alloc] initWithDelegate:delegate id:@"_1" mediaAccessMask:PermissionsRW]);
    XCTAssertNotNil([[LCProcess alloc] initWithDelegate:delegate id:@"_2" mediaAccessMask:PermissionsRW]);
    XCTAssertNotNil([[LCProcess alloc] initWithDelegate:delegate id:@"_3" mediaAccessMask:PermissionsRW]);
    volatile int countdown = 100;
    while (countdown > 0) {
        countdown = delegate.countdown;
    }
}

- (void)testMultiThread
{
    MultiThreadDelegate *delegate = [[MultiThreadDelegate alloc] init];
    delegate.done = false;
    XCTAssertNotNil([[LCProcess alloc] initWithDelegate:delegate id:@"_" mediaAccessMask:PermissionsRW]);
    volatile bool done = false;
    while (!done) {
        done = delegate.done;
    }
}

- (void)testForceExit
{
    ForceExitDelegate *delegate = [[ForceExitDelegate alloc] init];
    delegate.done = false;
    XCTAssertNotNil([[LCProcess alloc] initWithDelegate:delegate id:@"forceExitTest" mediaAccessMask:PermissionsRW]);
    volatile bool done = false;
    while (!done) {
        done = delegate.done;
    }
    
    [LCProcess uninstall:@"forceExitTest" scope:GLOBAL];
}
@end
