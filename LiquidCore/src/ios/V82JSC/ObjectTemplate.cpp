/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "ObjectTemplate.h"
#include "Object.h"
#include "JSCPrivate.h"
#include <string.h>

using namespace V82JSC;
using namespace v8;

static GenericNamedPropertyGetterCallback NullNamedGetter =
    [](Local<Name> property, const PropertyCallbackInfo<v8::Value>& info) {};
static GenericNamedPropertySetterCallback NullNamedSetter =
[](Local<Name> property, Local<v8::Value> value, const PropertyCallbackInfo<v8::Value>& info) {};
static GenericNamedPropertyDescriptorCallback NullNamedDescriptor =
    [](Local<Name> property, const PropertyCallbackInfo<v8::Value>& info) {};
static GenericNamedPropertyDeleterCallback NullNamedDeleter =
    [](Local<Name> property, const PropertyCallbackInfo<v8::Boolean>& info) {};
static GenericNamedPropertyEnumeratorCallback NullNamedEnumerator =
    [](const PropertyCallbackInfo<Array>& info) {};
static GenericNamedPropertyDefinerCallback NullNamedDefiner =
    [](Local<Name> property, const PropertyDescriptor& desc, const PropertyCallbackInfo<v8::Value>& info) {};
static GenericNamedPropertyQueryCallback NullNamedQuery =
    [](Local<Name> property, const PropertyCallbackInfo<Integer>& info) {};

static IndexedPropertyGetterCallback NullIndexedGetter =
    [](uint32_t index, const PropertyCallbackInfo<v8::Value>& info) {};
static IndexedPropertySetterCallback NullIndexedSetter =
[](uint32_t index, Local<v8::Value> value, const PropertyCallbackInfo<v8::Value>& info) {};
static IndexedPropertyDescriptorCallback NullIndexedDescriptor =
    [](uint32_t index, const PropertyCallbackInfo<v8::Value>& info) {};
static IndexedPropertyDeleterCallback NullIndexedDeleter =
    [](uint32_t index, const PropertyCallbackInfo<v8::Boolean>& info) {};
static IndexedPropertyEnumeratorCallback NullIndexedEnumerator =
    [](const PropertyCallbackInfo<Array>& info) {};
static IndexedPropertyDefinerCallback NullIndexedDefiner =
    [](uint32_t index, const PropertyDescriptor& desc, const PropertyCallbackInfo<v8::Value>& info) {};
static IndexedPropertyQueryCallback NullIndexedQuery =
    [](uint32_t index, const PropertyCallbackInfo<Integer>& info) {};

struct DefaultNamedHandlers : public NamedPropertyHandlerConfiguration
{
    DefaultNamedHandlers() : NamedPropertyHandlerConfiguration
    (NullNamedGetter, NullNamedSetter, NullNamedDescriptor, NullNamedDeleter,
     NullNamedEnumerator, NullNamedDefiner) { query = NullNamedQuery; }
};

struct DefaultIndexedHandlers : public IndexedPropertyHandlerConfiguration
{
    DefaultIndexedHandlers() : IndexedPropertyHandlerConfiguration
    (NullIndexedGetter, NullIndexedSetter, NullIndexedDescriptor, NullIndexedDeleter,
     NullIndexedEnumerator, NullIndexedDefiner) { query = NullIndexedQuery; }
};

#define THROW_ACCESS_ERROR() \
info.GetIsolate()->ThrowException(Exception::TypeError(v8::String::NewFromUtf8(info.GetIsolate(), "access denied", \
    v8::NewStringType::kNormal).ToLocalChecked()));

struct AccessDeniedNamedHandlers : public NamedPropertyHandlerConfiguration
{
    AccessDeniedNamedHandlers() : NamedPropertyHandlerConfiguration
    (
     [](Local<Name> property, const PropertyCallbackInfo<v8::Value>& info) { THROW_ACCESS_ERROR() },
     [](Local<Name> property, Local<v8::Value> value, const PropertyCallbackInfo<v8::Value>& info) { THROW_ACCESS_ERROR() },
     [](Local<Name> property, const PropertyCallbackInfo<v8::Value>& info) { THROW_ACCESS_ERROR() },
     [](Local<Name> property, const PropertyCallbackInfo<v8::Boolean>& info) { THROW_ACCESS_ERROR() },
     [](const PropertyCallbackInfo<Array>& info) { THROW_ACCESS_ERROR() },
     [](Local<Name> property, const PropertyDescriptor& desc, const PropertyCallbackInfo<v8::Value>& info) { THROW_ACCESS_ERROR() }
     )
    {
        query = [](Local<Name> property, const PropertyCallbackInfo<Integer>& info) { THROW_ACCESS_ERROR() };
    }
};

struct AccessDeniedIndexedHandlers : public IndexedPropertyHandlerConfiguration
{
    AccessDeniedIndexedHandlers() : IndexedPropertyHandlerConfiguration
    (
     [](uint32_t index, const PropertyCallbackInfo<v8::Value>& info) { THROW_ACCESS_ERROR() },
     [](uint32_t index, Local<v8::Value> value, const PropertyCallbackInfo<v8::Value>& info) { THROW_ACCESS_ERROR() },
     [](uint32_t index, const PropertyCallbackInfo<v8::Value>& info) { THROW_ACCESS_ERROR() },
     [](uint32_t index, const PropertyCallbackInfo<v8::Boolean>& info) { THROW_ACCESS_ERROR() },
     [](const PropertyCallbackInfo<Array>& info) { THROW_ACCESS_ERROR() },
     [](uint32_t index, const PropertyDescriptor& desc, const PropertyCallbackInfo<v8::Value>& info) { THROW_ACCESS_ERROR() }
     )
    {
        query = [](uint32_t index, const PropertyCallbackInfo<Integer>& info) { THROW_ACCESS_ERROR() };
    }
};


/** Creates an ObjectTemplate. */
Local<v8::ObjectTemplate> v8::ObjectTemplate::New(
                                 Isolate* isolate,
                                 Local<FunctionTemplate> constructor)
{
    EscapableHandleScope scope(isolate);
    
    if (!constructor.IsEmpty()) {
        return scope.Escape(constructor->InstanceTemplate());
    } else {
        auto otempl = static_cast<V82JSC::ObjectTemplate*>
            (HeapAllocator::Alloc(ToIsolateImpl(isolate),
                                     ToIsolateImpl(isolate)->m_object_template_map));

        otempl->m_constructor_template.Reset();
        otempl->m_named_data = 0;
        otempl->m_indexed_data = 0;
        otempl->m_named_handler = DefaultNamedHandlers();
        otempl->m_indexed_handler = DefaultIndexedHandlers();
        otempl->m_named_failed_access_handler = AccessDeniedNamedHandlers();
        otempl->m_indexed_failed_access_handler = AccessDeniedIndexedHandlers();

        return scope.Escape(CreateLocal<ObjectTemplate>(isolate, otempl));
    }
}

