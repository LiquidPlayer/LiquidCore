//
//  AnsiConsoleTextView.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/27/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol AnsiConsoleTextViewDelegate <NSObject>
- (void) onTextRefreshed;
@end

@interface AnsiConsoleTextView : UITextView
@property (nonatomic, readwrite, weak) NSObject<AnsiConsoleTextViewDelegate>* refreshDelegate;

- (void) print:(NSString *)ansiString;
- (void) println:(NSString *)ansiString;

@end
