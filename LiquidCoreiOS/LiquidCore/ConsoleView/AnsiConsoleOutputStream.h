/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <UIKit/UIKit.h>
#import "HtmlAnsiOutputStream.h"

#ifndef AnsiConsoleOutputStream_h
#define AnsiConsoleOutputStream_h

@protocol AnsiConsoleOutputStreamDelegate
- (void) onRefresh;
@end

@interface AnsiConsoleOutputStream : HtmlAnsiOutputStream
@property (strong, atomic) NSAttributedString *displayText;

- (id) initWithInitialDisplayText:(NSAttributedString *)text delegate:(NSObject<AnsiConsoleOutputStreamDelegate>*)delegate;
- (void) print:(NSString*)ansiString;
@end

#endif
