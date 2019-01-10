/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <JavaScriptCore/JavaScriptCore.h>

@class LCProcess;

@protocol LCProcessDelegate <NSObject>
- (void) onProcessStart:(LCProcess*)process context:(JSContext*)context;
- (void) onProcessAboutToExit:(LCProcess*)process exitCode:(int)code;
- (void) onProcessExit:(LCProcess*)process exitCode:(int)code;
- (void) onProcessFailed:(LCProcess*)process exception:(NSException*)exception;
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

@interface LCProcess : NSObject
@property (nonatomic, readonly, copy) NSString* modulePath;

- (id) initWithDelegate:(id<LCProcessDelegate>)delegate
                     id:(NSString*)uniqueID
        mediaAccessMask:(MediaAccessMask)mediaAccessMask;
- (void) addDelegate:(id<LCProcessDelegate>)delegate;
- (void) removeDelegate:(id<LCProcessDelegate>)delegate;
- (bool) isActive;
- (void) sync:(ProcessThreadBlock)block;
- (void) async:(ProcessThreadBlock)block;
- (void) exit:(int)code;
- (id<LoopPreserver>) keepAlive;
+ (void) uninstall:(NSString*)uniqueID scope:(UninstallScope)scope;
@end
