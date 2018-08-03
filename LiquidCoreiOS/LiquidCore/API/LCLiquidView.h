//
//  LCLiquidView.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/29/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <LiquidCore/LCMicroService.h>

IB_DESIGNABLE
@interface LCLiquidView : UIView
@property (nonatomic, strong) IBInspectable NSString* URL;
@property (nonatomic, strong) IBInspectable NSString* jsResource;
@property (nonatomic, strong) IBInspectable NSString* arguments;

- (void) enableSurface:(NSString*)canonicalName, ...;
- (LCMicroService *) start:(NSURL*)uri arguments:(NSString*)argv, ...;
- (LCMicroService *) start:(NSURL*)uri;

@end