/** Get a template included in the snapshot by index. */
MaybeLocal<v8::ObjectTemplate> v8::ObjectTemplate::FromSnapshot(Isolate* isolate,
                                               size_t index)
{
    NOT_IMPLEMENTED;
}

/** Creates a new instance of this template.*/
MaybeLocal<Object> v8::ObjectTemplate::NewInstance(Local<Context> context)
{
    auto impl = ToImpl<V82JSC::ObjectTemplate>(this);
    auto ctx = ToContextImpl(context);
    IsolateImpl* iso = ToIsolateImpl(ctx);
    Isolate* isolate = ToIsolate(iso);
    EscapableHandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    // Temporarily disable access checks until we are done setting up the object
    DisableAccessChecksScope disable_scope(iso, impl);
    
    Local<ObjectTemplate> thiz = CreateLocal<ObjectTemplate>(&iso->ii, impl);
    
    LocalException exception(iso);
    
    JSObjectRef instance = 0;
    if (!impl->m_constructor_template.IsEmpty()) {
        MaybeLocal<Function> ctor = impl->m_constructor_template.Get(isolate)->GetFunction(context);
        if (!ctor.IsEmpty()) {
            JSValueRef ctor_func = ToJSValueRef(ctor.ToLocalChecked(), context);
            instance = JSObjectCallAsConstructor(ctx->m_ctxRef, (JSObjectRef)ctor_func, 0, 0, &exception);
            return scope.Escape(V82JSC::Value::New(ctx, instance).As<Object>());
        } else {
            return MaybeLocal<Object>();
        }
    } else if (impl->m_callback) {
        JSClassDefinition def = kJSClassDefinitionEmpty;
        if (impl->m_callback) {
            def.callAsFunction = V82JSC::Template::callAsFunctionCallback;
            def.callAsConstructor = V82JSC::Template::callAsConstructorCallback;
        }
        JSClassRef claz = JSClassCreate(&def);
        void * data = PersistentData<ObjectTemplate>(isolate, thiz);
        def.finalize = [](JSObjectRef obj) {
            void *data = JSObjectGetPrivate(obj);
            ReleasePersistentData<ObjectTemplate>(data);
        };

        instance = JSObjectMake(ctx->m_ctxRef, claz, data);
    } else {
        instance = JSObjectMake(ctx->m_ctxRef, 0, 0);
    }
    MaybeLocal<Object> o = impl->NewInstance(context, instance, false);
    if (o.IsEmpty()) {
        return MaybeLocal<Object>();
    }
    return scope.Escape(o.ToLocalChecked());
}

#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)
#define CALLBACK_PARAMS JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, \
size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception
#define PASS ctx, function, thisObject, argumentCount, arguments, exception

class InterceptorGetter {};
class InterceptorSetter {};
class InterceptorOther {};

template <typename V, typename I>
JSValueRef PropertyHandler(CALLBACK_PARAMS,
                           void (*named_handler)(const V82JSC::ObjectTemplate*, Local<Name>, Local<v8::Value>, PropertyCallbackInfo<V>&, const NamedPropertyHandlerConfiguration&),
                           void (*indexed_handler)(const V82JSC::ObjectTemplate*, uint32_t, Local<v8::Value>, PropertyCallbackInfo<V>&, const IndexedPropertyHandlerConfiguration&))
{
    // Arguments:
    //  get            - target, property, receiver        -> Value
    //  set            - target, property, value, receiver -> True (assigned), False (not assigned)
    //  deleteProperty - target, property                  -> True (deleted), False (not deleted)
    //  has            - target, property                  -> True (has), False (not has)
    //  ownKeys        - target                            -> Array of keys
    IsolateImpl *isolateimpl = IsolateFromCtx(ctx);
    Isolate *isolate = ToIsolate(isolateimpl);
    v8::Locker lock(isolate);

    HandleScope scope(isolate);

    *exception = 0;
    assert(argumentCount > 0);
    JSValueRef excp = 0;
    JSObjectRef target = (JSObjectRef) arguments[0];
    JSStringRef propertyName = 0;
    bool isSymbol = false;
    bool isIndex = false;
    int index = 0;
    auto thread = IsolateImpl::PerThreadData::Get(isolateimpl);

    if (argumentCount > 1) {
        isSymbol = JSValueToBoolean(ctx, exec(ctx, "return typeof _1 === 'symbol'", 1, &arguments[1]));
        if (!isSymbol) {
            propertyName = JSValueToStringCopy(ctx, arguments[1], &excp);
        } else {
            JSValueRef args[] = {
                arguments[1],
                isolateimpl->m_private_symbol
            };
            if (JSValueToBoolean(ctx, exec(ctx, "return _1 === _2", 2, args))) {
                return NULL;
            }
        }
        assert(excp==0);
    } else {
        propertyName = JSStringCreateWithUTF8CString("NONE");
    }
    if (!isSymbol) {
        size_t size = JSStringGetMaximumUTF8CStringSize(propertyName);
        char property[size];
        JSStringGetUTF8CString(propertyName, property, size);
        char *p = nullptr;
        index = strtod(property, &p);
        if (p && (!strcmp(p, "constructor") || !strcmp(p, "__proto__") )) {
            return NULL;
        }
        if (!p || *p==0) isIndex = true;
    }

    int receiver_loc = std::is_same<I,InterceptorGetter>::value ? 2 : std::is_same<I,InterceptorSetter>::value ? 3 : 0;

    JSValueRef value;
    if (argumentCount > 2) {
        value = arguments[2];
    } else {
        value = JSValueMakeUndefined(ctx);
    }
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, target);
    
    auto templ = ToImpl<V82JSC::ObjectTemplate>(wrap->m_object_template.Get(isolate));
    Local<v8::Context> context = LocalContext::New(ToIsolate(isolateimpl), ctx);
    v8::Context::Scope context_scope(context);
    auto ctximpl = ToContextImpl(context);
    Local<v8::Value> holder = V82JSC::Value::New(ctximpl, wrap->m_proxy_security);

#ifdef USE_JAVASCRIPTCORE_PRIVATE_API
    JSGlobalContextRef creation_context = JSCPrivate::JSObjectGetGlobalContext(target);
#else
    JSGlobalContextRef creation_context = JSContextGetGlobalContext(ctx);
