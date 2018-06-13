//
//  ObjectTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include "JSObjectRefPrivate.h"
#include <string.h>

using namespace v8;

#define H V82JSC_HeapObject

static GenericNamedPropertyGetterCallback NullNamedGetter =
    [](Local<Name> property, const PropertyCallbackInfo<Value>& info) {};
static GenericNamedPropertySetterCallback NullNamedSetter =
    [](Local<Name> property, Local<Value> value, const PropertyCallbackInfo<Value>& info) {};
static GenericNamedPropertyDescriptorCallback NullNamedDescriptor =
    [](Local<Name> property, const PropertyCallbackInfo<Value>& info) {};
static GenericNamedPropertyDeleterCallback NullNamedDeleter =
    [](Local<Name> property, const PropertyCallbackInfo<v8::Boolean>& info) {};
static GenericNamedPropertyEnumeratorCallback NullNamedEnumerator =
    [](const PropertyCallbackInfo<Array>& info) {};
static GenericNamedPropertyDefinerCallback NullNamedDefiner =
    [](Local<Name> property, const PropertyDescriptor& desc, const PropertyCallbackInfo<Value>& info) {};
static GenericNamedPropertyQueryCallback NullNamedQuery =
    [](Local<Name> property, const PropertyCallbackInfo<Integer>& info) {};

static IndexedPropertyGetterCallback NullIndexedGetter =
    [](uint32_t index, const PropertyCallbackInfo<Value>& info) {};
static IndexedPropertySetterCallback NullIndexedSetter =
    [](uint32_t index, Local<Value> value, const PropertyCallbackInfo<Value>& info) {};
static IndexedPropertyDescriptorCallback NullIndexedDescriptor =
    [](uint32_t index, const PropertyCallbackInfo<Value>& info) {};
static IndexedPropertyDeleterCallback NullIndexedDeleter =
    [](uint32_t index, const PropertyCallbackInfo<v8::Boolean>& info) {};
static IndexedPropertyEnumeratorCallback NullIndexedEnumerator =
    [](const PropertyCallbackInfo<Array>& info) {};
static IndexedPropertyDefinerCallback NullIndexedDefiner =
    [](uint32_t index, const PropertyDescriptor& desc, const PropertyCallbackInfo<Value>& info) {};
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
info.GetIsolate()->ThrowException(Exception::TypeError(String::NewFromUtf8(info.GetIsolate(), "access denied", \
    NewStringType::kNormal).ToLocalChecked()));

struct AccessDeniedNamedHandlers : public NamedPropertyHandlerConfiguration
{
    AccessDeniedNamedHandlers() : NamedPropertyHandlerConfiguration
    (
     [](Local<Name> property, const PropertyCallbackInfo<Value>& info) { THROW_ACCESS_ERROR() },
     [](Local<Name> property, Local<Value> value, const PropertyCallbackInfo<Value>& info) { THROW_ACCESS_ERROR() },
     [](Local<Name> property, const PropertyCallbackInfo<Value>& info) { THROW_ACCESS_ERROR() },
     [](Local<Name> property, const PropertyCallbackInfo<v8::Boolean>& info) { THROW_ACCESS_ERROR() },
     [](const PropertyCallbackInfo<Array>& info) { THROW_ACCESS_ERROR() },
     [](Local<Name> property, const PropertyDescriptor& desc, const PropertyCallbackInfo<Value>& info) { THROW_ACCESS_ERROR() }
     )
    {
        query = [](Local<Name> property, const PropertyCallbackInfo<Integer>& info) { THROW_ACCESS_ERROR() };
    }
};

