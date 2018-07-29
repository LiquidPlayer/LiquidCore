//
//  MicroService.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/14/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

@class LCMicroService;
@class Process;

@interface LCSynchronizer : NSObject
- (void) enter;
- (void) exit;
@end

@protocol LCMicroServiceDelegate <NSObject>
@optional - (void) onStart:(LCMicroService*)service synchronizer:(LCSynchronizer*)synchronizer;
@optional - (void) onExit:(LCMicroService*)service exitCode:(int)exitCode;
@optional - (void) onError:(LCMicroService*)service exception:(NSException*)exception;
@end

@protocol LCMicroServiceEventListener <NSObject>
- (void) onEvent:(LCMicroService*)service event:(NSString*)event payload:(id)payload;
@end

@interface LCMicroService : NSObject
@property (nonatomic, readonly, copy) NSString* instanceId;
@property (atomic, readonly) Process* process;
@property (nonatomic, readwrite, copy) NSArray* availableSurfaces;

+ (id) serviceFromInstanceId:(NSString*)instanceId;
+ (void) uninstall:(NSURL *)serviceURI;

- (id) initWithURL:(NSURL*)serviceURI;
- (id) initWithURL:(NSURL*)serviceURI delegate:(id<LCMicroServiceDelegate>)delegate;

- (void) addEventListener:(NSString*)event listener:(id<LCMicroServiceEventListener>)listener;
- (void) removeEventListener:(NSString*)event listener:(id<LCMicroServiceEventListener>)listener;

- (void) emit:(NSString*)event;
- (void) emitObject:(NSString*)event object:(id)object;
- (void) emitNumber:(NSString*)event number:(NSNumber*)number;
- (void) emitString:(NSString*)event string:(NSString*)string;
- (void) emitBoolean:(NSString*)event boolean:(bool)boolean;

- (void) start;
- (void) startWithArguments:(NSArray*)argv;

@end
