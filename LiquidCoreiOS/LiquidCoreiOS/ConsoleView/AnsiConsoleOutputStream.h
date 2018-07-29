//
//  AnsiConsoleOutputStream.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/26/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

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
