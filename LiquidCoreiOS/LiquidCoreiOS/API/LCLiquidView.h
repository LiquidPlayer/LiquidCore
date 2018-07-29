//
//  LCLiquidView.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/29/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <LiquidCoreiOS/LCMicroService.h>

@interface LCLiquidView : UIView
- (void) enableSurface:(NSString*)canonicalName, ...;
- (LCMicroService *) start:(NSURL*)uri arguments:(NSString*)argv, ...;
- (LCMicroService *) start:(NSURL*)uri;

@end
