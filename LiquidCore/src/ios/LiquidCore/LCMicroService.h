/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
@class LCMicroService;
@class LCProcess;

/**
 An `LCMicroService` may have an optional delegate to listen for events on start,
 exit and error conditions.
 */
@protocol LCMicroServiceDelegate <NSObject>

@optional
/**
 Optional listener for when the `LCMicroService` has successfully started.
 
 Listens for when the `LCMicroService` has been inititialized and the environment is ready
 to receive event listeners.  This is called after the Node.js environment has been set
 up, but before the micro service JavaScript code has been executed.  It is safe to add any event
 listeners here, but emitted events will not be seen by the JavaScript service until its code
 has been run.  The JavaScript code should emit an event to let the host know that it is ready
 to receive events.
 @param service The micro service which has now started.
 */
- (void) onStart:(LCMicroService*_Nonnull)service;

@optional
/**
 Optional listener for when the `LCMicroService` has successfully exited.
 
 Listens for when the `LCMicroService` has exited gracefully.  The `LCMicroService` is no longer
 available and is shutting down.

 Called immediately before the `LCMicroService` exits.  This is a graceful exit, and is
 mutually exclusive with the `-onError:exception:` method.  Only one of either the exit listener
 or error listener will be called from any `LCMicroService`.
 @param service The micro service which is now exiting.
 @param exitCode The exit code emitted by the Node.js process.
 */
- (void) onExit:(LCMicroService*_Nonnull)service exitCode:(int)exitCode;

@optional
/**
 Optional listener for when the `LCMicroService` has exited unexpectedly.
 
 Listens for any errors that may cause the `LCMicroService` to shut down unexpectedly.  The
 `LCMicroService` is no longer available and may have already crashed.
 
 Called upon an exception state.  This is an unexpected exit, and is mutually exclusive with the
 `-onExit:exitCode:` method.  Only one of either the exit listener or error listener will be called from
 any MicroService.
 @param service The micro service which failed.
 @param exception The thrown error.
 */
- (void) onError:(LCMicroService*_Nonnull)service exception:(NSException*_Nonnull)exception;

@end

/**
 Clients of an `LCMicroService` communicate with the JavaScript service through an
 event emitter interface.  To listen for specific events, call
 `LCMicroService.-addEventListener:listener:`.  The delegates will be called through
 this protocol.
 
 In JavaScript, Node.js services trigger events by calling `LiquidCore.emit(event, payload)`.
 The `payload` can be any JavaScript object and will get converted to the appropriate object
 in the host language (Swift/Objective-C).
 */
@protocol LCMicroServiceEventListener <NSObject>

/**
 Called when an event registered with `-addEventListener:listener:`
 is triggered from JavaScript.
 @param service The micro service that emitted the event.
 @param event The emitted event.
 @param payload An optional data object emitted with the event.  The listener should know in advance
 the format emitted by the service.
 */
- (void) onEvent:(LCMicroService*_Nonnull)service event:(NSString*_Nonnull)event payload:(id _Nullable)payload;
@end

/**
 An `LCMicroService` is the basic building block of LiquidCore.  It encapsulates the runtime
 environment for a client-side micro app.  An `LCMicroService` is a complete virtual machine
 whose operation is defined by the code referenced by the service URI.  When an `LCMicroService`
 is instantiated, its Node.js environment is set up, its code downloaded (or fetched from cache)
 from the URI, and is executed in a VM.  The host may interact with the VM via a simple
 message-based API.
 */
@interface LCMicroService : NSObject

/**
 Each `LCMicroService` instance is mapped to a unique string id.  This id can be serialized
 in UIs and the instance retrieved by a call to `+serviceFromInstanceId:`
 */
@property (nonatomic, readonly, copy, nonnull) NSString* instanceId;

/**
 The Node.js Process object.
 */
@property (atomic, readonly, nullable) LCProcess* process;

/**
 The URI from which the service was started.
 */
@property (nonatomic, readonly, nonnull) NSURL* serviceURI;

/**
 Each `LCMicroService` instance is mapped to a unique string id.  This id can be serialized
 in UIs and the instance retrieved by a call to this method.
 @param instanceId  An id returned by the `instanceId` property
 @return  The associated `LCMicroService` or `nil` if no such service is active.
 */
+ (id _Nullable) serviceFromInstanceId:(NSString* _Nonnull) instanceId;

/**
 Uninstalls the `LCMicroService` from this host, and removes any global data associated with the
 service.
 @param serviceURI The URI of the service (should be the same URI that the service was started with).
 */
+ (void) uninstall:(NSURL* _Nonnull)serviceURI;

/**
 Generates a URL for fetching from a development server on the loopback address (localhost).
 @param fileName The name of the bundled javascript file to fetch (default: liquid.js)
 @param port The server's port (default: 8082)
 @return A service URL for use in the `LCMicroService` constructor
 */
+ (NSURL * _Nonnull) devServer:(NSString* _Nullable)fileName port:(NSNumber* _Nullable)port;

/**
 Generates a URL for fetching from a development server on the loopback address (localhost).
 Assumes the entry js file is `liquid.js` and is served from port 8082 on the emulator's host
 machine.
 @return A service URL for use in the `LCMicroService` constructor
 */
+ (NSURL * _Nonnull) devServer;

/**
 Generates a URL for fetching from the LiquidCore.bundle.  If compiled in DEBUG mode, this
 will attempt to first download from the development server.  If the server is unreachable, then
 it will default to the packaged bundle.  In release mode, this will always reference the packaged
 bundle.
 @param bundleName The name of the bundled file (ex. 'index' or 'example.js')
 @param options Development server options
 @return A service URL for use in the 'LCMicroService' constructor
 */
