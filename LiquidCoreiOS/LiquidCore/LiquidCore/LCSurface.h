/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <UIKit/UIKit.h>
#import <LiquidCore/LCMicroService.h>
#import <JavaScriptCore/JavaScriptCore.h>

/**
 A parameterless block for defining a callback to be received once
 the surface has been bound or attached.
 */
typedef void (^LCOnSuccessHandler)(void);

/**
 A block for defining a callback to received if binding or attaching fails.
 @param errorMessage The error message to send back to the client.
 */
typedef void (^LCOnFailHandler)(NSString *errorMessage);

/**
 An `LCSurface` is a UI interaction layer with an `LCMicroService`.
 */
@protocol LCSurface <NSObject>

/**
 The canonical name of the surface.
 */
@property (class, readonly, copy) NSString* SURFACE_CANONICAL_NAME;

/**
 A version string of the format `<major>.<minor>.<patch>` for the surface.
 */
@property (class, readonly, copy) NSString* SURFACE_VERSION;

/**
 Binds a surface to an `LCMicroService`. This gives the surface an opportunity to bind any native
 functions required by the module.
 @param service The micro service to bind.
 @param exportObject An object which will be passed back to the caller where the surface may
 export its JS bindings
 @param config An optional configuration parameter passed from JavaScript
 @param onBound A block which must be called if and when binding is successful
 @param onError A Block which must be called if and only if binding fails
 */
- (void) bind:(LCMicroService*)service
       export:(JSValue *)exportObject
       config:(JSValue *)config
      onBound:(LCOnSuccessHandler)onBound
      onError:(LCOnFailHandler)onError;

/**
 Attaches an `LCMicroService` to the UI.
 @param service  The `LCMicroService` to attach.
 @param onAttachedHandler A block which must be called if and when attaching is successful
 @param onError A Block which must be called if and only if attaching fails
 @return The UIView of the newly attached surface.
 */
- (UIView<LCSurface>*) attach:(LCMicroService*)service
                   onAttached:(LCOnSuccessHandler)onAttachedHandler
                      onError:(LCOnFailHandler)onError;

/**
 Detaches any currently attached `LCMicroService`.  The `LCSurface` may then be ready to be discarded
 or reattached to another `LCMicroService`.
 */
- (void) detach;

/**
 Detaches and clears the LCSurface.  This is the preferred method when the `LCSurface` is to be
 reused.  `-detach` may leave behind some UI state, as it expects that the micro service may
 later be re-attached.  `-reset` clears any UI state back to a fresh state.
 */
- (void) reset;

@end

/**
 An `LCSurface` must register itself with LiquidCore to become available to micro services.
 */
@interface LCSurfaceRegistration : NSObject

/**
 Registers an `LCSurface` class with LiquidCore.
 @param surfaceClass Must be an instantiable `UIView` subclass that implements the `LCSurface` protocol.
 */
+ (void) registerSurface:(Class<LCSurface>)surfaceClass;
@end
