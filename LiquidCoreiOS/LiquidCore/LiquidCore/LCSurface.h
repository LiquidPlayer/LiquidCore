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

/**
 @typedef
 @note A parameterless block for defining a callback to be received once
 the surface has attached.
 */
typedef void (^LCOnAttachedHandler)(void);

/**
 @protocol
 @note An `LCSurface` is a UI interaction layer with an `LCMicroService`.
 */
@protocol LCSurface <NSObject>

/**
 @property
 @note The canonical name of the surface.
 */
@property (class, readonly, copy) NSString* SURFACE_CANONICAL_NAME;

/**
 @property
 @note A version string of the format `<major>.<minor>.<patch>` for the surface.
 */
@property (class, readonly, copy) NSString* SURFACE_VERSION;

/**
 @method
 @note Binds an `LCMicroService` to the surface.  This will get called after the micro service
 is started, but before the service javascript is executed.  This gives the surface
 an opportunity to bind any native functions required by the module.
 @param service The micro service to bind.
 @param synchronizer Used for synchronizing asynchronous init.
 */
- (void) bind:(LCMicroService*)service synchronizer:(LCSynchronizer*)synchronizer;

/**
 @method
 @note Attaches an `LCMicroService` to the UI.
 @param service  The `LCMicroService` to attach.
 @param onAttached An execution block to be called after the UI is active
 @return The UIView of the newly attached surface.
 */
- (UIView<LCSurface>*) attach:(LCMicroService*)service onAttached:(LCOnAttachedHandler)onAttachedHandler;

/**
 @method
 @note Detaches any currently attached `LCMicroService`.  The `LCSurface` may then be ready to be discarded
 or reattached to another `LCMicroService`.
 */
- (void) detach;

/**
 @method
 @note Detaches and clears the LCSurface.  This is the preferred method when the `LCSurface` is to be
 reused.  `-detach` may leave behind some UI state, as it expects that the micro service may
 later be re-attached.  `-reset` clears any UI state back to a fresh state.
 */
- (void) reset;

@end

/**
 @interface
 @note An `LCSurface` must register itself with LiquidCore to become available to micro services.
 */
@interface LCSurfaceRegistration : NSObject

/**
 @method
 @note Registers an `LCSurface` class with LiquidCore.
 @param surfaceClass Must be an instantiable `UIView` subclass that implements the `LCSurface` protocol.
 */
+ (void) registerSurface:(Class<LCSurface>)surfaceClass;
@end
