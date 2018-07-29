//
//  LCSurface.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <LiquidCoreiOS/LCMicroService.h>

typedef void (^LCOnAttachedHandler)(void);

@protocol LCSurface <NSObject>
@property (class, readonly, copy) NSString* SURFACE_CANONICAL_NAME;
@property (class, readonly, copy) NSString* SURFACE_VERSION;

- (void) bind:(LCMicroService*)service synchronizer:(LCSynchronizer*)synchronizer;
- (UIView<LCSurface>*) attach:(LCMicroService*)service onAttached:(LCOnAttachedHandler)onAttachedHandler;
- (void) detach;
- (void) reset;

@end

@interface LCSurfaceRegistration : NSObject
+ (void) registerSurface:(Class<LCSurface>)surfaceClass;
@end
