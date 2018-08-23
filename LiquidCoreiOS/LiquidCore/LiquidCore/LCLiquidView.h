//
//  LCLiquidView.h
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

IB_DESIGNABLE
/**
 @interface
 @note  `LCLiquidView` exposes an `LCMicroService` through a UI.  An `LCMicroService` attaches to a UI
 in JavaScript by calling `LiquidCore.attach(surface, callback)` where
 `surface` is a string representing the `LCSurface` class canonical name
 (e.g. `org.liquidplayer.surfaces.console.ConsoleSurface`) and `callback` is a
 callback function which accepts an `error` parameter.  If `error` is `undefined`, then the
 `LCSurface` was attached correctly and is ready for use.  Otherwise, `error` is a descriptive
 error message.
 */
@interface LCLiquidView : UIView

/**
 @property
 @note The URL of the micro service.  The `URL` is mutually exclusive with the `jsResource`.  Use only
 one or the other.
 The property is exposed through the Interface Builder in XCode and should not
 used programmatically.
 */
@property (nonatomic, strong) IBInspectable NSString* URL;

/**
 @property
 @note The path to a JavaScript resource file.  The `jsResource` is mutually exclusive with the `URL`.  Use only
 one or the other.
 The property is exposed through the Interface Builder in XCode and is not
 used programmatically.
 */
@property (nonatomic, strong) IBInspectable NSString* jsResource;

/**
 @property
 @note An array of space-delimited arguments to start the micro service with.
 The property is exposed through the Interface Builder in XCode and is not
 used programmatically.
 */
@property (nonatomic, strong) IBInspectable NSString* arguments;

/**
 @property
 @note An array of space-delimited canonical surface names.
 The property is exposed through the Interface Builder in XCode and is not
 used programmatically.
 
 If neither the `availableSurfaces` property is set, nor `-enableSurface:` called, then all
 registered surfaces will be available to the micro service.  This property only needs to be set if
 surfaces are intentionally restricted.
 */
@property (nonatomic, strong) IBInspectable NSString* availableSurfaces;

/**
 @method
 @note Makes a surfaces available to the `LCMicroService`.  Must be called prior to `-start:`.  In
 Interface Builder, an array of available surfaces can be provided using the `availableSurfaces` property.
 
 If neither the `availableSurfaces` property is set, nor `-enableSurface:` called, then all
 registered surfaces will be available to the micro service.  This method only needs to be called if
 surfaces are intentionally restricted.
 */
- (void) enableSurface:(NSString*)canonicalName, ...;

/**
 @method
 @note Starts an `LCMicroService` asynchronously.  In Inteface Builder, this can be auto-started using
 the `URL` or `jsResource` and `arguments` properties.
 @param uri  The micro service URI
 @param arguments Optional arguments loaded into `process.argv[2:]`
 @return the `LCMicroService`
 */
- (LCMicroService *) start:(NSURL*)uri arguments:(NSString*)argv, ...;

/**
 @method
 @note Starts an `LCMicroService` asynchronously.  In Inteface Builder, this can be auto-started using
 the `URL` or `jsResource` properties.
 @param uri  The micro service URI
 @return the `LCMicroService`
 */
- (LCMicroService *) start:(NSURL*)uri;

@end
