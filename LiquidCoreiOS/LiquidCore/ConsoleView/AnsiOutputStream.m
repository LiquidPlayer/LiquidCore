//
//  AnsiOutputStream.m
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

#import <Foundation/Foundation.h>
#import "AnsiOutputStream.h"

typedef unsigned char byte;

@interface AnsiOutputStream()
@end

static const int MAX_ESCAPE_SEQUENCE_LENGTH = 100;
static const int LOOKING_FOR_FIRST_ESC_CHAR = 0;
static const int LOOKING_FOR_SECOND_ESC_CHAR = 1;
static const int LOOKING_FOR_NEXT_ARG = 2;
static const int LOOKING_FOR_STR_ARG_END = 3;
static const int LOOKING_FOR_INT_ARG_END = 4;
static const int LOOKING_FOR_OSC_COMMAND = 5;
static const int LOOKING_FOR_OSC_COMMAND_END = 6;
static const int LOOKING_FOR_OSC_PARAM = 7;
static const int LOOKING_FOR_ST = 8;

static const int FIRST_ESC_CHAR = 27;
static const int SECOND_ESC_CHAR = '[';
static const int SECOND_OSC_CHAR = ']';
static const int BEL = 7;
static const int SECOND_ST_CHAR = '\\';

@implementation AnsiOutputStream {
    int pos;
    int startOfValue;
    NSMutableArray *options;
    int state;
    byte buffer[MAX_ESCAPE_SEQUENCE_LENGTH];
    NSOutputStream *out;
}

- (void) close
{
    [self.outputStream close];
    [super close];
}

- (id) initWithOutputStream:(NSOutputStream *)os
{
    self = [super init];
    if (self) {
        pos = 0;
        state = LOOKING_FOR_FIRST_ESC_CHAR;
        options = [[NSMutableArray alloc] init];
        _outputStream = os;
    }
    return self;
}

- (BOOL) hasSpaceAvailable
{
    return [super hasSpaceAvailable];
}

- (NSInteger)write:(const uint8_t *)buffer maxLength:(NSUInteger)len
{
    for (int i=0; i<len; i++) {
        [self write:buffer[i]];
    }
    return len;
}

- (void)write:(byte) data
{
    switch (state) {
        case LOOKING_FOR_FIRST_ESC_CHAR:
            if (data == FIRST_ESC_CHAR) {
                buffer[pos++] = (byte) data;
                state = LOOKING_FOR_SECOND_ESC_CHAR;
            } else {
                [self.outputStream write:&data maxLength:1];
            }
            break;
            
        case LOOKING_FOR_SECOND_ESC_CHAR:
            buffer[pos++] = (byte) data;
            if (data == SECOND_ESC_CHAR) {
                state = LOOKING_FOR_NEXT_ARG;
            } else if (data == SECOND_OSC_CHAR) {
                state = LOOKING_FOR_OSC_COMMAND;
            } else {
                [self reset:false];
            }
            break;
            
        case LOOKING_FOR_NEXT_ARG:
            buffer[pos++] = (byte) data;
            if ('"' == data) {
                startOfValue = pos - 1;
                state = LOOKING_FOR_STR_ARG_END;
            } else if ('0' <= data && data <= '9') {
                startOfValue = pos - 1;
                state = LOOKING_FOR_INT_ARG_END;
            } else if (';' == data) {
                [options addObject:[[NSNull alloc] init]];
            } else if ('?' == data) {
                [options addObject:@"?"];
            } else if ('=' == data) {
                [options addObject:@"="];
            } else {
                [self reset:[self processEscapeCommand:options command:data]];
            }
            break;
        default:
            break;
            
        case LOOKING_FOR_INT_ARG_END:
            buffer[pos++] = (byte) data;
            if (!('0' <= data && data <= '9')) {
                NSData *d = [NSData dataWithBytes:&buffer[startOfValue] length:(pos - 1) - startOfValue];
                NSString *strValue = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];
                NSNumber *value = [NSNumber numberWithInt:strValue.integerValue];
                [options addObject:value];
                if (data == ';') {
                    state = LOOKING_FOR_NEXT_ARG;
                } else {
                    [self reset:[self processEscapeCommand:options command:data]];
                }
            }
            break;
            
        case LOOKING_FOR_STR_ARG_END:
            buffer[pos++] = (byte) data;
            if ('"' != data) {
                NSData *d = [NSData dataWithBytes:&buffer[startOfValue] length:(pos - 1) - startOfValue];
                NSString *strValue = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];
                [options addObject:strValue];
                if (data == ';') {
                    state = LOOKING_FOR_NEXT_ARG;
                } else {
                    [self reset:[self processEscapeCommand:options command:data]];
                }
            }
            break;
            
        case LOOKING_FOR_OSC_COMMAND:
            buffer[pos++] = (byte) data;
            if ('0' <= data && data <= '9') {
                startOfValue = pos - 1;
                state = LOOKING_FOR_OSC_COMMAND_END;
            } else {
                [self reset:false];
            }
            break;
            
        case LOOKING_FOR_OSC_COMMAND_END:
            buffer[pos++] = (byte) data;
            if (';' == data) {
                NSData *d = [NSData dataWithBytes:&buffer[startOfValue] length:(pos - 1) - startOfValue];
                NSString *strValue = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];
                NSNumber *value = [NSNumber numberWithInt:strValue.integerValue];
                [options addObject:value];
                startOfValue = pos;
                state = LOOKING_FOR_OSC_PARAM;
            } else if (!('0' <= data && data <= '9')) {
                // oops, did not expect this
                [self reset:false];
            }
            break;
            
        case LOOKING_FOR_OSC_PARAM:
            buffer[pos++] = (byte) data;
            if (BEL == data) {
                NSData *d = [NSData dataWithBytes:&buffer[startOfValue] length:(pos - 1) - startOfValue];
                NSString *strValue = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];
                [options addObject:strValue];
                [self reset:[self processOperatingSystemCommand:options]];
            } else if (FIRST_ESC_CHAR == data) {
                state = LOOKING_FOR_ST;
            } /*else {
               // just keep looking while adding text
               }*/
            break;
            
        case LOOKING_FOR_ST:
            buffer[pos++] = (byte) data;
            if (SECOND_ST_CHAR == data) {
                NSData *d = [NSData dataWithBytes:&buffer[startOfValue] length:(pos - 1) - startOfValue];
                NSString *strValue = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];
                [options addObject:strValue];
                [self reset:[self processOperatingSystemCommand:options]];
            } else {
                state = LOOKING_FOR_OSC_PARAM;
            }
            break;
    }
    
    // Is it just too long?
    if (pos >= MAX_ESCAPE_SEQUENCE_LENGTH) {
        [self reset:false];
    }
}