#endif
    
    bool ok = wrap->m_isGlobalObject && creation_context == JSContextGetGlobalContext(ctx);
    if (!ok && templ->m_access_check) {
        ok = templ->m_access_check(context,
                                   V82JSC::Value::New(ctximpl, target).As<Object>(),
                                   V82JSC::Value::New(ctximpl, templ->m_access_check_data));
    } else {
        ok = true;
    }
    
    Local<v8::Value> data;
    if (!ok) {
        if (isSymbol || !isIndex) { /* Is named */
            data = V82JSC::Value::New(ctximpl, templ->m_failed_named_data);
        } else { /* Is Indexed */
            data = V82JSC::Value::New(ctximpl, templ->m_failed_indexed_data);
        }
    } else {
        if (isSymbol || !isIndex) { /* Is named */
            data = V82JSC::Value::New(ctximpl, templ->m_named_data);
        } else { /* Is Indexed */
            data = V82JSC::Value::New(ctximpl, templ->m_indexed_data);
        }
    }

    ++ thread->m_callback_depth;

    Local<v8::Value> thiz = V82JSC::Value::New(ctximpl, arguments[receiver_loc]);
    typedef v8::internal::Heap::RootListIndex R;
    internal::Object *the_hole = isolateimpl->ii.heap()->root(R::kTheHoleValueRootIndex);
    
    // FIXME: I can think of no way to determine whether we were called from strict mode or not
    bool isStrict = false;
    internal::Object *shouldThrow = internal::Smi::FromInt(isStrict?1:0);

    v8::internal::Object * implicit[] = {
        shouldThrow,                                         // kShouldThrowOnErrorIndex = 0;
        * reinterpret_cast<v8::internal::Object**>(*holder), // kHolderIndex = 1;
        O(isolateimpl),                                      // kIsolateIndex = 2;
        the_hole,                                            // kReturnValueDefaultValueIndex = 3;
        the_hole,                                            // kReturnValueIndex = 4;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 5;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kThisIndex = 6;
    };
    
    PropertyCallback<V> info(implicit);
    Local<v8::Value> set = V82JSC::Value::New(ctximpl, value);
    
    thread->m_scheduled_exception = the_hole;
    TryCatch try_catch(ToIsolate(isolateimpl));

    if (isSymbol || !isIndex) {
        Local<Name> prop = Local<Name>();
        if (argumentCount>1) {
            prop = V82JSC::Value::New(ctximpl, arguments[1]).As<Name>();
        }
        named_handler(templ, prop, set, info, ok ? templ->m_named_handler : templ->m_named_failed_access_handler);
    } else {
        indexed_handler(templ, index, set, info, ok ? templ->m_indexed_handler : templ->m_indexed_failed_access_handler);
    }
    
    if (try_catch.HasCaught()) {
        *exception = ToJSValueRef(try_catch.Exception(), context);
    } else if (thread->m_scheduled_exception != the_hole) {
        internal::Object * excep = thread->m_scheduled_exception;
        *exception = ToJSValueRef_<v8::Value>(excep, context);
        thread->m_scheduled_exception = the_hole;
    }

    -- thread->m_callback_depth;
    
    if (implicit[4] == the_hole) {
        return NULL;
    }
    
    Local<v8::Value> retVal = info.GetReturnValue().Get();
    
    return ToJSValueRef<v8::Value>(retVal, context);
}

#define NAMED_PARAMS(R) const V82JSC::ObjectTemplate* impl, Local<Name> property, Local<v8::Value> value, \
    PropertyCallbackInfo<R>& info, const NamedPropertyHandlerConfiguration& config
#define INDEXED_PARAMS(R) const V82JSC::ObjectTemplate* impl, uint32_t index, Local<v8::Value> value, \
    PropertyCallbackInfo<R>& info, const IndexedPropertyHandlerConfiguration& config

static inline bool inGlobalPrototypeChain(JSContextRef ctx, JSObjectRef obj)
{
    JSObjectRef global = JSContextGetGlobalObject(ctx);
    while (JSValueIsObject(ctx, global)) {
        if (JSValueIsStrictEqual(ctx, global, obj)) return true;
        global = (JSObjectRef) JSObjectGetPrototype(ctx, global);
    }
    return false;
}

static JSValueRef proxy_get(CALLBACK_PARAMS)
{
    JSValueRef ret = PropertyHandler<v8::Value,InterceptorGetter>
    (
     PASS, [](NAMED_PARAMS(v8::Value)) { config.getter(property, info); },
     [](INDEXED_PARAMS(v8::Value)) { config.getter(index, info); }
     );
    if (ret == NULL && !*exception) {
        // Not handled.  Pass thru.
        assert(argumentCount>1);
        // If the receiver is not the proxy, do the 'get' via the prototype so that any
        // signature checks can be maintained properly
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
        if (!JSValueIsStrictEqual(ctx, wrap->m_proxy_security, arguments[2])) {
            JSObjectRef temp1 = JSObjectMake(ctx, 0, 0);
            JSObjectSetPrototype(ctx, temp1, arguments[0]);
            JSValueRef args[] = {
                temp1,
                arguments[1]
            };
            return exec(ctx, "return _1[_2]", 2, args, exception);
        }
        
        return exec(ctx, "return _1[_2]", 2, arguments, exception);
    }
    return ret;
}

static JSValueRef legacy_proxy_get(JSContextRef ctx, JSObjectRef object,
                                   JSStringRef propertyName, JSValueRef* exception)
{
    if (JSStringGetLength(propertyName) == 0 || !inGlobalPrototypeChain(ctx, object)) return NULL;
    JSValueRef args[] = {
        object, JSValueMakeString(ctx, propertyName), object
    };
    JSValueRef ret = PropertyHandler<v8::Value,InterceptorGetter>
    (
     ctx, object, object, 3, args, exception,
     [](NAMED_PARAMS(v8::Value)) { config.getter(property, info); },
     [](INDEXED_PARAMS(v8::Value)) { config.getter(index, info); }
     );
    return ret;
}

static JSValueRef proxy_set(CALLBACK_PARAMS)
{
    JSValueRef ret = PropertyHandler<v8::Value,InterceptorSetter>
    (PASS, [](NAMED_PARAMS(v8::Value)) { config.setter(property, value, info); },
     [](INDEXED_PARAMS(v8::Value)) { config.setter(index, value, info); }
     );
    if (*exception) {
        return JSValueMakeBoolean(ctx, false);
    }
    if (ret == NULL) {
        assert(argumentCount>2);
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
        assert(wrap);
        // If the receiver is not the proxy, do the 'set' via the prototype so that any
        // signature checks can be maintained properly
        if (!JSValueIsStrictEqual(ctx, wrap->m_proxy_security, arguments[3])) {
            JSValueRef args[] = {
                arguments[0],
                arguments[1],
                arguments[2],
                arguments[3],
                0
            };
            args[4] = JSObjectMake(ctx, 0, 0);
            JSObjectSetPrototype(ctx, (JSObjectRef)args[4], arguments[0]);
            return exec(ctx, "_5[_2] = _3; return _1[_2] = _3", 5, args, exception);
        }
        return exec(ctx, "return _1[_2] = _3", 3, arguments, exception);
    }
    return JSValueMakeBoolean(ctx, true);
}

