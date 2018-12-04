/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <UIKit/UIKit.h>

@protocol CommandTextFieldDelegate <NSObject>
- (void) onUpArrow;
- (void) onDownArrow;
@end

@interface CommandTextField : UITextField
@property (nonatomic, weak, readwrite) id<CommandTextFieldDelegate> commandDelegate;
@end
