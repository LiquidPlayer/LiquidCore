//
//  HtmlAnsiOutputStream.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/26/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef HtmlAnsiOutputStream_h
#define HtmlAnsiOutputStream_h

#import "AnsiOutputStream.h"

@interface HtmlAnsiOutputStream : AnsiOutputStream
- (void) flush;
@end

#endif /* HtmlAnsiOutputStream_h */