static bool legacy_proxy_set(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
                             JSValueRef value, JSValueRef* exception)
{
    if (JSStringGetLength(propertyName) == 0 || !inGlobalPrototypeChain(ctx, object)) return NULL;
    JSValueRef args[] = {
        object, JSValueMakeString(ctx, propertyName), value, object
    };
    JSValueRef ret = PropertyHandler<v8::Value,InterceptorSetter>
    (ctx, object, object, 4, args, exception,
     [](NAMED_PARAMS(v8::Value)) { config.setter(property, value, info); },
     [](INDEXED_PARAMS(v8::Value)) { config.setter(index, value, info); }
     );
    if (*exception || ret == NULL) {
        return false;
    }
    
    return true;
}

static JSValueRef proxy_has(CALLBACK_PARAMS)
{
    JSValueRef ret = PropertyHandler<Integer,InterceptorOther>
    (PASS,
     [](NAMED_PARAMS(Integer)) {
         if (config.query != NullNamedQuery) config.query(property, info);
         else if(config.getter != NullNamedGetter) info.GetReturnValue().Set(v8::PropertyAttribute::None);
     },
     [](INDEXED_PARAMS(Integer)) {
         if (config.query != NullIndexedQuery) config.query(index, info);
         else if(config.getter != NullIndexedGetter) info.GetReturnValue().Set(v8::PropertyAttribute::None);
     }
     );
    if (*exception) {
        return JSValueMakeBoolean(ctx, false);
    }
    if (ret == NULL) {
        assert(argumentCount>1);
        return exec(ctx, "return _1.hasOwnProperty(_2)", 2, arguments, exception);
    }
    return JSValueMakeBoolean(ctx, !JSValueIsUndefined(ctx, ret));
}

static bool legacy_proxy_has(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    if (JSStringGetLength(propertyName) == 0 || !inGlobalPrototypeChain(ctx, object)) return NULL;
    JSValueRef args[] = {
        object, JSValueMakeString(ctx, propertyName)
    };
    JSValueRef exception = 0;
    JSValueRef ret = PropertyHandler<Integer,InterceptorOther>
    (ctx, object, object, 2, args, &exception,
     [](NAMED_PARAMS(Integer)) {
         if (config.query != NullNamedQuery) config.query(property, info);
         else if(config.getter != NullNamedGetter) config.getter(property, reinterpret_cast<PropertyCallbackInfo<v8::Value>&>(info));
     },
     [](INDEXED_PARAMS(Integer)) {
         if (config.query != NullIndexedQuery) config.query(index, info);
         else if(config.getter != NullIndexedGetter) config.getter(index, reinterpret_cast<PropertyCallbackInfo<v8::Value>&>(info));
     }
     );
    if (exception || ret == NULL) {
        return false;
    }
    return true;
}

static JSValueRef proxy_deleteProperty(CALLBACK_PARAMS)
{
    JSValueRef ret = PropertyHandler<v8::Boolean,InterceptorOther>
    (PASS, [](NAMED_PARAMS(v8::Boolean)) { config.deleter(property, info); },
     [](INDEXED_PARAMS(v8::Boolean)) { config.deleter(index, info); });
    if (!*exception && ret == NULL) {
        assert(argumentCount>1);
        return exec(ctx, "return delete _1[_2]", 2, arguments, exception);
    }
    return ret;
}

static bool legacy_proxy_deleteProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
                                        JSValueRef* exception)
{
    if (JSStringGetLength(propertyName) == 0 || !inGlobalPrototypeChain(ctx, object)) return NULL;
    JSValueRef args[] = {
        object, JSValueMakeString(ctx, propertyName)
    };
    JSValueRef ret = PropertyHandler<v8::Boolean,InterceptorOther>
    (ctx, object, object, 2, args, exception,
     [](NAMED_PARAMS(v8::Boolean)) { config.deleter(property, info); },
     [](INDEXED_PARAMS(v8::Boolean)) { config.deleter(index, info); });
    if (ret==NULL || *exception) {
        return false;
    }
    return true;
}

static JSValueRef proxy_ownKeys(CALLBACK_PARAMS)
{
    JSValueRef ret = PropertyHandler<v8::Array,InterceptorOther>
    (PASS, [](NAMED_PARAMS(v8::Array)) { config.enumerator(info); },
     [](INDEXED_PARAMS(v8::Array)) { config.enumerator(info); });
    if (!*exception && ret == NULL) {
        IsolateImpl *iso = IsolateFromCtx(ctx);
        assert(argumentCount>0);
        JSValueRef args[] = {
            arguments[0],
            iso->m_private_symbol
        };
        return exec(ctx,
                            "return Array.from(new Set("
                            "  Object.getOwnPropertyNames(_1)"
                            "    .concat(Object.getOwnPropertySymbols(_1))"
                            ")).filter(p => p!==_2)",
                            2, args, exception);
    }
    return ret;
}

static void legacy_proxy_ownKeys(JSContextRef ctx, JSObjectRef object,
                                 JSPropertyNameAccumulatorRef acc)
{
    if (!inGlobalPrototypeChain(ctx, object)) return;
    JSValueRef exception = 0;
    JSValueRef ret = PropertyHandler<v8::Array,InterceptorOther>
    (ctx, object, object, 1, &object, &exception,
     [](NAMED_PARAMS(v8::Array)) { config.enumerator(info); },
     [](INDEXED_PARAMS(v8::Array)) { config.enumerator(info); });
    if (!exception && ret) {
        int length = static_cast<int>(JSValueToNumber(ctx, exec(ctx, "_1.length", 1, &ret), 0));
        for (int i=0; !exception && i<length; i++) {
            JSValueRef name = JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)ret, i, &exception);
            JSStringRef s = JSValueToStringCopy(ctx, name, 0);
            if (JSStringGetLength(s)) {
                JSPropertyNameAccumulatorAddName(acc, s);
            }
        }
    }
}

