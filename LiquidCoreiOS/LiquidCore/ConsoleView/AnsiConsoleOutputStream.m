//
//  AnsiConsoleOutputStream.m
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

#import "AnsiConsoleOutputStream.h"

@interface AnsiConsoleOutputStream()
@property (atomic, strong, readonly) NSMutableArray *consoleStrings;
@property (atomic, assign) BOOL halt;
@property (atomic) BOOL need_refresh;
@end

@implementation AnsiConsoleOutputStream {
    int index;
    NSMutableArray *cursorPositionStack;
    NSObject<AnsiConsoleOutputStreamDelegate>* delegate_;
}
- (id) initWithInitialDisplayText:(NSAttributedString *)text delegate:(NSObject<AnsiConsoleOutputStreamDelegate>*)delegate
{
    NSOutputStream *outputStream = [[NSOutputStream alloc] initToMemory];
    [outputStream open];
    self = [super initWithOutputStream:outputStream];
    if (self) {
        _displayText = text;
        index = _displayText.length;
        cursorPositionStack = [[NSMutableArray alloc] init];
        delegate_ = delegate;
        _consoleStrings = [[NSMutableArray alloc] init];
        _halt = NO;
        _need_refresh = NO;
        
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            volatile bool halt = self.halt;
            while (!halt) {
                int size = [self.consoleStrings count];
                if (size > 0) {
                    NSMutableString *queued = [[NSMutableString alloc] init];
                    for (int i=0; i<size; i++) {
                        [queued appendString:[self.consoleStrings firstObject]];
                        [self.consoleStrings removeObjectAtIndex:0];
                    }
                    NSData *data = [queued dataUsingEncoding:NSUTF8StringEncoding];
                    [self write:[data bytes] maxLength:data.length];
                    [self flush];
                }
                if (self.need_refresh) {
                    self.need_refresh = NO;
                    [self->delegate_ onRefresh];
                }
                usleep(100);
                halt = self.halt;
            }
        });
    }
    return self;
}

- (void) print:(NSString*)ansiString
{
    [self.consoleStrings addObject:ansiString];
}

