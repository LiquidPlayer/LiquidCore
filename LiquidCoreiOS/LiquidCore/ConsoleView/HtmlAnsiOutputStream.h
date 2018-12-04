/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef HtmlAnsiOutputStream_h
#define HtmlAnsiOutputStream_h

#import "AnsiOutputStream.h"

@interface HtmlAnsiOutputStream : AnsiOutputStream
- (void) flush;
@end

#endif /* HtmlAnsiOutputStream_h */