static JSValueRef proxy_defineProperty(CALLBACK_PARAMS)
{
    assert(argumentCount>2);
    JSValueRef ret = PropertyHandler<v8::Value,InterceptorOther>
    (PASS, [](NAMED_PARAMS(v8::Value)) { config.definer(property, value, info); },
     [](INDEXED_PARAMS(v8::Value)) { config.definer(index, value, info); });
    if (!*exception && ret == NULL) {
        return exec(ctx, "return Object.defineProperty(_1, _2, _3)", 3, arguments, exception);
    }
    return ret;
}

static JSValueRef proxy_getPrototypeOf(CALLBACK_PARAMS)
{
    assert(argumentCount>0);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
    Isolate *isolate = ToIsolate(IsolateFromCtx(ctx));
    HandleScope scope(isolate);
    Local<v8::Context> context = LocalContext::New(isolate, ctx);
    auto templ = ToImpl<V82JSC::ObjectTemplate>(wrap->m_object_template.Get(isolate));
    if (templ->m_access_check)
    {
        return JSValueMakeNull(ctx);
    }
    Local<v8::Value> proto = V82JSC::Value::New(ToContextImpl(context),
                                        arguments[0]).As<Object>()->GetPrototype();
    return ToJSValueRef(proto, context);
}

static JSValueRef proxy_setPrototypeOf(CALLBACK_PARAMS)
{
    assert(argumentCount>1);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
    Isolate *isolate = ToIsolate(IsolateFromCtx(ctx));
    HandleScope scope(isolate);
    Local<v8::Context> context = LocalContext::New(isolate, ctx);
    auto templ = ToImpl<V82JSC::ObjectTemplate>(wrap->m_object_template.Get(isolate));
    if (templ->m_access_check && !templ->m_access_check(context,
                                                        V82JSC::Value::New(ToContextImpl(context), arguments[0]).As<Object>(),
                                                        V82JSC::Value::New(ToContextImpl(context), templ->m_access_check_data)))
    {
        isolate->ThrowException(Exception::TypeError(v8::String::NewFromUtf8(isolate, "access denied",
                                                                         NewStringType::kNormal).ToLocalChecked()));
        
    }
    Maybe<bool> r = V82JSC::Value::New(ToContextImpl(context),arguments[0])
    .As<Object>()->SetPrototype(context, V82JSC::Value::New(ToContextImpl(context), arguments[1]));
    return JSValueMakeBoolean(ctx, r.FromJust());
}

static JSValueRef proxy_getOwnPropertyDescriptor(CALLBACK_PARAMS)
{
    assert(argumentCount>1);
    // First, try a descriptor interceptor
    JSValueRef descriptor = PropertyHandler<v8::Value,InterceptorOther>
    (PASS, [](NAMED_PARAMS(v8::Value)) { config.descriptor(property, info); },
     [](INDEXED_PARAMS(v8::Value)) { config.descriptor(index, info); });
    if (descriptor) return descriptor;
    if (exception && *exception) return NULL;
    
    // Second, see if we have a real property descriptor
    descriptor = exec(ctx, "return Object.getOwnPropertyDescriptor(_1, _2)", 2, arguments, exception);
    if (descriptor && !JSValueIsStrictEqual(ctx, descriptor, JSValueMakeUndefined(ctx))) return descriptor;
    if (exception && *exception) return NULL;
    
    // Third, try calling the querier to see if the property exists
    JSValueRef attributes = PropertyHandler<Integer,InterceptorOther>
    (PASS,
     [](NAMED_PARAMS(Integer)) {
         if (config.query != NullNamedQuery) config.query(property, info);
         else if(config.getter != NullNamedGetter) info.GetReturnValue().Set(-1);
     },
     [](INDEXED_PARAMS(Integer)) {
         if (config.query != NullIndexedQuery) config.query(index, info);
         else if(config.getter != NullIndexedGetter) info.GetReturnValue().Set(-1);
     }
     );
    if (exception && *exception) return NULL;
    
    // attributes can be NULL (has querier, property does not exist), -1 (no querier, defer to value), PropertyAttribute (has property)
    if (attributes == NULL) {
        return JSValueMakeUndefined(ctx);
    }
    int pattr = static_cast<int>(JSValueToNumber(ctx, attributes, 0));
    
    // Finally, check the getter to see if we should claim a value
    JSValueRef value = PropertyHandler<v8::Value,InterceptorOther>
    (
     PASS, [](NAMED_PARAMS(v8::Value)) { config.getter(property, info); },
     [](INDEXED_PARAMS(v8::Value)) { config.getter(index, info); }
     );
    if (exception && *exception) return NULL;
    
    if (pattr != -1 || value != NULL) {
        v8::PropertyAttribute attr = PropertyAttribute::None;
        if (pattr != -1) {
            attr = static_cast<v8::PropertyAttribute>(pattr);
        }
        JSValueRef args[] = {
            JSValueMakeBoolean(ctx, !(attr & v8::PropertyAttribute::ReadOnly)),
            JSValueMakeBoolean(ctx, !(attr & PropertyAttribute::DontEnum)),
            value
        };
        if (value != NULL) {
            return exec(ctx, "return { writable: _1, enumerable: _2, configurable: true, value: _3 }", 3, args);
        } else {
            return exec(ctx, "return { writable: _1, enumerable: _2, configurable: true }", 2, args);
        }
    }
    
    // No property
    return JSValueMakeUndefined(ctx);
}