- (void) flush
{
    [super flush];
    NSOutputStream *currentStream = self.outputStream;

    self.outputStream = [[NSOutputStream alloc] initToMemory];
    [self.outputStream open];

    [currentStream close];

    NSData *contents = [currentStream propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    NSString *htmlString = [[NSString alloc] initWithData:contents encoding:NSUTF8StringEncoding];

    htmlString = [NSString stringWithFormat:@"<span style=\"font-family: Menlo; font-size: 12\">%@</span>", htmlString];

    NSMutableAttributedString *text_ = [[NSMutableAttributedString alloc]
                                initWithData: [htmlString dataUsingEncoding:NSUnicodeStringEncoding]
                                     options: @{ NSDocumentTypeDocumentAttribute: NSHTMLTextDocumentType }
                          documentAttributes: nil
                                       error: nil];

    if (index == self.displayText.length) {
        NSMutableAttributedString *mutable = [[NSMutableAttributedString alloc] initWithAttributedString:self.displayText];
        [mutable appendAttributedString:text_];
        self.displayText = mutable;
    } else if (index < self.displayText.length) {
        if (index + text_.length >= self.displayText.length) {
            NSMutableAttributedString *mutable = [[NSMutableAttributedString alloc] initWithAttributedString:[self.displayText attributedSubstringFromRange:NSMakeRange(0, index)]];
            [mutable appendAttributedString:text_];
            self.displayText = mutable;
        } else {
            NSMutableAttributedString *mutable = [[NSMutableAttributedString alloc] initWithAttributedString:[self.displayText attributedSubstringFromRange:NSMakeRange(0, index)]];
            NSAttributedString *last = [self.displayText attributedSubstringFromRange:NSMakeRange(index + text_.length, self.displayText.length)];
            [mutable appendAttributedString:text_];
            [mutable appendAttributedString:last];
            self.displayText = mutable;
        }
    } else {
        NSLog(@"Console flush: index is greater than total buffer length");
        NSMutableAttributedString *mutable = [[NSMutableAttributedString alloc] initWithAttributedString:self.displayText];
        [mutable appendAttributedString:text_];
        index = self.displayText.length;
        self.displayText = mutable;
    }
    index += text_.length;

    self.need_refresh = YES;
}

- (void) processEraseScreen:(EraseScreenOption) eraseOption
{
    switch(eraseOption) {
        case ERASE_SCREEN_TO_END:
            self.displayText = [self.displayText attributedSubstringFromRange:NSMakeRange(0, MIN(index,self.displayText.length))];
            break;
        case ERASE_SCREEN_TO_BEGINING:
            // FIXME
            self.displayText = [[NSAttributedString alloc] init];
            index = 0;
            break;
        case ERASE_SCREEN:
            self.displayText = [[NSAttributedString alloc] init];
            index = 0;
            break;
    }
    [self flush];
}

- (void) processCursorTo:(int) row col:(int) col
{
    // Move to row 'row'
    index = 0;
    int row_count = 1;
    int col_count = 1;
    NSString* text = [self.displayText string];
    while (row_count < row && index < text.length) {
        if ([text characterAtIndex:index] == '\n') {
            row_count ++;
        }
        index ++;
    }
    
    // Move to column 'col'
    for (col_count = 1;
         col_count < col && index < text.length && [text characterAtIndex:index] != '\n';
         col_count++)
        index++;
    [self flush];
}

- (CGRect) currentCursorPos
{
    CGRect rect = CGRectMake(1,1,0,0);
    NSString* text = [self.displayText string];

    for (int i=0; i<index; i++) {
        if ([text characterAtIndex:i] == '\n') {
            rect.origin.y ++;
            rect.origin.x = 0;
        } else {
            rect.origin.x ++;
        }
    }
    
    return rect;
}

- (void) processCursorToColumn:(int) x
{
    CGRect pos = [self currentCursorPos];
    [self processCursorTo:pos.origin.y col:MAX(1,pos.origin.x)];
}

- (void) processCursorUpLine:(int) count
{
    CGRect pos = [self currentCursorPos];
    [self processCursorTo:MAX(pos.origin.y - count,1) col:1];
}

- (void) processCursorDownLine:(int) count
{
    CGRect pos = [self currentCursorPos];
    [self processCursorTo:pos.origin.y + count col:1];
}

- (void) processCursorLeft:(int) count
{
    CGRect pos = [self currentCursorPos];
    [self processCursorTo:pos.origin.y col:MAX(pos.origin.x - count,1)];
}

- (void) processCursorRight:(int) count
{
    CGRect pos = [self currentCursorPos];
    [self processCursorTo:pos.origin.y col:pos.origin.x + count];
}

- (void) processCursorDown:(int) count
{
    CGRect pos = [self currentCursorPos];
    [self processCursorTo:pos.origin.y + count col:pos.origin.x];
}

- (void) processCursorUp:(int) count
{
    CGRect pos = [self currentCursorPos];
    [self processCursorTo:MAX(pos.origin.y - count,1) col:pos.origin.x];
}


- (void) processRestoreCursorPosition
{
    if (cursorPositionStack.count > 0) {
        CGRect pos = [[cursorPositionStack lastObject] CGRectValue];
        [cursorPositionStack removeLastObject];
        
        [self processCursorTo:pos.origin.y col:pos.origin.x];
    }
}

- (void) processSaveCursorPosition
{
    CGRect pos = [self currentCursorPos];
    [cursorPositionStack addObject:[NSValue valueWithCGRect:pos]];
}

- (void) processScrollDown:(int)optionInt
{
    NSLog(@"ConsoleTextView: processScrollDown");
}

- (void) processScrollUp:(int)lines
{
    NSLog(@"ConsoleTextView: processScrollUp");
}

- (void) processEraseLine:(EraseLineOption)eraseOption
{
    switch(eraseOption) {
        case ERASE_LINE_TO_END: {
            NSString *text = [self.displayText string];
            int next;
            for (next = index; next < text.length && [text characterAtIndex:next] != '\n'; index++);
            if (next < text.length) {
                NSMutableAttributedString *mutable = [[NSMutableAttributedString alloc] initWithAttributedString:[self.displayText attributedSubstringFromRange:NSMakeRange(0, index)]];
                [mutable appendAttributedString:[self.displayText attributedSubstringFromRange:NSMakeRange(next, text.length)]];
                self.displayText = mutable;
            } else {
                self.displayText = [self.displayText attributedSubstringFromRange:NSMakeRange(0, index)];
            }
            break;
        }
        case ERASE_LINE_TO_BEGINING: {
            NSString *text = [self.displayText string];
            int currIndex = index;
            CGRect currPos = [self currentCursorPos];
            [self processCursorTo:currPos.origin.y col:1];
            NSMutableAttributedString *mutable = [[NSMutableAttributedString alloc] initWithAttributedString:[self.displayText attributedSubstringFromRange:NSMakeRange(0, index)]];
            [mutable appendAttributedString:[self.displayText attributedSubstringFromRange:NSMakeRange(currIndex, text.length)]];
            self.displayText = mutable;
            break;
        }
        case ERASE_LINE: {
            CGRect currPos = [self currentCursorPos];
            [self processCursorTo:currPos.origin.y col:1];
            [self processEraseLine:ERASE_LINE_TO_END];
            break;
        }
    }
    [self flush];
}

@end
