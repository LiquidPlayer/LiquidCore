//
//  MicroServiceTests.m
//  LiquidCoreiOSTests
//
//  Created by Eric Lange on 7/15/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>
#import "MicroService.h"
#import "Process.h"

@interface MicroServiceTest : XCTestCase <MicroServiceDelegate, MicroServiceEventListener>
- (void) onStart:(MicroService *)service synchronizer:(Synchronizer *)synchronizer;

@property (atomic, assign) bool serverReady;
@property (atomic, assign) int finishCount;
@end

@implementation MicroServiceTest {
    NSInteger port_;
    MicroService *server_;
    MicroService *client_;
}
- (void) testMicroService
{
    _serverReady = false;
    _finishCount = 2;

    NSBundle *testBundle = [NSBundle bundleForClass:[self class]];
    NSURL *server_js = [testBundle URLForResource:@"Resources/server" withExtension:@"js"];
    
    // First, start a MicroService from a file.  This service creates a small HTTP file server
    server_ = [[MicroService alloc] initWithURL:server_js delegate:self];
    [server_ start];
    volatile bool serverReady = self.serverReady; while (!serverReady) { serverReady = self.serverReady; }
    
    NSURL *client_js = [NSURL URLWithString:[NSString stringWithFormat:@"http://localhost:%ld/hello.js", port_]];
    
    // Next, start a MicroService that is served from the server
    client_ = [[MicroService alloc] initWithURL:client_js delegate:self];
    [client_ start];
    
    volatile int finishCount = self.finishCount; while (finishCount>0) { finishCount = self.finishCount; }
    
    [MicroService uninstall:server_js];
    [MicroService uninstall:client_js];
}

- (void) onStart:(MicroService *)service synchronizer:(Synchronizer *)synchronizer
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
- (void) onError:(MicroService *)service exception:(NSException *)exception
{
    XCTAssertTrue(false);
}
- (void) onExit:(MicroService *)service exitCode:(int)exitCode
{
    XCTAssertEqual(exitCode, 0);
    self.finishCount --;
    if ([service isEqual:client_]) {
        [server_.process exit:0];
    }
}

- (void) onEvent:(MicroService *)service event:(NSString *)event payload:(id)payload
{
    // Server
    if ([event isEqualToString:@"listening"]) {
        XCTAssertEqualObjects(service, server_);
        XCTAssertTrue([[payload class] isKindOfClass:[NSDictionary class]]);
        NSNumber *p = payload[@"port"];
        port_ = [p integerValue];
        self.serverReady = true;
        return;
    }
    
    // Client
    XCTAssertEqualObjects(service, client_);
    if ([event isEqualToString:@"msg"]) {
        XCTAssertEqualObjects(@"Hello, World", payload[@"string"]);
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
