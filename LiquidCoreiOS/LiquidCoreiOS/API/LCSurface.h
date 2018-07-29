//
//  LCSurface.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MicroService.h"

typedef void (^OnAttachedHandler)(void);

@protocol LCSurface <NSObject>

- (void) bind:(MicroService*)service synchronizer:(Synchronizer*)synchronizer;
- (UIView*) attach:(MicroService*)service onAttached:(OnAttachedHandler)onAttachedHandler;
- (void) detach;
- (void) reset;

@end