v8::MaybeLocal<v8::Object> V82JSC::ObjectTemplate::NewInstance(v8::Local<v8::Context> context,
                                                           JSObjectRef root, bool isHiddenPrototype,
                                                           JSClassDefinition* definition,
                                                           void *data)
{
    auto ctx = ToContextImpl(context);
    IsolateImpl* iso = ToIsolateImpl(ctx);
    Isolate* isolate = ToIsolate(iso);
    
    EscapableHandleScope scope(isolate);
    
    LocalException exception(iso);
    Local<v8::ObjectTemplate> thiz = CreateLocal<v8::ObjectTemplate>(isolate, this);
    
    TrackedObject *wrap;
    
    if (definition) {
        if (m_need_proxy) {
            definition->getProperty = legacy_proxy_get;
            definition->setProperty = legacy_proxy_set;
            definition->hasProperty = legacy_proxy_has;
            definition->deleteProperty = legacy_proxy_deleteProperty;
            definition->getPropertyNames = legacy_proxy_ownKeys;
        }
        JSClassRef klass = JSClassCreate(definition);
        root = JSObjectMake(ctx->m_ctxRef, klass, data);
        JSClassRelease(klass);
    }
    assert(root);
    wrap = V82JSC::TrackedObject::makePrivateInstance(iso, ctx->m_ctxRef, root);
    
    // Structure:
    //
    // proxy -----> root . [[PrivateSymbol]] -->  lifecycle_object(wrap) --> TrackedObjectImpl*
    
    // Create lifecycle object
    wrap->m_object_template.Reset(isolate, thiz);
    wrap->m_num_internal_fields = m_internal_fields;
    JSValueRef initarray[m_internal_fields];
    for (int i=0; i<m_internal_fields; i++) {
        initarray[i] = JSValueMakeUndefined(ctx->m_ctxRef);
    }
    wrap->m_internal_fields_array = JSObjectMakeArray(ctx->m_ctxRef, m_internal_fields, initarray, 0);
    JSValueProtect(ctx->m_ctxRef, wrap->m_internal_fields_array);
    wrap->m_isHiddenPrototype = isHiddenPrototype;

    // Create proxy
    JSObjectRef handler = 0;
    if (m_need_proxy && !wrap->m_isGlobalObject) {
        handler = JSObjectMake(ctx->m_ctxRef, nullptr, nullptr);
        auto handler_func = [ctx, handler](const char *name, JSObjectCallAsFunctionCallback callback) -> void {
            JSValueRef excp = 0;
            JSStringRef sname = JSStringCreateWithUTF8CString(name);
            JSObjectRef f = JSObjectMakeFunctionWithCallback(ctx->m_ctxRef, sname, callback);
            JSObjectSetProperty(ctx->m_ctxRef, handler, sname, f, 0, &excp);
            JSStringRelease(sname);
            assert(excp==0);
        };
        
        handler_func("get", proxy_get);
        handler_func("set", proxy_set);
        handler_func("has", proxy_has);
        handler_func("deleteProperty", proxy_deleteProperty);
        handler_func("ownKeys", proxy_ownKeys);
        handler_func("defineProperty", proxy_defineProperty);
        handler_func("getPrototypeOf", proxy_getPrototypeOf);
        handler_func("setPrototypeOf", proxy_setPrototypeOf);
        handler_func("getOwnPropertyDescriptor", proxy_getOwnPropertyDescriptor);
    }

    MaybeLocal<Object> instance;
    if (!m_constructor_template.IsEmpty()) {
        instance = reinterpret_cast<Template*>(this)->
        InitInstance(context, root, exception, m_constructor_template.Get(isolate));
    } else {
        instance = reinterpret_cast<Template*>(this)->InitInstance(context, root, exception);
    }
    if (instance.IsEmpty()) {
        return instance;
    }

    if (m_need_proxy) {
        JSValueRef args[] = {root, handler};
        JSValueRef proxy_object = exec(ctx->m_ctxRef, "return new Proxy(_1, _2)", 2, args);
        // Important!  Set the security proxy before calling ValueImpl::New().  We don't want the proxy object
        // to have its own wrap
        wrap->m_proxy_security = proxy_object;
        Local<Object> proxy = V82JSC::Value::New(ctx, proxy_object).As<Object>();
        instance = proxy;
    }
    
    if (isHiddenPrototype) {
        const char* proxy_code =
        "const handler = {"
        "    set(target,prop,val,receiver) {"
        "        var d = Object.getOwnPropertyDescriptor(target, prop);"
        "        var exists = d !== undefined;"
        "        var r = (target[prop] = val);"
        "        if (!exists) {"
        "            _2(target, prop, receiver);"
        "        }"
        "        return r;"
        "    },"
        "    deleteProperty(target,prop) {"
        "        var d = Object.getOwnPropertyDescriptor(target, prop);"
        "        var exists = d !== undefined;"
        "        var r = delete target[prop];"
        "        if (exists && r) {"
        "            _3(target, prop);"
        "        }"
        "        return r;"
        "    },"
        "    defineProperty(target,prop,desc) {"
        "        var d = Object.getOwnPropertyDescriptor(target, prop);"
        "        var exists = d !== undefined;"
        "        try {"
        "            Object.defineProperty(target, prop, desc);"
        "        } catch (e) {"
        "            return false;"
        "        }"
        "        if (!exists) {"
        "            _2(target, prop);"
        "        }"
        "        return true;"
        "    }"
        "};"
        "return new Proxy(_1, handler);";
        
        JSStringRef sname = JSStringCreateWithUTF8CString("propagate_set");
        JSObjectRef propagate_set = JSObjectMakeFunctionWithCallback(ctx->m_ctxRef, sname, [](CALLBACK_PARAMS) -> JSValueRef {
            auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
            assert(wrap && wrap->m_hidden_proxy_security);
            Isolate *isolate = ToIsolate(IsolateFromCtx(ctx));
            HandleScope scope(isolate);
            Local<v8::Context> context = LocalContext::New(isolate, ctx);
            Local<Name> property = V82JSC::Value::New(ToContextImpl(context), arguments[1]).As<Name>();
            if (JSValueIsStrictEqual(ctx, arguments[2], JSValueMakeUndefined(ctx)) ||
                JSValueIsStrictEqual(ctx, arguments[2], wrap->m_hidden_proxy_security)) {
                
                ToImpl<V82JSC::HiddenObject>(V82JSC::Value::New(ToContextImpl(context), arguments[0]))
                    ->PropagateOwnPropertyToChildren(context, property);
            }
            return JSValueMakeUndefined(ctx);
        });
        JSStringRelease(sname);
        sname = JSStringCreateWithUTF8CString("propagate_delete");
        JSObjectRef propagate_delete = JSObjectMakeFunctionWithCallback(ctx->m_ctxRef, sname, [](CALLBACK_PARAMS) -> JSValueRef {
            assert(0); // FIXME!  We need to propagate deletes
            return NULL;
        });
        JSStringRelease(sname);
        JSValueRef args[] = {
            ToJSValueRef(instance.ToLocalChecked(), context),
            propagate_set,
            propagate_delete
        };
        JSValueRef hidden_proxy_object = exec(ctx->m_ctxRef, proxy_code, 3, args);
        // Same here.  Set the hidden proxy reference before calling ValueImpl::New()
        wrap->m_hidden_proxy_security = hidden_proxy_object;
        Local<Object> hidden_proxy = V82JSC::Value::New(ctx, hidden_proxy_object).As<Object>();
        instance = hidden_proxy;
    }
    
    return scope.Escape(V82JSC::TrackedObject::SecureValue(instance.ToLocalChecked()).As<Object>());
}