struct AccessDeniedIndexedHandlers : public IndexedPropertyHandlerConfiguration
{
    AccessDeniedIndexedHandlers() : IndexedPropertyHandlerConfiguration
    (
     [](uint32_t index, const PropertyCallbackInfo<Value>& info) { THROW_ACCESS_ERROR() },
     [](uint32_t index, Local<Value> value, const PropertyCallbackInfo<Value>& info) { THROW_ACCESS_ERROR() },
     [](uint32_t index, const PropertyCallbackInfo<Value>& info) { THROW_ACCESS_ERROR() },
     [](uint32_t index, const PropertyCallbackInfo<v8::Boolean>& info) { THROW_ACCESS_ERROR() },
     [](const PropertyCallbackInfo<Array>& info) { THROW_ACCESS_ERROR() },
     [](uint32_t index, const PropertyDescriptor& desc, const PropertyCallbackInfo<Value>& info) { THROW_ACCESS_ERROR() }
     )
    {
        query = [](uint32_t index, const PropertyCallbackInfo<Integer>& info) { THROW_ACCESS_ERROR() };
    }
};


/** Creates an ObjectTemplate. */
Local<ObjectTemplate> ObjectTemplate::New(
                                 Isolate* isolate,
                                 Local<FunctionTemplate> constructor)
{
    if (!constructor.IsEmpty()) {
        return constructor->InstanceTemplate();
    } else {
        ObjectTemplateImpl *otempl = static_cast<ObjectTemplateImpl*>
            (H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate),
                                     V82JSC::ToIsolateImpl(isolate)->m_object_template_map));

        otempl->m_constructor_template = Copyable(v8::FunctionTemplate)();
        otempl->m_named_data = 0;
        otempl->m_indexed_data = 0;
        otempl->m_named_handler = DefaultNamedHandlers();
        otempl->m_indexed_handler = DefaultIndexedHandlers();
        otempl->m_named_failed_access_handler = AccessDeniedNamedHandlers();
        otempl->m_indexed_failed_access_handler = AccessDeniedIndexedHandlers();

        return V82JSC::CreateLocal<ObjectTemplate>(isolate, otempl);
    }
}

/** Get a template included in the snapshot by index. */
MaybeLocal<ObjectTemplate> ObjectTemplate::FromSnapshot(Isolate* isolate,
                                               size_t index)
{
    assert(0);
    return MaybeLocal<ObjectTemplate>();
}

