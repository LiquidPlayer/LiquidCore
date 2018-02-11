//
//  Array.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Array.h"

using namespace v8;

uint32_t Array::Length() const
{
    return 0;
}

/**
 * Creates a JavaScript array with the given length. If the length
 * is negative the returned array will have length 0.
 */
Local<Array> Array::New(Isolate* isolate, int length)
{
    return Local<Array>();
}