+ (NSURL * _Nonnull) bundle:(NSString* _Nullable)bundleName options:(NSDictionary* _Nullable)options;

/**
 Generates a URL for fetching from the LiquidCore.bundle.  If compiled in DEBUG mode, this
 will attempt to first download from the development server.  If the server is unreachable, then
 it will default to the packaged bundle.  In release mode, this will always reference the packaged
 bundle.
 @param bundleName The name of the bundled file (ex. 'index' or 'example.js')
 @return A service URL for use in the 'LCMicroService' constructor
*/
+ (NSURL * _Nonnull) bundle:(NSString* _Nullable)bundleName;

/**
 Creates a new instance of the micro service referenced by `serviceURI`.
 @param serviceURI  The URI (can be a network URL or local file/resource) of the micro service
 code
 */
- (id _Nullable) initWithURL:(NSURL* _Nonnull)serviceURI;

/**
 Creates a new instance of the micro service referenced by `serviceURI`
 @param serviceURI  The URI (can be a network URL or local file/resource) of the MicroService
 code
 @param delegate The `LCMicroServiceDelegate` for this service
 */
- (id _Nullable) initWithURL:(NSURL* _Nonnull)serviceURI delegate:(id<LCMicroServiceDelegate> _Nullable)delegate;

/**
 Adds an event listener for an event triggered by 'LiquidCore.emit(event, payload)' in
 JavaScript.  Example:
 ```javascript
      LiquidCore.emit('my_event', { stringData: 'foo', bar : 6 });
 ```
 This will trigger the `listener` added here, with the JavaScript object as a Swift/Objective-C
 dictionary
 @param event  The String event id
 @param listener  The event listener
 */
- (void) addEventListener:(NSString* _Nonnull)event listener:(id<LCMicroServiceEventListener> _Nonnull)listener;

/**
 Removes an EventListener previously added with `-addEventListener:listener:`.
 @param event  The event for which to unregister the listener
 @param listener  The listener to unregister
*/
- (void) removeEventListener:(NSString* _Nonnull)event listener:(id<LCMicroServiceEventListener> _Nonnull)listener;

/**
 Emits an event that can be received by the JavaScript code, if the `LCMicroService` has
 registered a listener.  Example:
 
 ```javascript
 LiquidCore.on('my_event', function() {
    console.log('Received my_event');
 });
 ```
 
 On the iOS (Swift) side:
 
 ```swift
 myService.emit("my_event")
 ```
 @param event  The event to trigger
 */
- (void) emit:(NSString* _Nonnull)event;

/**
 Emits an event that can be received by the JavaScript code, if the `LCMicroService` has
 registered a listener.  Example:
 
 ```javascript
 LiquidCore.on('my_event', function(payload) {
    // Do something with the payload data
    console.log(payload.hello);
 });
 ```
 
 On the iOS (Swift) side:
 
 ```swift
 let foo = [ "hello": "world" ]
 myService.emitObject("my_event", object:foo)
 ```
 @param event  The event to trigger
 @param object  The Swift/Objective-C object to emit
 */
- (void) emitObject:(NSString* _Nonnull)event object:(id _Nullable)object;

/**
 Emits an event that can be received by the JavaScript code, if the `LCMicroService` has
 registered a listener.  Example:
 
 ```javascript
 LiquidCore.on('my_event', function(number) {
     // Do something with the payload data
     console.log(payload.number + 2);
 });
 ```
 
 On the iOS (Swift) side:
 
 ```swift
 myService.emitNumber("my_event", number:40)
 ```
 @param event  The event to trigger
 @param number  The number to emit
 */
- (void) emitNumber:(NSString* _Nonnull)event number:(NSNumber* _Nonnull)number;

/**
 Emits an event that can be received by the JavaScript code, if the `LCMicroService` has
 registered a listener.  Example:
 
 ```javascript
 LiquidCore.on('my_event', function(string) {
     // Do something with the string data
     console.log(string + " are belong to us.");
 });
 ```
 
 On the iOS (Swift) side:
 
 ```swift
 myService.emitString("my_event", string: "All your base")
 ```
 @param event  The event to trigger
 @param string  The string to emit
 */
- (void) emitString:(NSString* _Nonnull)event string:(NSString* _Nonnull)string;

/**
 Emits an event that can be received by the JavaScript code, if the `LCMicroService` has
 registered a listener.  Example:
 
 ```javascript
 LiquidCore.on('my_event', function(bool) {
     // Do something with the string data
     console.log(bool === true ? "YUP" : "NOPE");
 });
 ```
 
 On the iOS (Swift) side:<br/>
 
 ```swift
 myService.emitBoolean("my_event", boolean:true)
 ```
 @param event  The event to trigger
 @param boolean  The boolean to emit
 */
- (void) emitBoolean:(NSString* _Nonnull)event boolean:(BOOL)boolean;

/**
 Starts the `LCMicroService`.  This method will return immediately and initialization and
 startup will occur asynchronously in a separate thread.  It will download the code from
 the service URI (if not cached), set the arguments in `process.argv` and execute the script.
 */
- (void) start;

/**
 Starts the `LCMicroService`.  This method will return immediately and initialization and
 startup will occur asynchronously in a separate thread.  It will download the code from
 the service URI (if not cached), set the arguments in `process.argv` and execute the script.
 @param argv  The list of arguments to sent to the `LCMicroService`.  This is similar to running
 node from a command line. The first two arguments will be the application (node)
 followed by the local module code (`/home/module/[service.js`].  `argv` arguments
 will then be appended in `process.argv[2:]`
 */
- (void) startWithArguments:(NSArray* _Nullable)argv;

@end