/**
 * Sets an accessor on the object template.
 *
 * Whenever the property with the given name is accessed on objects
 * created from this ObjectTemplate the getter and setter callbacks
 * are called instead of getting and setting the property directly
 * on the JavaScript object.
 *
 * \param name The name of the property for which an accessor is added.
 * \param getter The callback to invoke when getting the property.
 * \param setter The callback to invoke when setting the property.
 * \param data A piece of data that will be passed to the getter and setter
 *   callbacks whenever they are invoked.
 * \param settings Access control settings for the accessor. This is a bit
 *   field consisting of one of more of
 *   DEFAULT = 0, ALL_CAN_READ = 1, or ALL_CAN_WRITE = 2.
 *   The default is to not allow cross-context access.
 *   ALL_CAN_READ means that all cross-context reads are allowed.
 *   ALL_CAN_WRITE means that all cross-context writes are allowed.
 *   The combination ALL_CAN_READ | ALL_CAN_WRITE can be used to allow all
 *   cross-context access.
 * \param attribute The attributes of the property for which an accessor
 *   is added.
 * \param signature The signature describes valid receivers for the accessor
 *   and is used to perform implicit instance checks against them. If the
 *   receiver is incompatible (i.e. is not an instance of the constructor as
 *   defined by FunctionTemplate::HasInstance()), an implicit TypeError is
 *   thrown and no callback is invoked.
 */
void v8::ObjectTemplate::SetAccessor(
                 Local<v8::String> name, AccessorGetterCallback getter,
                 AccessorSetterCallback setter, Local<v8::Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    SetAccessor(name.As<Name>(),
                reinterpret_cast<AccessorNameGetterCallback>(getter),
                reinterpret_cast<AccessorNameSetterCallback>(setter),
                data, settings, attribute, signature);
}
void v8::ObjectTemplate::SetAccessor(
                 Local<Name> name, AccessorNameGetterCallback getter,
                 AccessorNameSetterCallback setter, Local<Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    auto this_ = ToImpl<V82JSC::ObjectTemplate,v8::ObjectTemplate>(this);
    Isolate* isolate = ToIsolate(ToIsolateImpl(this_));
    HandleScope scope(isolate);
    
    auto accessor = static_cast<V82JSC::ObjAccessor *>
    (HeapAllocator::Alloc(ToIsolateImpl(this_), ToIsolateImpl(this_)->m_object_accessor_map));
    
    accessor->name.Reset(isolate, name);
    accessor->getter = getter;
    accessor->setter = setter ? setter :
    [](Local<Name> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
        info.GetReturnValue().Set(Undefined(info.GetIsolate()));
    };
    accessor->data.Reset(isolate, data);
    accessor->settings = settings;
    accessor->attribute = attribute;
    
    // For now, Signature and AccessorSignature are the same
    Local<Signature> sig = * reinterpret_cast<Local<Signature>*>(&signature);
    
    accessor->signature.Reset(isolate, sig);
    
    Local<v8::ObjAccessor> local = CreateLocal<v8::ObjAccessor>(isolate, accessor);
    accessor->next_.Reset(isolate, this_->m_accessors.Get(isolate));
    this_->m_accessors.Reset(isolate, local);
}

/**
 * Sets a named property handler on the object template.
 *
 * Whenever a property whose name is a string is accessed on objects created
 * from this object template, the provided callback is invoked instead of
 * accessing the property directly on the JavaScript object.
 *
 * SetNamedPropertyHandler() is different from SetHandler(), in
 * that the latter can intercept symbol-named properties as well as
 * string-named properties when called with a
 * NamedPropertyHandlerConfiguration. New code should use SetHandler().
 *
 * \param getter The callback to invoke when getting a property.
 * \param setter The callback to invoke when setting a property.
 * \param query The callback to invoke to check if a property is present,
 *   and if present, get its attributes.
 * \param deleter The callback to invoke when deleting a property.
 * \param enumerator The callback to invoke to enumerate all the named
 *   properties of an object.
 * \param data A piece of data that will be passed to the callbacks
 *   whenever they are invoked.
 */
// TODO(dcarney): deprecate
void v8::ObjectTemplate::SetNamedPropertyHandler(NamedPropertyGetterCallback getter,
                             NamedPropertySetterCallback setter,
                             NamedPropertyQueryCallback query,
                             NamedPropertyDeleterCallback deleter,
                             NamedPropertyEnumeratorCallback enumerator,
                             Local<Value> data)
{
    // FIXME: This is a nasty hack.
    NamedPropertyHandlerConfiguration config;
    config.getter = reinterpret_cast<GenericNamedPropertyGetterCallback>(getter);
    config.setter = reinterpret_cast<GenericNamedPropertySetterCallback>(setter);
    config.query = reinterpret_cast<GenericNamedPropertyQueryCallback>(query);
    config.deleter = reinterpret_cast<GenericNamedPropertyDeleterCallback>(deleter);
    config.enumerator = reinterpret_cast<GenericNamedPropertyEnumeratorCallback>(enumerator);
    config.data = data;
    SetHandler(config);
}

/**
 * Sets a named property handler on the object template.
 *
 * Whenever a property whose name is a string or a symbol is accessed on
 * objects created from this object template, the provided callback is
 * invoked instead of accessing the property directly on the JavaScript
 * object.
 *
 * @param configuration The NamedPropertyHandlerConfiguration that defines the
 * callbacks to invoke when accessing a property.
 */
void v8::ObjectTemplate::SetHandler(const NamedPropertyHandlerConfiguration& configuration)
{
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    HandleScope scope(ToIsolate(templ->GetIsolate()));
    
    Local<Value> data = configuration.data;
    if (configuration.getter) templ->m_named_handler.getter = configuration.getter;
    if (configuration.setter) templ->m_named_handler.setter = configuration.setter;
    if (configuration.descriptor) templ->m_named_handler.descriptor = configuration.descriptor;
    if (configuration.deleter) templ->m_named_handler.deleter = configuration.deleter;
    if (configuration.enumerator) templ->m_named_handler.enumerator = configuration.enumerator;
    if (configuration.definer) templ->m_named_handler.definer = configuration.definer;
    if (configuration.query) templ->m_named_handler.query = configuration.query;

    templ->m_named_handler.data.Clear();

    if (data.IsEmpty()) {
        data = Undefined(Isolate::GetCurrent());
    }
    templ->m_named_data = ToJSValueRef(configuration.data, Isolate::GetCurrent());
    JSValueProtect(ToContextRef(Isolate::GetCurrent()), templ->m_named_data);
    templ->m_need_proxy = true;
}

/**
 * Sets an indexed property handler on the object template.
 *
 * Whenever an indexed property is accessed on objects created from
 * this object template, the provided callback is invoked instead of
 * accessing the property directly on the JavaScript object.
 *
 * @param configuration The IndexedPropertyHandlerConfiguration that defines
 * the callbacks to invoke when accessing a property.
 */
