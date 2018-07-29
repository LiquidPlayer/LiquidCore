//
//  CommandTextField.m
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/27/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import "CommandTextField.h"

@implementation CommandTextField

- (NSArray *) keyCommands
{
    UIKeyCommand *upArrow = [UIKeyCommand keyCommandWithInput: UIKeyInputUpArrow modifierFlags: 0 action: @selector(upArrow:)];
    UIKeyCommand *downArrow = [UIKeyCommand keyCommandWithInput: UIKeyInputDownArrow modifierFlags: 0 action: @selector(downArrow:)];
    return [[NSArray alloc] initWithObjects: upArrow, downArrow, nil];
}

- (void) upArrow: (UIKeyCommand *) keyCommand
{
    if (self.commandDelegate != nil) {
        [self.commandDelegate onUpArrow];
    }
}

- (void) downArrow: (UIKeyCommand *) keyCommand
{
    if (self.commandDelegate != nil) {
        [self.commandDelegate onDownArrow];
    }
}

@end
