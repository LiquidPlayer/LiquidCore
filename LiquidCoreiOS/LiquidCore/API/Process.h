/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
