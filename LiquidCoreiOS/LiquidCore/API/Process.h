//
//  Process.h
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

#ifndef Process_h
#define Process_h

#import <JavaScriptCore/JavaScriptCore.h>

@class Process;

@protocol ProcessDelegate <NSObject>
- (void) onProcessStart:(Process*)process context:(JSContext*)context;
- (void) onProcessAboutToExit:(Process*)process exitCode:(int)code;
- (void) onProcessExit:(Process*)process exitCode:(int)code;
- (void) onProcessFailed:(Process*)process exception:(NSException*)exception;
@end

typedef void (^ProcessThreadBlock)(JSContext*);

typedef enum _UninstallScope { LOCAL, GLOBAL } UninstallScope;
typedef enum _MediaAccessMask {
    PermissionsNone = 0,
    PermissionsRead = 1,
    PermissionsWrite = 2,
    PermissionsRW = 3
} MediaAccessMask;

@protocol LoopPreserver<NSObject>
- (void) letDie;
@end

@interface Process : NSObject
@property (nonatomic, readonly, copy) NSString* modulePath;

- (id) initWithDelegate:(id<ProcessDelegate>)delegate
                     id:(NSString*)uniqueID
        mediaAccessMask:(MediaAccessMask)mediaAccessMask;
- (void) addDelegate:(id<ProcessDelegate>)delegate;
- (void) removeDelegate:(id<ProcessDelegate>)delegate;
- (bool) isActive;
- (void) sync:(ProcessThreadBlock)block;
- (void) async:(ProcessThreadBlock)block;
- (void) exit:(int)code;
- (id<LoopPreserver>) keepAlive;
+ (void) uninstall:(NSString*)uniqueID scope:(UninstallScope)scope;
@end

#endif /* Node_h */