/** Creates a new instance of this template.*/
MaybeLocal<Object> ObjectTemplate::NewInstance(Local<Context> context)
{
    ObjectTemplateImpl *impl = V82JSC::ToImpl<ObjectTemplateImpl>(this);
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctx);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    Local<ObjectTemplate> thiz = V82JSC::CreateLocal<ObjectTemplate>(&iso->ii, impl);
    
    LocalException exception(iso);
    
    JSObjectRef instance = 0;
    if (!impl->m_constructor_template.IsEmpty()) {
        MaybeLocal<Function> ctor = impl->m_constructor_template.Get(isolate)->GetFunction(context);
        if (!ctor.IsEmpty()) {
            JSValueRef ctor_func = V82JSC::ToJSValueRef(ctor.ToLocalChecked(), context);
            instance = JSObjectCallAsConstructor(ctx->m_ctxRef, (JSObjectRef)ctor_func, 0, 0, &exception);
            return ValueImpl::New(ctx, instance).As<Object>();
        } else {
            return MaybeLocal<Object>();
        }
    } else if (impl->m_callback) {
        JSClassDefinition def = kJSClassDefinitionEmpty;
        if (impl->m_callback) {
            def.callAsFunction = TemplateImpl::callAsFunctionCallback;
            def.callAsConstructor = TemplateImpl::callAsConstructorCallback;
        }
        JSClassRef claz = JSClassCreate(&def);
        void * data = V82JSC::PersistentData<ObjectTemplate>(isolate, thiz);
        def.finalize = [](JSObjectRef obj) {
            void *data = JSObjectGetPrivate(obj);
            V82JSC::ReleasePersistentData<ObjectTemplate>(data);
        };

        instance = JSObjectMake(ctx->m_ctxRef, claz, data);
    } else {
        instance = JSObjectMake(ctx->m_ctxRef, 0, 0);
    }
    return impl->NewInstance(context, instance, false);
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
                           void (*named_handler)(const ObjectTemplateImpl*, Local<Name>, Local<Value>, PropertyCallbackInfo<V>&, const NamedPropertyHandlerConfiguration&),
                           void (*indexed_handler)(const ObjectTemplateImpl*, uint32_t, Local<Value>, PropertyCallbackInfo<V>&, const IndexedPropertyHandlerConfiguration&))
{
    // Arguments:
    //  get            - target, property, receiver        -> Value
    //  set            - target, property, value, receiver -> True (assigned), False (not assigned)
    //  deleteProperty - target, property                  -> True (deleted), False (not deleted)
    //  has            - target, property                  -> True (has), False (not has)
    //  ownKeys        - target                            -> Array of keys
    IsolateImpl *isolateimpl = IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)];
    *exception = 0;
    assert(argumentCount > 0);
    JSValueRef excp = 0;
    JSObjectRef target = (JSObjectRef) arguments[0];
    JSStringRef propertyName = 0;
    bool isSymbol = false;
    bool isIndex = false;
    int index = 0;
    if (argumentCount > 1) {
        isSymbol = JSValueToBoolean(ctx, V82JSC::exec(ctx, "return typeof _1 === 'symbol'", 1, &arguments[1]));
        if (!isSymbol) {
            propertyName = JSValueToStringCopy(ctx, arguments[1], &excp);
        } else {
            JSValueRef args[] = {
                arguments[1],
                isolateimpl->m_private_symbol
            };
            if (JSValueToBoolean(ctx, V82JSC::exec(ctx, "return _1 === _2", 2, args))) {
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

    JSValueRef value;
    if (argumentCount > 2) {
        value = arguments[2];
    } else {
        value = JSValueMakeUndefined(ctx);
    }
    TrackedObjectImpl* wrap = getPrivateInstance(ctx, target);
    Isolate *isolate = V82JSC::ToIsolate(isolateimpl);
    HandleScope scope(isolate);
    
    const ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl>(wrap->m_object_template.Get(isolate));
    Local<Context> context = LocalContextImpl::New(V82JSC::ToIsolate(isolateimpl), ctx);
    Context::Scope context_scope(context);
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    Local<Value> holder = ValueImpl::New(ctximpl, wrap->m_proxy_security);

    JSGlobalContextRef creation_context = JSObjectGetGlobalContext(target);
    
    bool ok = wrap->m_isGlobalObject && creation_context == JSContextGetGlobalContext(ctx);
    if (!ok && templ->m_access_check) {
        ok = templ->m_access_check(context,
                                   ValueImpl::New(ctximpl, target).As<Object>(),
                                   ValueImpl::New(ctximpl, templ->m_access_check_data));
    } else {
        ok = true;
    }
    
    Local<Value> data;
    if (!ok) {
        if (isSymbol || !isIndex) { /* Is named */
            data = ValueImpl::New(ctximpl, templ->m_failed_named_data);
        } else { /* Is Indexed */
            data = ValueImpl::New(ctximpl, templ->m_failed_indexed_data);
        }
    } else {
        if (isSymbol || !isIndex) { /* Is named */
            data = ValueImpl::New(ctximpl, templ->m_named_data);
        } else { /* Is Indexed */
            data = ValueImpl::New(ctximpl, templ->m_indexed_data);
        }
    }

    ++ isolateimpl->m_callback_depth;

    int receiver_loc = std::is_same<I,InterceptorGetter>::value ? 2 : std::is_same<I,InterceptorSetter>::value ? 3 : 0;
    Local<Value> thiz = ValueImpl::New(ctximpl, arguments[receiver_loc]);
    typedef v8::internal::Heap::RootListIndex R;
    internal::Object *the_hole = isolateimpl->ii.heap()->root(R::kTheHoleValueRootIndex);
    
    // FIXME: This doesn't work
    bool isStrict = JSValueToBoolean(ctx, V82JSC::exec(ctx, "return (function() { return !this; })();", 0, nullptr));
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
    
    PropertyCallbackImpl<V> info(implicit);
    Local<Value> set = ValueImpl::New(ctximpl, value);
    
    isolateimpl->ii.thread_local_top()->scheduled_exception_ = the_hole;
    TryCatch try_catch(V82JSC::ToIsolate(isolateimpl));

    if (isSymbol || !isIndex) {
        Local<Name> prop = Local<Name>();
        if (argumentCount>1) {
            prop = ValueImpl::New(ctximpl, arguments[1]).As<Name>();
        }
        named_handler(templ, prop, set, info, ok ? templ->m_named_handler : templ->m_named_failed_access_handler);
    } else {
        indexed_handler(templ, index, set, info, ok ? templ->m_indexed_handler : templ->m_indexed_failed_access_handler);
    }
    
    if (try_catch.HasCaught()) {
        *exception = V82JSC::ToJSValueRef(try_catch.Exception(), context);
    } else if (isolateimpl->ii.thread_local_top()->scheduled_exception_ != the_hole) {
        internal::Object * excep = isolateimpl->ii.thread_local_top()->scheduled_exception_;
        *exception = V82JSC::ToJSValueRef_<Value>(excep, context);
        isolateimpl->ii.thread_local_top()->scheduled_exception_ = the_hole;
    }

    if (implicit[4] == the_hole) {
        if (-- isolateimpl->m_callback_depth == 0 && isolateimpl->m_pending_garbage_collection) {
            isolateimpl->CollectGarbage();
        }
        return NULL;
    }
    
    Local<Value> retVal = info.GetReturnValue().Get();
    if (-- isolateimpl->m_callback_depth == 0 && isolateimpl->m_pending_garbage_collection) {
        isolateimpl->CollectGarbage();
    }
    return V82JSC::ToJSValueRef<Value>(retVal, context);
}

#define NAMED_PARAMS(R) const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, \
    PropertyCallbackInfo<R>& info, const NamedPropertyHandlerConfiguration& config
#define INDEXED_PARAMS(R) const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, \
    PropertyCallbackInfo<R>& info, const IndexedPropertyHandlerConfiguration& config

v8::MaybeLocal<v8::Object> ObjectTemplateImpl::NewInstance(v8::Local<v8::Context> context, JSObjectRef root, bool isHiddenPrototype)
{
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctx);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    
    LocalException exception(iso);
    Local<v8::ObjectTemplate> thiz = V82JSC::CreateLocal<v8::ObjectTemplate>(isolate, this);
    
    // Structure:
    //
    // proxy -----> root . [[PrivateSymbol]] -->  lifecycle_object(wrap) --> TrackedObjectImpl*
    
    // Create lifecycle object
    TrackedObjectImpl *wrap = makePrivateInstance(iso, ctx->m_ctxRef, root);
    wrap->m_object_template.Reset(isolate, thiz);
    wrap->m_num_internal_fields = m_internal_fields;
    wrap->m_internal_fields_array = JSObjectMakeArray(ctx->m_ctxRef, 0, nullptr, 0);
    JSValueProtect(ctx->m_ctxRef, wrap->m_internal_fields_array);
    wrap->m_isHiddenPrototype = isHiddenPrototype;

    // Create proxy
    JSObjectRef handler = 0;
    if (m_need_proxy) {
        handler = JSObjectMake(ctx->m_ctxRef, nullptr, nullptr);
        auto handler_func = [ctx, handler](const char *name, JSObjectCallAsFunctionCallback callback) -> void {
            JSValueRef excp = 0;
            JSStringRef sname = JSStringCreateWithUTF8CString(name);
            JSObjectRef f = JSObjectMakeFunctionWithCallback(ctx->m_ctxRef, sname, callback);
            JSObjectSetProperty(ctx->m_ctxRef, handler, sname, f, 0, &excp);
            JSStringRelease(sname);
            assert(excp==0);
        };
        
        handler_func("get", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Value,InterceptorGetter>
            (
             PASS, [](NAMED_PARAMS(Value)) { config.getter(property, info); },
             [](INDEXED_PARAMS(Value)) { config.getter(index, info); }
            );
            if (ret == NULL && !*exception) {
                // Not handled.  Pass thru.
                assert(argumentCount>1);
                // If the receiver is not the proxy, do the 'get' via the prototype so that any
                // signature checks can be maintained properly
                TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
                if (!JSValueIsStrictEqual(ctx, wrap->m_proxy_security, arguments[2])) {
                    JSObjectRef temp1 = JSObjectMake(ctx, 0, 0);
                    JSObjectSetPrototype(ctx, temp1, arguments[0]);
                    JSValueRef args[] = {
                        temp1,
                        arguments[1]
                    };
                    return V82JSC::exec(ctx, "return _1[_2]", 2, args, exception);
                }
                
                return V82JSC::exec(ctx, "return _1[_2]", 2, arguments, exception);
            }
            return ret;
        });
        handler_func("set", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Value,InterceptorSetter>
            (PASS, [](NAMED_PARAMS(Value)) { config.setter(property, value, info); },
             [](INDEXED_PARAMS(Value)) { config.setter(index, value, info); }
            );
            if (*exception) {
                return JSValueMakeBoolean(ctx, false);
            }
            if (ret == NULL) {
                assert(argumentCount>2);
                TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
                assert(wrap);
                // If the receiver is not the proxy, do the 'set' via the prototype so that any
                // signature checks can be maintained properly
                if (!JSValueIsStrictEqual(ctx, wrap->m_proxy_security, arguments[3])) {
                    JSObjectRef temp1 = JSObjectMake(ctx, 0, 0);
                    JSObjectSetPrototype(ctx, temp1, arguments[0]);
                    JSValueRef args[] = {
                        arguments[0],
                        arguments[1],
                        arguments[2],
                        temp1
                    };
                    return V82JSC::exec(ctx, "_4[_2] = _3; return _1[_2] = _3", 4, args, exception);
                }
                return V82JSC::exec(ctx, "return _1[_2] = _3", 3, arguments, exception);
            }
            return JSValueMakeBoolean(ctx, true);
        });
        handler_func("has", [](CALLBACK_PARAMS) -> JSValueRef
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
                return V82JSC::exec(ctx, "return _1.hasOwnProperty(_2)", 2, arguments, exception);
            }
            return JSValueMakeBoolean(ctx, !JSValueIsUndefined(ctx, ret));
        });
        handler_func("deleteProperty", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<v8::Boolean,InterceptorOther>
            (PASS, [](NAMED_PARAMS(v8::Boolean)) { config.deleter(property, info); },
             [](INDEXED_PARAMS(v8::Boolean)) { config.deleter(index, info); });
            if (!*exception && ret == NULL) {
                assert(argumentCount>1);
                return V82JSC::exec(ctx, "return delete _1[_2]", 2, arguments, exception);
            }
            return ret;
        });
        handler_func("ownKeys", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<v8::Array,InterceptorOther>
            (PASS, [](NAMED_PARAMS(v8::Array)) { config.enumerator(info); },
             [](INDEXED_PARAMS(v8::Array)) { config.enumerator(info); });
            if (!*exception && ret == NULL) {
                IsolateImpl *iso = IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)];
                assert(argumentCount>0);
                JSValueRef args[] = {
                    arguments[0],
                    iso->m_private_symbol
                };
                return V82JSC::exec(ctx,
                                    "return Array.from(new Set("
                                    "  Object.getOwnPropertyNames(_1)"
                                    "    .concat(Object.getOwnPropertySymbols(_1))"
                                    ")).filter(p => p!==_2)",
                                    2, args, exception);
            }
            return ret;
        });
        handler_func("defineProperty", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>2);
            JSValueRef ret = PropertyHandler<Value,InterceptorOther>
            (PASS, [](NAMED_PARAMS(Value)) { config.definer(property, value, info); },
             [](INDEXED_PARAMS(Value)) { config.definer(index, value, info); });
            if (!*exception && ret == NULL) {
                return V82JSC::exec(ctx, "return Object.defineProperty(_1, _2, _3)", 3, arguments, exception);
            }
            return ret;
        });
        handler_func("getPrototypeOf", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>0);
            TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
            Isolate *isolate = V82JSC::ToIsolate(IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)]);
            HandleScope scope(isolate);
            Local<Context> context = LocalContextImpl::New(isolate, ctx);
            ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl>(wrap->m_object_template.Get(isolate));
            /*
            if (templ->m_access_check && !templ->m_access_check(context,
                                                                ValueImpl::New(V82JSC::ToContextImpl(context), arguments[0]).As<Object>(),
                                                                ValueImpl::New(V82JSC::ToContextImpl(context), templ->m_access_check_data)))
            */
            if (templ->m_access_check)
            {
                return JSValueMakeNull(ctx);
            }
            Local<Value> proto = ValueImpl::New(V82JSC::ToContextImpl(context),
                                                arguments[0]).As<Object>()->GetPrototype();
            return V82JSC::ToJSValueRef(proto, context);
        });
        handler_func("setPrototypeOf", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>1);
            TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
            Isolate *isolate = V82JSC::ToIsolate(IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)]);
            HandleScope scope(isolate);
            Local<Context> context = LocalContextImpl::New(isolate, ctx);
            ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl>(wrap->m_object_template.Get(isolate));
            if (templ->m_access_check && !templ->m_access_check(context,
                                                                ValueImpl::New(V82JSC::ToContextImpl(context), arguments[0]).As<Object>(),
                                                                ValueImpl::New(V82JSC::ToContextImpl(context), templ->m_access_check_data)))
            {
                isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "access denied",
                                                                                 NewStringType::kNormal).ToLocalChecked()));
                
            }
            Maybe<bool> r = ValueImpl::New(V82JSC::ToContextImpl(context),arguments[0])
                .As<Object>()->SetPrototype(context, ValueImpl::New(V82JSC::ToContextImpl(context), arguments[1]));
            return JSValueMakeBoolean(ctx, r.FromJust());
        });
        handler_func("getOwnPropertyDescriptor", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>1);
            // First, try a descriptor interceptor
            JSValueRef descriptor = PropertyHandler<Value,InterceptorOther>
            (PASS, [](NAMED_PARAMS(Value)) { config.descriptor(property, info); },
             [](INDEXED_PARAMS(Value)) { config.descriptor(index, info); });
            if (descriptor) return descriptor;
            if (exception && *exception) return NULL;
            
            // Second, see if we have a real property descriptor
            descriptor = V82JSC::exec(ctx, "return Object.getOwnPropertyDescriptor(_1, _2)", 2, arguments, exception);
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
            JSValueRef value = PropertyHandler<Value,InterceptorOther>
            (
             PASS, [](NAMED_PARAMS(Value)) { config.getter(property, info); },
             [](INDEXED_PARAMS(Value)) { config.getter(index, info); }
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
                    return V82JSC::exec(ctx, "return { writable: _1, enumerable: _2, configurable: true, value: _3 }", 3, args);
                } else {
                    return V82JSC::exec(ctx, "return { writable: _1, enumerable: _2, configurable: true }", 2, args);
                }
            }

            // No property
            return JSValueMakeUndefined(ctx);
        });
    }

    MaybeLocal<Object> instance;
    if (!m_constructor_template.IsEmpty()) {
        instance = reinterpret_cast<TemplateImpl*>(this)->
        InitInstance(context, root, exception, m_constructor_template.Get(isolate));
    } else {
        instance = reinterpret_cast<TemplateImpl*>(this)->InitInstance(context, root, exception);
    }
    if (instance.IsEmpty()) {
        return instance;
    }

    if (m_need_proxy) {
        JSValueRef args[] = {root, handler};
        JSValueRef proxy_object = V82JSC::exec(ctx->m_ctxRef, "return new Proxy(_1, _2)", 2, args);
        // Important!  Set the security proxy before calling ValueImpl::New().  We don't want the proxy object
        // to have its own wrap
        wrap->m_proxy_security = proxy_object;
        Local<Object> proxy = ValueImpl::New(ctx, proxy_object).As<Object>();
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
            TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
            assert(wrap && wrap->m_hidden_proxy_security);
            Isolate *isolate = V82JSC::ToIsolate(IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)]);
            HandleScope scope(isolate);
            Local<Context> context = LocalContextImpl::New(isolate, ctx);
            Local<Name> property = ValueImpl::New(V82JSC::ToContextImpl(context), arguments[1]).As<Name>();
            if (JSValueIsStrictEqual(ctx, arguments[2], JSValueMakeUndefined(ctx)) ||
                JSValueIsStrictEqual(ctx, arguments[2], wrap->m_hidden_proxy_security)) {
                
                V82JSC::ToImpl<HiddenObjectImpl>(ValueImpl::New(V82JSC::ToContextImpl(context), arguments[0]))
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
            V82JSC::ToJSValueRef(instance.ToLocalChecked(), context),
            propagate_set,
            propagate_delete
        };
        JSValueRef hidden_proxy_object = V82JSC::exec(ctx->m_ctxRef, proxy_code, 3, args);
        // Same here.  Set the hidden proxy reference before calling ValueImpl::New()
        wrap->m_hidden_proxy_security = hidden_proxy_object;
        Local<Object> hidden_proxy = ValueImpl::New(ctx, hidden_proxy_object).As<Object>();
        instance = hidden_proxy;
    }
    
    return instance;
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
void ObjectTemplate::SetAccessor(
                 Local<String> name, AccessorGetterCallback getter,
                 AccessorSetterCallback setter, Local<Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    SetAccessor(name.As<Name>(),
                reinterpret_cast<AccessorNameGetterCallback>(getter),
                reinterpret_cast<AccessorNameSetterCallback>(setter),
                data, settings, attribute, signature);
}
void ObjectTemplate::SetAccessor(
                 Local<Name> name, AccessorNameGetterCallback getter,
                 AccessorNameSetterCallback setter, Local<Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    ObjectTemplateImpl *this_ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(this_));
    
    H::ObjAccessor *accessor = static_cast<H::ObjAccessor *>
    (H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(this_), V82JSC::ToIsolateImpl(this_)->m_object_accessor_map));
    
    accessor->name.Reset(isolate, name);
    accessor->getter = getter;
    accessor->setter = setter;
    accessor->data.Reset(isolate, data);
    accessor->settings = settings;
    accessor->attribute = attribute;
    
    // For now, Signature and AccessorSignature are the same
    Local<Signature> sig = * reinterpret_cast<Local<Signature>*>(&signature);
    
    accessor->signature.Reset(isolate, sig);
    
    Local<v8::ObjAccessor> local = V82JSC::CreateLocal<v8::ObjAccessor>(isolate, accessor);
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
void ObjectTemplate::SetNamedPropertyHandler(NamedPropertyGetterCallback getter,
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
void ObjectTemplate::SetHandler(const NamedPropertyHandlerConfiguration& configuration)
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
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
    templ->m_named_data = V82JSC::ToJSValueRef(configuration.data, Isolate::GetCurrent());
    JSValueProtect(V82JSC::ToContextRef(Isolate::GetCurrent()), templ->m_named_data);
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
void ObjectTemplate::SetHandler(const IndexedPropertyHandlerConfiguration& configuration)
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
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
    templ->m_indexed_data = V82JSC::ToJSValueRef(configuration.data, Isolate::GetCurrent());
    JSValueProtect(V82JSC::ToContextRef(Isolate::GetCurrent()), templ->m_indexed_data);
    templ->m_need_proxy = true;
}