/**
 * Resets all state to continue with regular parsing
 * @param skipBuffer if current buffer should be skipped or written to out
 */
- (void) reset:(bool) skipBuffer
{
    pos = 0;
    startOfValue = 0;
    [options removeAllObjects];
    state = LOOKING_FOR_FIRST_ESC_CHAR;
}

- (bool) processEscapeCommand:(NSArray*)options command:(byte)command
{
    @try {
        switch (command) {
            case 'A':
                [self processCursorUp:[self optionInt:options index:0 defaultValue:1]];
                return true;
            case 'B':
                [self processCursorDown:[self optionInt:options index:0 defaultValue:1]];
                return true;
            case 'C':
                [self processCursorRight:[self optionInt:options index:0 defaultValue:1]];
                return true;
            case 'D':
                [self processCursorLeft:[self optionInt:options index:0 defaultValue:1]];
                return true;
            case 'E':
                [self processCursorDownLine:[self optionInt:options index:0 defaultValue:1]];
                return true;
            case 'F':
                [self processCursorUpLine:[self optionInt:options index:0 defaultValue:1]];
                return true;
            case 'G':
                [self processCursorToColumn:[self optionInt:options index:0]];
                return true;
            case 'H':
            case 'f':
                [self processCursorTo:[self optionInt:options index:0 defaultValue:1]
                                  col:[self optionInt:options index:1 defaultValue:1]];
                return true;
            case 'J':
                [self processEraseScreen:[self optionInt:options index:0 defaultValue:0]];
                return true;
            case 'K':
                [self processEraseLine:[self optionInt:options index:0 defaultValue:0]];
                return true;
            case 'S':
                [self processScrollUp:[self optionInt:options index:0 defaultValue:1]];
                return true;
            case 'T':
                [self processScrollDown:[self optionInt:options index:0 defaultValue:1]];
                return true;
            case 'm':
                // Validate all options are ints...
                for (NSObject* next in options) {
                    if (![next isKindOfClass:NSNull.class] && ![next isKindOfClass:NSNumber.class]) {
                        @throw NSInvalidArgumentException;
                    }
                }
                
                int count = 0;
                for (NSObject* next in options) {
                    if (![next isKindOfClass:NSNull.class]) {
                        count++;
                        int value = [(NSNumber*)next integerValue];
                        if (30 <= value && value <= 37) {
                            [self processSetForegroundColor:value - 30];
                        } else if (40 <= value && value <= 47) {
                            [self processSetBackgroundColor:value - 40];
                        } else if (90 <= value && value <= 97) {
                            [self processSetForegroundColor:value - 90 bright:true];
                        } else if (100 <= value && value <= 107) {
                            [self processSetBackgroundColor:value - 100 bright:true];
                        } else {
                            switch (value) {
                                case 39:
                                    [self processDefaultTextColor];
                                    break;
                                case 49:
                                    [self processDefaultBackgroundColor];
                                    break;
                                case 0:
                                    [self processAttributeRest];
                                    break;
                                default:
                                    [self processSetAttribute:value];
                            }
                        }
                    }
                }
                if (count == 0) {
                    [self processAttributeRest];
                }
                return true;
            case 's':
                [self processSaveCursorPosition];
                return true;
            case 'u':
                [self processRestoreCursorPosition];
                return true;
                
            default:
                if ('a' <= command && 'z' <= command) {
                    [self processUnknownExtension:options command:command];
                    return true;
                }
                if ('A' <= command && 'Z' <= command) {
                    [self processUnknownExtension:options command:command];
                    return true;
                }
                return false;
        }
    } @catch (NSException *error) {
        // Ignore
    }
    return false;
}

