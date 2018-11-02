//
//  LCSurface.h
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
