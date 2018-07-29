//
//  LCLiquidView.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/29/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MicroService.h"
#import "LCSurface.h"

@interface LCLiquidView : UIView
- (void) enableSurface:(NSString*)canonicalName, ...;
- (MicroService *) start:(NSURL*)uri arguments:(NSString*)argv, ...;
- (MicroService *) start:(NSURL*)uri;

@end
