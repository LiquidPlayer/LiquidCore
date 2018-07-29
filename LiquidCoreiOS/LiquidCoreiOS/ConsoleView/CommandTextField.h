//
//  CommandTextField.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/27/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol CommandTextFieldDelegate <NSObject>
- (void) onUpArrow;
- (void) onDownArrow;
@end

@interface CommandTextField : UITextField
@property (nonatomic, weak, readwrite) id<CommandTextFieldDelegate> commandDelegate;
@end
