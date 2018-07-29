//
//  AnsiConsoleTextView.m
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/27/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import "AnsiConsoleTextView.h"
#import "AnsiConsoleOutputStream.h"

@interface AnsiConsoleTextView() <AnsiConsoleOutputStreamDelegate>
@property (readonly, strong, nonatomic) AnsiConsoleOutputStream* stream;
@property (readwrite, assign, atomic) BOOL awaitingRefresh;
@end

@implementation AnsiConsoleTextView

- (id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        _awaitingRefresh = NO;
        _stream = [[AnsiConsoleOutputStream alloc] initWithInitialDisplayText:self.attributedText delegate:self];
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        _awaitingRefresh = NO;
        _stream = [[AnsiConsoleOutputStream alloc] initWithInitialDisplayText:self.attributedText delegate:self];
    }
    return self;
}

- (void) onRefresh
{
    if (!self.awaitingRefresh) {
        self.awaitingRefresh = YES;
        dispatch_async(dispatch_get_main_queue(), ^{
            self.awaitingRefresh = NO;
            [self setAttributedText:self.stream.displayText];
            if (self.refreshDelegate != nil) {
                [self.refreshDelegate onTextRefreshed];
            }
        });
    }
}

- (void) print:(NSString *)ansiString
{
    [self.stream print:ansiString];
}

- (void) println:(NSString *)ansiString
{
    [self print:[NSString stringWithFormat:@"%@\n", ansiString]];
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

@end
