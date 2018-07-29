//
//  MicroService.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/14/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef MicroService_h
#define MicroService_h

@class MicroService;
@class Process;

@interface Synchronizer : NSObject
- (void) enter;
- (void) exit;
@end

@protocol MicroServiceDelegate <NSObject>
@optional - (void) onStart:(MicroService*)service synchronizer:(Synchronizer*)synchronizer;
@optional - (void) onExit:(MicroService*)service exitCode:(int)exitCode;
@optional - (void) onError:(MicroService*)service exception:(NSException*)exception;
@end

@protocol MicroServiceEventListener <NSObject>
- (void) onEvent:(MicroService*)service event:(NSString*)event payload:(id)payload;
@end

@interface MicroService : NSObject
@property (nonatomic, readonly, copy) NSString* instanceId;
@property (atomic, readonly) Process* process;
@property (nonatomic, readwrite, copy) NSArray* availableSurfaces;

+ (id) serviceFromInstanceId:(NSString*)instanceId;
+ (void) uninstall:(NSURL *)serviceURI;

- (id) initWithURL:(NSURL*)serviceURI;
- (id) initWithURL:(NSURL*)serviceURI delegate:(id<MicroServiceDelegate>)delegate;

- (void) addEventListener:(NSString*)event listener:(id<MicroServiceEventListener>)listener;
- (void) removeEventListener:(NSString*)event listener:(id<MicroServiceEventListener>)listener;

- (void) emit:(NSString*)event;
- (void) emitObject:(NSString*)event object:(id)object;
- (void) emitNumber:(NSString*)event number:(NSNumber*)number;
- (void) emitString:(NSString*)event string:(NSString*)string;
- (void) emitBoolean:(NSString*)event boolean:(bool)boolean;

- (void) start;
- (void) startWithArguments:(NSArray*)argv;

@end
#endif /* MicroService_h */
