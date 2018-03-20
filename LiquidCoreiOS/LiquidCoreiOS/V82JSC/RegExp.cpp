//
//  RegExp.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/**
 * Creates a regular expression from the given pattern string and
 * the flags bit field. May throw a JavaScript exception as
 * described in ECMA-262, 15.10.4.1.
 *
 * For example,
 *   RegExp::New(v8::String::New("foo"),
 *               static_cast<RegExp::Flags>(kGlobal | kMultiline))
 * is equivalent to evaluating "/foo/gm".
 */
MaybeLocal<RegExp> RegExp::New(Local<Context> context,
                              Local<String> pattern,
                              Flags flags)
{
    return MaybeLocal<RegExp>();
}

/**
 * Returns the value of the source property: a string representing
 * the regular expression.
 */
Local<String> RegExp::GetSource() const
{
    return Local<String>();
}

/**
 * Returns the flags bit field.
 */
RegExp::Flags RegExp::GetFlags() const
{
    return Flags();
}
