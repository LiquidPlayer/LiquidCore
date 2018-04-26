//
//  Symbol.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/6/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Local<Value> Symbol::Name() const
{
    ValueImpl* symbol = V82JSC::ToImpl<ValueImpl>(this);
    JSValueRef name = V82JSC::exec(symbol->m_context->m_ctxRef,
                                   "return /^Symbol\\((.*)\\)/.exec(_1.toString())[1]",
                                   1, &symbol->m_value);
    return ValueImpl::New(symbol->m_context, name);
}

/**
 * Create a symbol. If name is not empty, it will be used as the description.
 */
Local<Symbol> Symbol::New(Isolate* isolate, Local<String> name)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    Local<Context> context = _local<Context>(impl->m_defaultContext).toLocal();
    JSValueRef name_;
    if (*name) {
        name_ = V82JSC::ToJSValueRef(name, context);
    } else {
        name_ = JSValueMakeUndefined(impl->m_defaultContext->m_ctxRef);
    }
    JSValueRef symbol = V82JSC::exec(impl->m_defaultContext->m_ctxRef, "return Symbol(_1)", 1, &name_);
    return ValueImpl::New(impl->m_defaultContext, symbol).As<Symbol>();
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
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    Local<Context> context = _local<Context>(impl->m_defaultContext).toLocal();
    JSValueRef name_value = V82JSC::ToJSValueRef(name, context);
    JSValueRef symbol = V82JSC::exec(impl->m_defaultContext->m_ctxRef, "return Symbol.for(_1)", 1, &name_value);

    return ValueImpl::New(impl->m_defaultContext, symbol).As<Symbol>();
}

/**
 * Retrieve a global symbol. Similar to |For|, but using a separate
 * registry that is not accessible by (and cannot clash with) JavaScript code.
 */
Local<Symbol> Symbol::ForApi(Isolate *isolate, Local<String> name)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    Local<Context> context = _local<Context>(impl->m_defaultContext).toLocal();

    String::Utf8Value symbol_name(name);
    if (impl->m_global_symbols.count(*symbol_name) == 0) {
        impl->m_global_symbols[*symbol_name] = V82JSC::ToJSValueRef(Symbol::New(isolate), context);
        JSValueProtect(impl->m_defaultContext->m_ctxRef, impl->m_global_symbols[*symbol_name]);
    }
    JSValueRef symbol = impl->m_global_symbols[*symbol_name];
    return ValueImpl::New(impl->m_defaultContext, symbol).As<Symbol>();
}

// Well-known symbols
Local<Symbol> Symbol::GetHasInstance(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.hasInstance", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetIsConcatSpreadable(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.isConcatSpreadable", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetIterator(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.iterator", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetMatch(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.match", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetReplace(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.replace", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetSearch(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.search", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetSplit(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.split", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetToPrimitive(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.toPrimitive", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetToStringTag(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.toStringTag", 0, 0)).As<Symbol>();
}
Local<Symbol> Symbol::GetUnscopables(Isolate* isolate)
{
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    return ValueImpl::New(impl->m_defaultContext, V82JSC::exec(impl->m_defaultContext->m_ctxRef,
                                                               "return Symbol.unscopables", 0, 0)).As<Symbol>();
}

Local<Value> Private::Name() const
{
    return _local<Symbol>(const_cast<Private*>(this)).toLocal()->Name();
}

/**
 * Create a private symbol. If name is not empty, it will be the description.
 */
Local<Private> Private::New(Isolate* isolate,
                          Local<String> name)
{
    Local<Symbol> symbol = Symbol::New(isolate, name);
    return _local<Private>(*symbol).toLocal();
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
    IsolateImpl *impl = V82JSC::ToIsolateImpl(isolate);
    Local<Context> context = _local<Context>(impl->m_defaultContext).toLocal();
    
    String::Utf8Value symbol_name(name);
    if (impl->m_private_symbols.count(*symbol_name) == 0) {
        impl->m_private_symbols[*symbol_name] = V82JSC::ToJSValueRef(Symbol::New(isolate), context);
        JSValueProtect(impl->m_defaultContext->m_ctxRef, impl->m_private_symbols[*symbol_name]);
    }
    JSValueRef symbol = impl->m_private_symbols[*symbol_name];
    return _local<Private>(*ValueImpl::New(impl->m_defaultContext, symbol)).toLocal();
}