void v8::ObjectTemplate::SetHandler(const IndexedPropertyHandlerConfiguration& configuration)
{
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    HandleScope scope(ToIsolate(templ->GetIsolate()));

    Local<Value> data = configuration.data;
    if (configuration.getter) templ->m_indexed_handler.getter = configuration.getter;
    if (configuration.setter) templ->m_indexed_handler.setter = configuration.setter;
    if (configuration.descriptor) templ->m_indexed_handler.descriptor = configuration.descriptor;
    if (configuration.deleter) templ->m_indexed_handler.deleter = configuration.deleter;
    if (configuration.enumerator) templ->m_indexed_handler.enumerator = configuration.enumerator;
    if (configuration.definer) templ->m_indexed_handler.definer = configuration.definer;
    if (configuration.query) templ->m_indexed_handler.query = configuration.query;

    templ->m_indexed_handler.data.Clear();

    if (data.IsEmpty()) {
        data = Undefined(Isolate::GetCurrent());
    }
    templ->m_indexed_data = ToJSValueRef(configuration.data, Isolate::GetCurrent());
    JSValueProtect(ToContextRef(Isolate::GetCurrent()), templ->m_indexed_data);
    templ->m_need_proxy = true;
}

/**
 * Sets the callback to be used when calling instances created from
 * this template as a function.  If no callback is set, instances
 * behave like normal JavaScript objects that cannot be called as a
 * function.
 */
void v8::ObjectTemplate::SetCallAsFunctionHandler(FunctionCallback callback,
                              Local<Value> data)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    
    Local<Context> context = ToCurrentContext(this);
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    templ->m_callback = callback;
    if (data.IsEmpty()) {
        data = Undefined(isolate);
    }
    templ->m_data = ToJSValueRef<Value>(data, isolate);
    JSValueProtect(ToContextRef(context), templ->m_data);
}

/**
 * Mark object instances of the template as undetectable.
 *
 * In many ways, undetectable objects behave as though they are not
 * there.  They behave like 'undefined' in conditionals and when
 * printed.  However, properties can be accessed and called as on
 * normal objects.
 */
void v8::ObjectTemplate::MarkAsUndetectable()
{
    printf("V82JSC: Undetectable objects not supported in JSC\n");
}

/**
 * Sets access check callback on the object template and enables access
 * checks.
 *
 * When accessing properties on instances of this object template,
 * the access check callback will be called to determine whether or
 * not to allow cross-context access to the properties.
 */
void v8::ObjectTemplate::SetAccessCheckCallback(AccessCheckCallback callback,
                            Local<Value> data)
{
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    Isolate *isolate = ToIsolate(ToIsolateImpl(templ));
    HandleScope scope(isolate);
    
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    templ->m_access_check = callback;
    if (data.IsEmpty()) {
        data = Undefined(isolate);
    }
    templ->m_access_check_data = ToJSValueRef(data, context);
    JSValueProtect(ctx, templ->m_access_check_data);
}

/**
 * Like SetAccessCheckCallback but invokes an interceptor on failed access
 * checks instead of looking up all-can-read properties. You can only use
 * either this method or SetAccessCheckCallback, but not both at the same
 * time.
 */
void v8::ObjectTemplate::SetAccessCheckCallbackAndHandler(
                                      AccessCheckCallback callback,
                                      const NamedPropertyHandlerConfiguration& named_handler,
                                      const IndexedPropertyHandlerConfiguration& indexed_handler,
                                      Local<Value> data)
{
    SetAccessCheckCallback(callback, data);
    
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    HandleScope scope(ToIsolate(templ->GetIsolate()));

    Local<Value> named_data = named_handler.data;
    if (named_handler.getter) templ->m_named_failed_access_handler.getter = named_handler.getter;
    if (named_handler.setter) templ->m_named_failed_access_handler.setter = named_handler.setter;
    if (named_handler.descriptor) templ->m_named_failed_access_handler.descriptor = named_handler.descriptor;
    if (named_handler.deleter) templ->m_named_failed_access_handler.deleter = named_handler.deleter;
    if (named_handler.enumerator) templ->m_named_failed_access_handler.enumerator = named_handler.enumerator;
    if (named_handler.definer) templ->m_named_failed_access_handler.definer = named_handler.definer;
    if (named_handler.query) templ->m_named_failed_access_handler.query = named_handler.query;
    
    templ->m_named_failed_access_handler.data.Clear();
    
    if (named_data.IsEmpty()) {
        named_data = Undefined(Isolate::GetCurrent());
    } 
    templ->m_failed_named_data = ToJSValueRef(named_data, Isolate::GetCurrent());
    JSValueProtect(ToContextRef(Isolate::GetCurrent()), templ->m_failed_named_data);

    Local<Value> indexed_data = indexed_handler.data;
    if (indexed_handler.getter) templ->m_indexed_failed_access_handler.getter = indexed_handler.getter;
    if (indexed_handler.setter) templ->m_indexed_failed_access_handler.setter = indexed_handler.setter;
    if (indexed_handler.descriptor) templ->m_indexed_failed_access_handler.descriptor = indexed_handler.descriptor;
    if (indexed_handler.deleter) templ->m_indexed_failed_access_handler.deleter = indexed_handler.deleter;
    if (indexed_handler.enumerator) templ->m_indexed_failed_access_handler.enumerator = indexed_handler.enumerator;
    if (indexed_handler.definer) templ->m_indexed_failed_access_handler.definer = indexed_handler.definer;
    if (indexed_handler.query) templ->m_indexed_failed_access_handler.query = indexed_handler.query;
    
    templ->m_indexed_failed_access_handler.data.Clear();
    
    if (indexed_data.IsEmpty()) {
        indexed_data = Undefined(Isolate::GetCurrent());
    }
    templ->m_failed_indexed_data = ToJSValueRef(named_data, Isolate::GetCurrent());
    JSValueProtect(ToContextRef(Isolate::GetCurrent()), templ->m_failed_indexed_data);
}

/**
 * Gets the number of internal fields for objects generated from
 * this template.
 */
int v8::ObjectTemplate::InternalFieldCount()
{
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    return templ->m_internal_fields;
}

/**
 * Sets the number of internal fields for objects generated from
 * this template.
 */
void v8::ObjectTemplate::SetInternalFieldCount(int value)
{
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    templ->m_internal_fields = value > 0 ? value : 0;
}

/**
 * Returns true if the object will be an immutable prototype exotic object.
 */
bool v8::ObjectTemplate::IsImmutableProto()
{
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    return templ->m_is_immutable_proto;
}

/**
 * Makes the ObjectTempate for an immutable prototype exotic object, with an
 * immutable __proto__.
 */
void v8::ObjectTemplate::SetImmutableProto()
{
    auto templ = ToImpl<V82JSC::ObjectTemplate,ObjectTemplate>(this);
    templ->m_is_immutable_proto = true;
}