/**
 * Sets the callback to be used when calling instances created from
 * this template as a function.  If no callback is set, instances
 * behave like normal JavaScript objects that cannot be called as a
 * function.
 */
void ObjectTemplate::SetCallAsFunctionHandler(FunctionCallback callback,
                              Local<Value> data)
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> context = V82JSC::ToCurrentContext(this);
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    templ->m_callback = callback;
    if (data.IsEmpty()) {
        data = Undefined(isolate);
    }
    templ->m_data = V82JSC::ToJSValueRef<Value>(data, isolate);
    JSValueProtect(V82JSC::ToContextRef(context), templ->m_data);
}

/**
 * Mark object instances of the template as undetectable.
 *
 * In many ways, undetectable objects behave as though they are not
 * there.  They behave like 'undefined' in conditionals and when
 * printed.  However, properties can be accessed and called as on
 * normal objects.
 */
void ObjectTemplate::MarkAsUndetectable()
{
    assert(0);
}

/**
 * Sets access check callback on the object template and enables access
 * checks.
 *
 * When accessing properties on instances of this object template,
 * the access check callback will be called to determine whether or
 * not to allow cross-context access to the properties.
 */
void ObjectTemplate::SetAccessCheckCallback(AccessCheckCallback callback,
                            Local<Value> data)
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    Isolate *isolate = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(templ));
    HandleScope scope(isolate);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    templ->m_access_check = callback;
    if (data.IsEmpty()) {
        data = Undefined(isolate);
    }
    templ->m_access_check_data = V82JSC::ToJSValueRef(data, context);
    JSValueProtect(ctx, templ->m_access_check_data);
    templ->m_need_proxy = true;
}