- (bool)processOperatingSystemCommand:(NSArray*)options
{
    int command = [self optionInt:options index:0];
    NSString* label = options[1];
    // for command > 2 label could be composed (i.e. contain ';'), but we'll leave
    // it to processUnknownOperatingSystemCommand implementations to handle that
    @try {
        switch (command) {
            case 0:
                [self processChangeIconNameAndWindowTitle:label];
                return true;
            case 1:
                [self processChangeIconName:label];
                return true;
            case 2:
                [self processChangeWindowTitle:label];
                return true;
                
            default:
                // not exactly unknown, but not supported through dedicated process methods:
                [self processUnknownOperatingSystemCommand:command param:label];
                return true;
        }
    } @catch (NSException* ignore) {
    }
    return false;
}

- (void) processRestoreCursorPosition
{
}

- (void) processSaveCursorPosition
{
}

- (void) processScrollDown:(int)optionInt
{
}

- (void) processScrollUp:(int)optionInt
{
}

- (void) processEraseScreen:(EraseScreenOption) eraseOption
{
}

- (void) processEraseLine:(EraseLineOption) eraseOption
{
}

- (void) processSetAttribute:(AnsiAttribute) attribute
{
}

- (void) processSetForegroundColor:(AnsiColor) color
{
    [self processSetForegroundColor:color bright:false];
}

- (void) processSetForegroundColor:(AnsiColor) color bright:(bool) bright
{
}

- (void) processSetBackgroundColor:(AnsiColor) color
{
    [self processSetBackgroundColor:color bright:false];
}

- (void) processSetBackgroundColor:(AnsiColor) color bright:(bool)bright
{
}

- (void) processDefaultTextColor
{
}

- (void) processDefaultBackgroundColor
{
}

- (void) processAttributeRest
{
}

- (void) processCursorTo:(int) row col:(int) col
{
}

- (void) processCursorToColumn:(int) x
{
}

- (void) processCursorUpLine:(int) count
{
}

- (void) processCursorDownLine:(int) count
{
    // Poor mans impl..
    for (int i = 0; i < count; i++) {
        [self.outputStream write:(const byte*)"\n" maxLength:1];
    }
}

- (void) processCursorLeft:(int) count
{
}

- (void) processCursorRight:(int) count
{
    // Poor mans impl..
    for (int i = 0; i < count; i++) {
        [self.outputStream write:(const byte*)" " maxLength:1];
    }
}

- (void) processCursorDown:(int) count
{
}

- (void) processCursorUp:(int) count
{
}

- (void) processUnknownExtension:(NSArray*)options command:(int)command
{
}

- (void) processChangeIconNameAndWindowTitle:(NSString*) label
{
    [self processChangeIconName:label];
    [self processChangeWindowTitle:label];
}

- (void) processChangeIconName:(NSString*) label
{
}

- (void) processChangeWindowTitle:(NSString*) label
{
}

- (void) processUnknownOperatingSystemCommand:(int) command param:(NSString*) param
{
}

- (int) optionInt:(NSArray*) options index:(int)index
{
    if (options.count <= index)
        @throw NSInvalidArgumentException;
    if (![options[index] isKindOfClass:NSNumber.class])
        @throw NSInvalidArgumentException;
    return ((NSNumber*)options[index]).integerValue;
}

- (int) optionInt:(NSArray*) options index:(int)index defaultValue:(int)defaultValue
{
    if (options.count > index) {
        if ([options[index] isKindOfClass:NSNumber.class])
            return ((NSNumber*)options[index]).integerValue;
    }
    return defaultValue;
}

@end
