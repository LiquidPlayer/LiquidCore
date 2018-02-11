//
//  Exception.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Exception.h"

using namespace v8;

Local<Value> Exception::RangeError(Local<String> message)
{
    return Local<Value>();
}
Local<Value> Exception::ReferenceError(Local<String> message)
{
    return Local<Value>();
}
Local<Value> Exception::SyntaxError(Local<String> message)
{
    return Local<Value>();
}
Local<Value> Exception::TypeError(Local<String> message)
{
    return Local<Value>();
}
Local<Value> Exception::Error(Local<String> message)
{
    return Local<Value>();
}

/**
 * Creates an error message for the given exception.
 * Will try to reconstruct the original stack trace from the exception value,
 * or capture the current stack trace if not available.
 */
Local<Message> Exception::CreateMessage(Isolate* isolate, Local<Value> exception)
{
    return Local<Message>();
}

/**
 * Returns the original stack trace that was captured at the creation time
 * of a given exception, or an empty handle if not available.
 */
Local<StackTrace> Exception::GetStackTrace(Local<Value> exception)
{
    return Local<StackTrace>();
}
