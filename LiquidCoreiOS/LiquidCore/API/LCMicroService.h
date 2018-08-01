//
//  MicroService.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/14/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

@class LCMicroService;
@class Process;

/*!
@interface
@discussion The LCSynchronizer object is used during MicrosService initialization to
 synchronize asynchronous initialization.  This object is only needed if you require some
 asynchronous initialization and can be ignored otherwise.
 */
@interface LCSynchronizer : NSObject

/*!
@method
@abstract Notify initializer that you are about to enter an asynchronous process.
 */
- (void) enter;

/*!
 @method
 @abstract Notify initializer that async init is complete.  Calls to 'enter' and 'exit' must
 be balanced.
 */
- (void) exit;

@end

/*!
 @protocol
 @discussion An LCMicroService may have an optional delegate to listen for events on start,
 exit and error conditions.
 */
@protocol LCMicroServiceDelegate <NSObject>

/*!
 @method
 @abstract Optional listener for when the LCMicroService has successfully started.
 @discussion Listens for when the MicroService has been inititialized and the environment is ready
 to receive event listeners.  This is called after the Node.js environment has been set
 up, but before the micro service JavaScript code has been executed.  It is safe to add any event
 listeners here, but emitted events will not be seen by the JavaScript service until its code
 has been run.  The JavaScript code should emit an event to let the host know that it is ready
 to receive events.
 @param service The micro service which has now started.
 @param synchronizer Used to synchronize asynchronous init.  Ignore if you have no
 async initialization.  Can be nil if the process cannot be managed asynchronously, so check first.
 */
@optional - (void) onStart:(LCMicroService*)service synchronizer:(LCSynchronizer* _Nullable)synchronizer;

/*!
 @method
 @abstract Optional listener for when the LCMicroService has successfully exited.
 @discussion Listens for when the LCMicroService has exited gracefully.  The LCMicroService is no longer
 available and is shutting down.

 Called immediately before the LCMicroService exits.  This is a graceful exit, and is
 mutually exclusive with the 'onError' method.  Only one of either the exit listener
 or error listener will be called from any LCMicroService.
 @param service The micro service which is now exiting.
 @param exitCode The exit code emitted by the Node.js process.
 */
@optional - (void) onExit:(LCMicroService*)service exitCode:(int)exitCode;

/*!
 @method
 @abstract Optional listener for when the LCMicroService has exited unexpectedly.
 @discussion Listens for any errors that may cause the LCMicroService to shut down unexpectedly.  The
 LCMicroService is no longer available and may have already crashed.
 
 Called upon an exception state.  This is an unexpected exit, and is mutually exclusive with the
 'onExit' method.  Only one of either the exit listener or error listener will be called from
 any MicroService.
 @param service The micro service which failed.
 @param exception The thrown error.
 */
@optional - (void) onError:(LCMicroService*)service exception:(NSException*)exception;

@end

/*!
 @protocol
 @discussion Clients of an LCMicroService communicate with the JavaScript service through an
 event emitter interface.  To listen for specific events, call
 <code>LCMicroService.addEventListener:listener:</code>.  The delegates will be called through
 this protocol.
 
 In JavaScript, Node.js services trigger events by calling <code>LiquidCore.emit(event, payload)</code>.
 The <code>payload</code> can be any JavaScript object and will get converted to the appropriate object
 in the host language (Swift/Objective-C).
 */
@protocol LCMicroServiceEventListener <NSObject>

/*!
 @method
 @abstract Called when an event registered with <code>LCMicroService.addEventListener:listener:</code>
 is triggered from JavaScript.
 @param service The micro service that emitted the event.
 @param event The emitted event.
 @param payload An optional data object emitted with the event.  The listener should know in advance
 the format emitted by the service.
 */
- (void) onEvent:(LCMicroService*)service event:(NSString*)event payload:(id _Nullable)payload;
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
