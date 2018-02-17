//
//  Symbol.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/6/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Symbol.h"
#include "Utils.h"

using namespace v8;

Local<Value> Symbol::Name() const
{
    return Local<Value>();
}

/**
 * Create a symbol. If name is not empty, it will be used as the description.
 */
Local<Symbol> Symbol::New(Isolate* isolate, Local<String> name)
{
    return Local<Symbol>();
}

/**
 * Access global symbol registry.
 * Note that symbols created this way are never collected, so
 * they should only be used for statically fixed properties.
 * Also, there is only one global name space for the names used as keys.
 * To minimize the potential for clashes, use qualified names as keys.
 */
Local<Symbol> Symbol::For(Isolate *isolate, Local<String> name)
{
    return Local<Symbol>();
}

/**
 * Retrieve a global symbol. Similar to |For|, but using a separate
 * registry that is not accessible by (and cannot clash with) JavaScript code.
 */
Local<Symbol> Symbol::ForApi(Isolate *isolate, Local<String> name)
{
    return Local<Symbol>();
}

// Well-known symbols
Local<Symbol> Symbol::GetHasInstance(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetIsConcatSpreadable(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetIterator(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetMatch(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetReplace(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetSearch(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetSplit(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetToPrimitive(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetToStringTag(Isolate* isolate)
{
    return Local<Symbol>();
}
Local<Symbol> Symbol::GetUnscopables(Isolate* isolate)
{
    return Local<Symbol>();
}

Local<Value> Private::Name() const
{
    return Local<Value>();
}

/**
 * Create a private symbol. If name is not empty, it will be the description.
 */
Local<Private> Private::New(Isolate* isolate,
                          Local<String> name)
{
    return Local<Private>();
}

/**
 * Retrieve a global private symbol. If a symbol with this name has not
 * been retrieved in the same isolate before, it is created.
 * Note that private symbols created this way are never collected, so
 * they should only be used for statically fixed properties.
 * Also, there is only one global name space for the names used as keys.
 * To minimize the potential for clashes, use qualified names as keys,
 * e.g., "Class#property".
 */
Local<Private> Private::ForApi(Isolate* isolate, Local<String> name)
{
    return Local<Private>();
}
