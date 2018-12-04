/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
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
