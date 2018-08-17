//
//  AnsiConsoleTextView.m
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
