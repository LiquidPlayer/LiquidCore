//
//  AnsiOutputStream.h
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

#ifndef AnsiOutputStream_h
#define AnsiOutputStream_h

typedef enum _EraseScreenOption
{
    ERASE_SCREEN_TO_END = 0,
    ERASE_SCREEN_TO_BEGINING = 1,
    ERASE_SCREEN = 2
} EraseScreenOption;

typedef enum _EraseLineOption
{
    ERASE_LINE_TO_END = 0,
    ERASE_LINE_TO_BEGINING = 1,
    ERASE_LINE = 2
} EraseLineOption;

typedef enum _AnsiAttribute
{
    ATTRIBUTE_INTENSITY_BOLD = 1, //     Intensity: Bold
    ATTRIBUTE_INTENSITY_FAINT = 2, //     Intensity; Faint     not widely supported
    ATTRIBUTE_ITALIC = 3, //     Italic; on     not widely supported. Sometimes treated as inverse.
    ATTRIBUTE_UNDERLINE = 4, //     Underline; Single
    ATTRIBUTE_BLINK_SLOW = 5, //     Blink; Slow     less than 150 per minute
    ATTRIBUTE_BLINK_FAST = 6, //     Blink; Rapid     MS-DOS ANSI.SYS; 150 per minute or more
    ATTRIBUTE_NEGATIVE_ON = 7, //     Image; Negative     inverse or reverse; swap foreground and background
    ATTRIBUTE_CONCEAL_ON = 8, //     Conceal on
    ATTRIBUTE_UNDERLINE_DOUBLE = 21, //     Underline; Double     not widely supported
    ATTRIBUTE_INTENSITY_NORMAL = 22, //     Intensity; Normal     not bold and not faint
    ATTRIBUTE_UNDERLINE_OFF = 24, //     Underline; None
    ATTRIBUTE_BLINK_OFF = 25, //     Blink; off
    ATTRIBUTE_NEGATIVE_Off = 27, //     Image; Positive
    ATTRIBUTE_CONCEAL_OFF = 28 //     Reveal     conceal off
} AnsiAttribute;


typedef enum _AnsiColor
{
    BLACK = 0,
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    MAGENTA = 5,
    CYAN = 6,
    WHITE = 7
} AnsiColor;

@interface AnsiOutputStream : NSOutputStream
@property (strong, readwrite, atomic) NSOutputStream *outputStream;

- (id) initWithOutputStream:(NSOutputStream *)os;
- (void) write:(unsigned char) data;
- (void) processRestoreCursorPosition;
- (void) processSaveCursorPosition;
- (void) processScrollDown:(int)lines;
- (void) processScrollUp:(int)lines;
- (void) processEraseScreen:(EraseScreenOption) eraseOption;
- (void) processEraseLine:(EraseLineOption) eraseOption;
- (void) processSetAttribute:(AnsiAttribute) attribute;
- (void) processSetForegroundColor:(AnsiColor) color;
- (void) processSetForegroundColor:(AnsiColor) color bright:(bool) bright;
- (void) processSetBackgroundColor:(AnsiColor) color;
- (void) processSetBackgroundColor:(AnsiColor) color bright:(bool)bright;
- (void) processDefaultTextColor;
- (void) processDefaultBackgroundColor;
- (void) processAttributeRest;
- (void) processCursorTo:(int) row col:(int) col;
- (void) processCursorToColumn:(int) x;
- (void) processCursorUpLine:(int) count;
- (void) processCursorDownLine:(int) count;
- (void) processCursorLeft:(int) count;
- (void) processCursorRight:(int) count;
- (void) processCursorDown:(int) count;
- (void) processCursorUp:(int) count;
- (void) processUnknownExtension:(NSArray*)options command:(int)command;
- (void) processChangeIconNameAndWindowTitle:(NSString*) label;
- (void) processChangeIconName:(NSString*) label;
- (void) processChangeWindowTitle:(NSString*) label;
- (void) processUnknownOperatingSystemCommand:(int) command param:(NSString*) param;
@end

#endif /* AnsiOutputStream_h */
