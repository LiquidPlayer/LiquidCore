//
//  ConsoleView.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/27/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AnsiConsoleTextView.h"
#import "CommandTextField.h"

@interface ConsoleView : UIView <UITextFieldDelegate, CommandTextFieldDelegate, UITextViewDelegate>

@property (weak, nonatomic) IBOutlet AnsiConsoleTextView *console;
@property (weak, nonatomic) IBOutlet CommandTextField *command;
@property (weak, nonatomic) IBOutlet UIButton *upHistory;
@property (weak, nonatomic) IBOutlet UIButton *downHistory;

@property (nonatomic) NSMutableArray* history;
@property (nonatomic) NSInteger item;

- (void) processCommand:(NSString *)cmd;
@end
