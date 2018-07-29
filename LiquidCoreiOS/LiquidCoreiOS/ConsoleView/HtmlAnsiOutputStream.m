//
//  HtmlAnsiOutputStream.m
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/26/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HtmlAnsiOutputStream.h"

static NSString* ANSI_COLOR_MAP[] = {@"black", @"red",
    @"green", @"yellow", @"blue", @"magenta", @"cyan", @"white"};

static const char* BYTES_QUOT = "&quot;";
static const char* BYTES_AMP = "&amp;";
static const char* BYTES_LT = "&lt;";
static const char* BYTES_GT = "&gt;";
static const char* BYTES_BR = "<br>";
static const char* BYTES_SP = "&nbsp;";

@implementation HtmlAnsiOutputStream {
    bool concealOn;
    NSMutableArray* closingAttributes;
}

- (void)flush
{
    [self closeAttributes];
}

- (id)initWithOutputStream:(NSOutputStream *)os
{
    self = [super initWithOutputStream:os];
    if (self) {
        concealOn = false;
        closingAttributes = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void) writeString:(NSString *)s
{
    const char * cs = [s cStringUsingEncoding:NSUTF8StringEncoding];
    [self.outputStream write:(uint8_t*)cs maxLength:strlen(cs)];
}

- (void) writeAttribute:(NSString *) s
{
    [self.outputStream write:(uint8_t*)"<" maxLength:1];
    const char * cs = [s cStringUsingEncoding:NSUTF8StringEncoding];
    [self.outputStream write:(uint8_t*)cs maxLength:strlen(cs)];
    [self.outputStream write:(uint8_t*)">" maxLength:1];
    [closingAttributes insertObject:[[s componentsSeparatedByString:@" "] objectAtIndex:0] atIndex:0];
}

- (void) closeAttributes
{
    for (NSString *attr in closingAttributes) {
        [self.outputStream write:(uint8_t*)"</" maxLength:2];
        const char * cs = [attr cStringUsingEncoding:NSUTF8StringEncoding];
        [self.outputStream write:(uint8_t*)cs maxLength:strlen(cs)];
        [self.outputStream write:(uint8_t*)">" maxLength:1];
    }
    [closingAttributes removeAllObjects];
}

- (void) write:(unsigned char) data
{
    switch (data) {
        case 34: // "
            [self.outputStream write:(uint8_t*)BYTES_QUOT maxLength:strlen(BYTES_QUOT)];
            break;
        case 38: // &
            [self.outputStream write:(uint8_t*)BYTES_AMP maxLength:strlen(BYTES_AMP)];
            break;
        case 60: // <
            [self.outputStream write:(uint8_t*)BYTES_LT maxLength:strlen(BYTES_LT)];
            break;
        case 62: // >
            [self.outputStream write:(uint8_t*)BYTES_GT maxLength:strlen(BYTES_GT)];
            break;
        case '\r':
            // ignore
            break;
        case '\n': // <br>
            [self.outputStream write:(uint8_t*)BYTES_BR maxLength:strlen(BYTES_BR)];
            break;
        case ' ':
            [self.outputStream write:(uint8_t*)BYTES_SP maxLength:strlen(BYTES_SP)];
            break;
        default:
            [super write:data];
    }
}

- (void) processSetAttribute:(AnsiAttribute) attribute
{
    NSString *enc = [NSString stringWithFormat:@"%C[8m", 0x001B];
    
    switch (attribute) {
        case ATTRIBUTE_CONCEAL_ON:
            [self writeString:enc];
            concealOn = true;
            break;
        case ATTRIBUTE_INTENSITY_BOLD:
            [self writeAttribute:@"strong"];
            break;
        case ATTRIBUTE_INTENSITY_NORMAL:
            [self closeAttributes];
            break;
        case ATTRIBUTE_UNDERLINE:
            [self writeAttribute:@"u"];
            break;
        case ATTRIBUTE_UNDERLINE_OFF:
            [self closeAttributes];
            break;
        case ATTRIBUTE_NEGATIVE_ON:
            break;
        case ATTRIBUTE_NEGATIVE_Off:
            break;
        default:
            break;
    }
}

- (void) processAttributeRest
{
    if (concealOn) {
        NSString *enc = [NSString stringWithFormat:@"%C[0m", 0x001B];
        [self writeString:enc];
        concealOn = false;
    }
    [self closeAttributes];
}

- (void) processSetForegroundColor:(AnsiColor) color bright:(bool)bright
{
    [self writeAttribute:[[@"font color=\"" stringByAppendingString:ANSI_COLOR_MAP[color]] stringByAppendingString:@"\""]];
}

- (void) processSetBackgroundColor:(AnsiColor) color bright:(bool)bright
{
    [self writeAttribute:[[@"font background-color=\"" stringByAppendingString:ANSI_COLOR_MAP[color]] stringByAppendingString:@"\""]];
}


@end
