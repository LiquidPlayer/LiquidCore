//
//  ArrayBuffer.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/7/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef ArrayBuffer_h
#define ArrayBuffer_h

#include <v8.h>
#include <JavaScriptCore/JavaScript.h>

struct ArrayBufferViewImpl : v8::ArrayBufferView
{
    int m_byte_length;
};

#endif /* ArrayBuffer_h */
