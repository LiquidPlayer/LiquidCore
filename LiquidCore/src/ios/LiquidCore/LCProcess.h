/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <JavaScriptCore/JavaScriptCore.h>

@class LCProcess;

/**
 Clients must subclass an `LCProcessDelegate` to get state change information from the
 node.js process
 */
@protocol LCProcessDelegate <NSObject>

/**
 Called when a node.js process is actively running.  This is the one and only opportunity
 to start running JavaScript in the process.  If the receiver returns from this method
 without executing asynchronous JS, the process will exit almost immediately afterward.
 This is the time to execute scripts.
 
 @note This method is called inside of the node.js process thread, so any JavaScript code called
 here is executed synchronously.
 
 @note In the event that an `LCProcessDelegate` is added by `-addDelegate:` after a
 process has already started, this method will be called immediately (in the process
 thread) if the process is still active.  Otherwise, `-onProcessExit:exitCode:` will be called.
 
 @note Clients must hold a reference to the context for as long as they want the process to
 remain active.  If the context is garbage collected, the process will be exited
 automatically in order to prevent misuse.
 
 @param process The node.js `LCProcess` object
 @param context The JavaScript `JSContext` for this process
 */
- (void) onProcessStart:(LCProcess*)process context:(JSContext*)context;

/**
 Called when a node.js process has completed all of its callbacks and has nothing left
 to do.  If there is any cleanup required, now is the time to do it.  If the caller would
 like the process to abort its exit sequence, this is the time to either call
 `[process keepAlive]` or execute an asynchronous operation.  If no callbacks are pending
 after this method returns, then the process will exit.
 
 @note This method is called inside of the node.js process thread, so any JavaScript code called
 here is executed synchronously.
 
 @param process The node.js `LCProcess` object
 @param exitCode The node.js exit code
 */

- (void) onProcessAboutToExit:(LCProcess*)process exitCode:(int)code;

/**
 Called after a process has exited.  The process is no longer active and cannot be used.
 
 @note In the event that an EventListener is added by `[process addDelegate:]` after a
 process has already exited, this method will be called immediately.
 
 @param process The defunct node.js `LCProcess` object
 @param exitCode The node.js exit code
 */
- (void) onProcessExit:(LCProcess*)process exitCode:(int)code;

/**
 Called in the event of an `LCProcess` failure
 
 @param process  The node.js `LCProcess` object
 @param exception The thrown exception
 */
- (void) onProcessFailed:(LCProcess*)process exception:(NSException*)exception;
@end

/**
 A callback block to execute synchronous or asynchronous JavaScript on a node
 process thread.
 */
typedef void (^ProcessThreadBlock)(JSContext*);

/**
 Determines the scope of an uninstallation.  A Local uninstallation will only clear
 data and files related to instances on this host.  A Global uninstallation will
 clear also public data.
 */
typedef enum UninstallScope {
    /** Uninstall only local files */
    LOCAL,
    /** Uninstall global files as well as local */
    GLOBAL
} /** */
UninstallScope;

/**
 Possible permissions for media access.
 */
typedef enum MediaAccessMask {
    /**
     No access allowed.
     */
    PermissionsNone = 0,
    /**
     Read-only access.
     */
    PermissionsRead = 1,
    /**
     Write-only access.
     */
    PermissionsWrite = 2,
    /**
     Read/write access.
     */
    PermissionsRW = 3
} /** */
MediaAccessMask;

/**
 Holds a process open while this object is not collected or until `-letDie` is called.
 */
@protocol LoopPreserver<NSObject>

/**
 Invalidates this preserver and allows the process to die when it is done.
 */
- (void) letDie;

@end

/**
 A node.js process instance
 */
@interface LCProcess : NSObject

/**
 The real path of the `/home/module` alias for this process.
 */
@property (nonatomic, readonly, copy) NSString* modulePath;

/**
 The real path of the `/home/node_modules` path for this process.
 */
@property (nonatomic, readonly, copy) NSString* node_modulesPath;

/**
 Creates a node.js process and attaches a delegate
 
 @param delegate The process delegate.
 @param uniqueID A unique ID to identify the process namespace across runs.  The file system will
                be preserved and reused for all processes using the same ID.
 @param mediaAccessMask Permission mask for read/write access to external media.
 */
- (id) initWithDelegate:(id<LCProcessDelegate>)delegate
                     id:(NSString*)uniqueID
        mediaAccessMask:(MediaAccessMask)mediaAccessMask;

/**
 Adds an `LCProcessDelegate` to this process
 @param delegate the new delegate
 */
- (void) addDelegate:(id<LCProcessDelegate>)delegate;

/**
 Removes a delegate from this process
 @param delegate the delegate to remove
 */
- (void) removeDelegate:(id<LCProcessDelegate>)delegate;

/**
 Determines if the process is currently active.  If it is inactive, either it hasn't
 yet been started, or the process completed. Use an `LCProcessDelegate` to determine the
 state.
 @return true if active, false otherwise
 */
- (bool) isActive;

/**
 Schedules a callback to be called on the process thread and blocks the current thread until
 completed.
 
 @param block the callback to execute on the Node process theead.
 */
- (void) sync:(ProcessThreadBlock)block;

/**
 Schedules a callback to be called on the process thread asynchronously.

 @param block the callback to execute on the Node process theead.
 */
- (void) async:(ProcessThreadBlock)block;

/**
 Instructs the VM to halt execution as quickly as possible
 @param exitc The exit code
 */
- (void) exit:(int)code;

/**
 Instructs the VM not to shutdown the process when no more callbacks are pending.  In effect,
 this method indefinitely leaves a callback pending until the resulting
 `LoopPreserver` is released.  The loop preserver
 must eventually be released or the process will remain active indefinitely.
 
 @return A preserver object
 */
- (id<LoopPreserver>) keepAlive;

/**
 Can be used by an embedder to expose a directory in the underlying iOS  filesystem
 that is accessible from the host app.  The directory must not be root (/) or /home,
 and it must be absolute (not relative pathing).
 @param dir The directory to expose
 @param mediaAccessMask The access rights given to the Process
 */
- (void) exposeHostDirectory:(NSString * _Nonnull)dir
             mediaAccessMask:(MediaAccessMask)mediaAccessMask;

/**
 Uninstalls a given process class identified by its `uniqueID`
 
 @param uniqueID The id of the process class
 @param scope scope in which to uninstall the process class
 */
+ (void) uninstall:(NSString*)uniqueID scope:(UninstallScope)scope;

@end
