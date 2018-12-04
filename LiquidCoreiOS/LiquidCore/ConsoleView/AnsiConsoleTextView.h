/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <UIKit/UIKit.h>

@protocol AnsiConsoleTextViewDelegate <NSObject>
- (void) onTextRefreshed;
@end

@interface AnsiConsoleTextView : UITextView
@property (nonatomic, readwrite, weak) NSObject<AnsiConsoleTextViewDelegate>* refreshDelegate;

- (void) print:(NSString *)ansiString;
- (void) println:(NSString *)ansiString;

@end
