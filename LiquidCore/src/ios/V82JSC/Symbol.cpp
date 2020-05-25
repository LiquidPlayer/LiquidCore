/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;
using namespace v8;

Local<v8::Value> Symbol::Name() const
{
    EscapableHandleScope scope(ToIsolate(this));
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef symbol = ToJSValueRef(this, context);
    JSValueRef name = exec(ctx,
                                   "return /^Symbol\\((.*)\\)/.exec(_1.toString())[1]",
                                   1, &symbol);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), name));
}

/**
 * Create a symbol. If name is not empty, it will be used as the description.
 */
Local<Symbol> Symbol::New(Isolate* isolate, Local<String> name)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);

    JSValueRef name_;
    if (*name) {
        name_ = ToJSValueRef(name, context);
    } else {
        name_ = JSValueMakeUndefined(ctx);
    }
    JSValueRef symbol = exec(ctx, "return Symbol(_1)", 1, &name_);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), symbol).As<Symbol>());
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
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef name_value = ToJSValueRef(name, context);
    JSValueRef symbol = exec(ctx, "return Symbol.for(_1)", 1, &name_value);

    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), symbol).As<Symbol>());
}

/**
 * Retrieve a global symbol. Similar to |For|, but using a separate
 * registry that is not accessible by (and cannot clash with) JavaScript code.
 */
Local<Symbol> Symbol::ForApi(Isolate *isolate, Local<String> name)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* impl = ToIsolateImpl(isolate);

    String::Utf8Value symbol_name(name);
    if (impl->m_global_symbols.count(*symbol_name) == 0) {
        impl->m_global_symbols[*symbol_name] = ToJSValueRef(Symbol::New(isolate), context);
        JSValueProtect(ctx, impl->m_global_symbols[*symbol_name]);
    }
    JSValueRef symbol = impl->m_global_symbols[*symbol_name];
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), symbol).As<Symbol>());
}

// Well-known symbols
Local<Symbol> Symbol::GetHasInstance(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.hasInstance", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetIsConcatSpreadable(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.isConcatSpreadable", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetIterator(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.iterator", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetMatch(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.match", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetReplace(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.replace", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetSearch(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.search", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetSplit(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.split", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetToPrimitive(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.toPrimitive", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetToStringTag(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.toStringTag", 0, 0)).As<Symbol>());
}
Local<Symbol> Symbol::GetUnscopables(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "return Symbol.unscopables", 0, 0)).As<Symbol>());
}

Local<v8::Value> Private::Name() const
{
    return reinterpret_cast<const Symbol*>(this)->Name();
}

/**
 * Create a private symbol. If name is not empty, it will be the description.
 */
Local<Private> Private::New(Isolate* isolate,
                          Local<String> name)
{
    EscapableHandleScope scope(isolate);
    Local<Symbol> symbol = Symbol::New(isolate, name);
    return scope.Escape(* reinterpret_cast<Local<Private>*>(&symbol));
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
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* impl = ToIsolateImpl(isolate);

    String::Utf8Value symbol_name(name);
    if (impl->m_private_symbols.count(*symbol_name) == 0) {
        impl->m_private_symbols[*symbol_name] = ToJSValueRef(Symbol::New(isolate), context);
        JSValueProtect(ctx, impl->m_private_symbols[*symbol_name]);
    }
    JSValueRef symbol = impl->m_private_symbols[*symbol_name];
    Local<Value> priv = V82JSC::Value::New(ToContextImpl(context), symbol);
    return scope.Escape(* reinterpret_cast<Local<Private>*>(&priv));
}
