//
//  MicroServiceTests.m
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

#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>
#import "LCMicroService.h"
#import "Process.h"

@interface MicroServiceTest : XCTestCase <LCMicroServiceDelegate, LCMicroServiceEventListener>
- (void) onStart:(LCMicroService *)service;

@property (atomic, assign) bool serverReady;
@property (atomic, assign) int finishCount;
@end

@implementation MicroServiceTest {
    NSInteger port_;
    LCMicroService *server_;
    LCMicroService *client_;
}
- (void) testMicroService
{
    _serverReady = false;
    _finishCount = 2;

    NSBundle *testBundle = [NSBundle bundleForClass:[self class]];
    NSURL *server_js = [testBundle URLForResource:@"Resources/server" withExtension:@"js"];
    
    // First, start a MicroService from a file.  This service creates a small HTTP file server
    server_ = [[LCMicroService alloc] initWithURL:server_js delegate:self];
    [server_ start];
    volatile bool serverReady = self.serverReady; while (!serverReady) { serverReady = self.serverReady; }
    
    NSURL *client_js = [NSURL URLWithString:[NSString stringWithFormat:@"http://localhost:%ld/hello.js", port_]];
    
    // Next, start a MicroService that is served from the server
    client_ = [[LCMicroService alloc] initWithURL:client_js delegate:self];
    [client_ start];
    
    volatile int finishCount = self.finishCount; while (finishCount>0) { finishCount = self.finishCount; }
    
    [LCMicroService uninstall:server_js];
    [LCMicroService uninstall:client_js];
}

- (void) onStart:(LCMicroService *)service
{
    if ([service isEqual:server_]) {
        [service addEventListener:@"listening" listener:self];
    } else {
        XCTAssertEqualObjects(service, client_);
        [service addEventListener:@"msg" listener:self];
        [service addEventListener:@"null" listener:self];
        [service addEventListener:@"number" listener:self];
        [service addEventListener:@"string" listener:self];
        [service addEventListener:@"boolean" listener:self];
        [service addEventListener:@"array" listener:self];
    }
}
- (void) onError:(LCMicroService *)service exception:(NSException *)exception
{
    XCTAssertTrue(false);
}
- (void) onExit:(LCMicroService *)service exitCode:(int)exitCode
{
    XCTAssertEqual(exitCode, 0);
    self.finishCount --;
    if ([service isEqual:client_]) {
        [server_.process exit:0];
    }
}

- (void) onEvent:(LCMicroService *)service event:(NSString *)event payload:(id)payload
{
    // Server
    if ([event isEqualToString:@"listening"]) {
        XCTAssertEqualObjects(service, server_);
        NSNumber *p = payload[@"port"];
        port_ = [p integerValue];
        self.serverReady = true;
        return;
    }
    
    // Client
    XCTAssertEqualObjects(service, client_);
    if ([event isEqualToString:@"msg"]) {
        XCTAssertEqualObjects(@"Hello, World!", payload[@"msg"]);
        [service emitObject:@"js_msg" object:@{ @"msg" : @"Hallo die Weld!" }];
    } else if ([event isEqualToString:@"null"]) {
        XCTAssertNil(payload);
        [service emit:@"js_null"];
    } else if ([event isEqualToString:@"number"]) {
        XCTAssertEqualObjects(@5.2, payload);
        [service emitNumber:@"js_number" number:@2.5];
    } else if ([event isEqualToString:@"string"]) {
        XCTAssertEqualObjects(@"foo", payload);
        [service emitString:@"js_string" string:@"bar"];
    } else if ([event isEqualToString:@"boolean"]) {
        XCTAssertTrue([payload boolValue]);
        [service emitBoolean:@"js_boolean" boolean:false];
    } else if ([event isEqualToString:@"array"]) {
        NSArray *array = (NSArray*)payload;
        XCTAssertEqualObjects(array[0], @1);
        XCTAssertEqualObjects(array[1], @"two");
        XCTAssertEqualObjects(array[2], [NSNumber numberWithBool:true]);
        XCTAssertEqualObjects(array[3][@"str"], @"bar");
        [service emitObject:@"js_array" object:@[@5]];
    }
}

@end