/**
 * Like SetAccessCheckCallback but invokes an interceptor on failed access
 * checks instead of looking up all-can-read properties. You can only use
 * either this method or SetAccessCheckCallback, but not both at the same
 * time.
 */
void ObjectTemplate::SetAccessCheckCallbackAndHandler(
                                      AccessCheckCallback callback,
                                      const NamedPropertyHandlerConfiguration& named_handler,
                                      const IndexedPropertyHandlerConfiguration& indexed_handler,
                                      Local<Value> data)
{
    SetAccessCheckCallback(callback, data);
    
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
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
    templ->m_failed_named_data = V82JSC::ToJSValueRef(named_data, Isolate::GetCurrent());
    JSValueProtect(V82JSC::ToContextRef(Isolate::GetCurrent()), templ->m_failed_named_data);

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
    templ->m_failed_indexed_data = V82JSC::ToJSValueRef(named_data, Isolate::GetCurrent());
    JSValueProtect(V82JSC::ToContextRef(Isolate::GetCurrent()), templ->m_failed_indexed_data);
}

/**
 * Gets the number of internal fields for objects generated from
 * this template.
 */
int ObjectTemplate::InternalFieldCount()
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    return templ->m_internal_fields;
}

/**
 * Sets the number of internal fields for objects generated from
 * this template.
 */
void ObjectTemplate::SetInternalFieldCount(int value)
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    templ->m_internal_fields = value > 0 ? value : 0;
}

/**
 * Returns true if the object will be an immutable prototype exotic object.
 */
bool ObjectTemplate::IsImmutableProto()
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    return templ->m_is_immutable_proto;
}

/**
 * Makes the ObjectTempate for an immutable prototype exotic object, with an
 * immutable __proto__.
 */
void ObjectTemplate::SetImmutableProto()
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    templ->m_is_immutable_proto = true;
}

