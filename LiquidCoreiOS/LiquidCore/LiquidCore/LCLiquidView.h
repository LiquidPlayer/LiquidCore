/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <UIKit/UIKit.h>
#import <LiquidCore/LCMicroService.h>

IB_DESIGNABLE
/**
 `LCLiquidView` exposes an `LCMicroService` through a UI.  An `LCMicroService` attaches to a UI
 in JavaScript by calling `LiquidCore.attach(surface, callback)` where
 `surface` is a string representing the `LCSurface` class canonical name
 (e.g. `org.liquidplayer.surfaces.console.ConsoleSurface`) and `callback` is a
 callback function which accepts an `error` parameter.  If `error` is `undefined`, then the
 `LCSurface` was attached correctly and is ready for use.  Otherwise, `error` is a descriptive
 error message.
 */
@interface LCLiquidView : UIView

/**
 The URL of the micro service.  The `URL` is mutually exclusive with the `jsResource`.  Use only
 one or the other.
 @note The property is exposed through the Interface Builder in XCode and should not
 used programmatically.
 */
@property (nonatomic, strong) IBInspectable NSString* URL;

/**
 The path to a JavaScript resource file.  The `jsResource` is mutually exclusive with the `URL`.  Use only
 one or the other.
 @note The property is exposed through the Interface Builder in XCode and is not
 used programmatically.
 */
@property (nonatomic, strong) IBInspectable NSString* jsResource;

/**
 An array of space-delimited arguments to start the micro service with.
 @note The property is exposed through the Interface Builder in XCode and is not
 used programmatically.
 */
@property (nonatomic, strong) IBInspectable NSString* arguments;

/**
 An array of space-delimited canonical surface names.
 @note The property is exposed through the Interface Builder in XCode and is not
 used programmatically.
 
 If neither the `availableSurfaces` property is set, nor `-enableSurface:` called, then all
 registered surfaces will be available to the micro service.  This property only needs to be set if
 surfaces are intentionally restricted.
 */
@property (nonatomic, strong) IBInspectable NSString* availableSurfaces;

/**
 Makes a surface available to the `LCMicroService`.  Must be called prior to `-start:`.  In
 Interface Builder, an array of available surfaces can be provided using the `availableSurfaces` property.
 
 If neither the `availableSurfaces` property is set, nor `-enableSurface:` called, then all
 registered surfaces will be available to the micro service.  This method only needs to be called if
 surfaces are intentionally restricted.
 */
- (void) enableSurface:(NSString*)canonicalName, ...;

/**
 Starts an `LCMicroService` asynchronously.  In Inteface Builder, this can be auto-started using
 the `URL` or `jsResource` and `arguments` properties.
 @param uri  The micro service URI
 @param argv Optional arguments loaded into `process.argv[2:]`
 @return the `LCMicroService`
 */
- (LCMicroService *) start:(NSURL*)uri arguments:(NSString*)argv, ...;

/**
 Starts an `LCMicroService` asynchronously.  In Inteface Builder, this can be auto-started using
 the `URL` or `jsResource` properties.
 @param uri  The micro service URI
 @return the `LCMicroService`
 */
- (LCMicroService *) start:(NSURL*)uri;

@end
