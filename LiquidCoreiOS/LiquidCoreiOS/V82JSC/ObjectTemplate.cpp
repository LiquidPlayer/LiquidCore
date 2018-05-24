//
//  ObjectTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include <string.h>

using namespace v8;

/** Creates an ObjectTemplate. */
Local<ObjectTemplate> ObjectTemplate::New(
                                 Isolate* isolate,
                                 Local<FunctionTemplate> constructor)
{
    auto destructor = [](InternalObjectImpl *o)
    {
        ObjectTemplateImpl *otempl = static_cast<ObjectTemplateImpl*>(o);
        v8::Isolate* i = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(o));
        JSContextRef ctx = V82JSC::ToContextRef(i);
        if (otempl->m_indexed_data) JSValueUnprotect(ctx, otempl->m_indexed_data);
        if (otempl->m_named_data) JSValueUnprotect(ctx, otempl->m_named_data);
        otempl->m_constructor_template.Reset();
        templateDestructor(o);
    };
    
    if (!constructor.IsEmpty()) {
        return constructor->InstanceTemplate();
    } else {
        ObjectTemplateImpl *otempl = (ObjectTemplateImpl*) TemplateImpl::New(isolate, sizeof(ObjectTemplateImpl), destructor);
        V82JSC::Map(otempl)->set_instance_type(v8::internal::OBJECT_TEMPLATE_INFO_TYPE);

        otempl->m_constructor_template = Copyable(v8::FunctionTemplate)();
        otempl->m_named_data = 0;
        otempl->m_indexed_data = 0;

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
        TemplateWrap *wrap = new TemplateWrap();
        wrap->m_template.Reset(isolate, thiz);
        wrap->m_isolate = iso;
        def.finalize = [](JSObjectRef obj) {
            TemplateWrap* wrap = (TemplateWrap*) JSObjectGetPrivate(obj);
            wrap->m_template.Reset();
            delete wrap;
        };

        instance = JSObjectMake(ctx->m_ctxRef, claz, wrap);
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
                           void (*named_handler)(const ObjectTemplateImpl*, Local<Name>, Local<Value>, PropertyCallbackInfo<V>&),
                           void (*indexed_handler)(const ObjectTemplateImpl*, uint32_t, Local<Value>, PropertyCallbackInfo<V>&))
{
    // Arguments:
    //  get            - target, property, receiver        -> Value
    //  set            - target, property, value, receiver -> True (assigned), False (not assigned)
    //  deleteProperty - target, property                  -> True (deleted), False (not deleted)
    //  has            - target, property                  -> True (has), False (not has)
    //  ownKeys        - target                            -> Array of keys
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
            if (JSValueToBoolean(ctx, V82JSC::exec(ctx, "return _1 === Symbol.for('" GLOBAL_PRIVATE_SYMBOL "')", 1, &arguments[1]))) {
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
    InstanceWrap* wrap = V82JSC::getPrivateInstance(ctx, target);
    Isolate *isolate = V82JSC::ToIsolate(wrap->m_isolate);
    HandleScope scope(isolate);
    
    const ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl>(wrap->m_object_template.Get(isolate));
    IsolateImpl *isolateimpl = wrap->m_isolate;
    Local<Context> context = ContextImpl::New(V82JSC::ToIsolate(isolateimpl), ctx);
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    Local<Value> holder = ValueImpl::New(ctximpl, wrap->m_proxy_security);

    Local<Value> data;
    if (templ) {
        if (isSymbol || !isIndex) { /* Is named */
            data = ValueImpl::New(ctximpl, templ->m_named_data);
        } else { /* Is Indexed */
            data = ValueImpl::New(ctximpl, templ->m_indexed_data);
        }
    } else {
        data = Undefined(isolate);
    }

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
        named_handler(templ,
                      prop,
                      set, info);
    } else {
        indexed_handler(templ, index, set, info);
    }
    
    if (try_catch.HasCaught()) {
        *exception = V82JSC::ToJSValueRef(try_catch.Exception(), context);
    } else if (isolateimpl->ii.thread_local_top()->scheduled_exception_ != the_hole) {
        internal::Object * excep = isolateimpl->ii.thread_local_top()->scheduled_exception_;
        if (excep->IsHeapObject()) {
            ValueImpl* i = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(excep) - internal::kHeapObjectTag);
            *exception = i->m_value;
        } else {
            *exception = JSValueMakeNumber(ctx, internal::Smi::ToInt(excep));
        }
        isolateimpl->ii.thread_local_top()->scheduled_exception_ = the_hole;
    }

    if (implicit[4] == the_hole) {
        return NULL;
    }
    
    Local<Value> retVal = info.GetReturnValue().Get();
    return V82JSC::ToJSValueRef<Value>(retVal, context);
}

InstanceWrap::~InstanceWrap()
{
    Isolate* isolate = reinterpret_cast<Isolate*>(m_isolate);
    HandleScope scope(isolate);
    
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    if (m_num_internal_fields && m_internal_fields) {
        for (int i=0; i<m_num_internal_fields; i++) {
            if (m_internal_fields[i]) JSValueUnprotect(ctx, m_internal_fields[i]);
        }
        delete m_internal_fields;
    }
    if (m_private_properties) {
        JSValueUnprotect(ctx, m_private_properties);
    }
    m_object_template.Reset();
}

v8::MaybeLocal<v8::Object> ObjectTemplateImpl::NewInstance(v8::Local<v8::Context> context, JSObjectRef root, bool isHiddenPrototype)
{
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctx);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    
    LocalException exception(iso);
    Local<ObjectTemplate> thiz = V82JSC::CreateLocal<ObjectTemplate>(isolate, this);
    
    // Structure:
    //
    // proxy -----> root . Symbol.for('org.liquidplayer.javascript.__v82jsc_private__') -->  lifecycle_object(wrap) --> InstanceWrap*
    
    // Create lifecycle object
    InstanceWrap *wrap = V82JSC::makePrivateInstance(iso, ctx->m_ctxRef, root);
    wrap->m_object_template.Reset(isolate, thiz);
    wrap->m_num_internal_fields = m_internal_fields;
    wrap->m_internal_fields = new JSValueRef[m_internal_fields]();
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
        
        handler_func("apply", [](CALLBACK_PARAMS) -> JSValueRef
        {
            // FIXME!
            assert(0);
            return NULL;
        });
        handler_func("get", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Value,InterceptorGetter>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_named_handler.getter)
                    impl->m_named_handler.getter(property, info);
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_indexed_handler.getter)
                    impl->m_indexed_handler.getter(index, info);
            });
            if (ret == NULL && !*exception) {
                // Not handled.  Pass thru.
                assert(argumentCount>1);
                return V82JSC::exec(ctx, "return _1[_2]", 2, arguments, exception);
            }
            return ret;
        });
        handler_func("set", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Value,InterceptorSetter>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_named_handler.setter) {
                    impl->m_named_handler.setter(property, value, info);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_indexed_handler.setter) {
                    impl->m_indexed_handler.setter(index, value, info);
                }
            });
            if (*exception) {
                return JSValueMakeBoolean(ctx, false);
            }
            if (ret == NULL) {
                assert(argumentCount>2);
                InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
                assert(wrap);
                return V82JSC::exec(ctx, "return _1[_2] = _3", 3, arguments, exception);
            }
            return JSValueMakeBoolean(ctx, true);
        });
        handler_func("has", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Integer,InterceptorOther>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Integer>& info)
            {
                if (impl->m_named_handler.query) {
                    impl->m_named_handler.query(property, info);
                } else if (impl->m_named_handler.getter) {
                    info.GetReturnValue().Set(v8::PropertyAttribute::None);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Integer>& info)
            {
                if (impl->m_indexed_handler.query) {
                    impl->m_indexed_handler.query(index, info);
                } else if (impl->m_indexed_handler.getter) {
                    info.GetReturnValue().Set(v8::PropertyAttribute::None);
                }
            });
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
            JSValueRef ret = PropertyHandler<v8::Boolean,InterceptorOther>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<v8::Boolean>& info)
            {
                if (impl->m_named_handler.deleter) {
                    impl->m_named_handler.deleter(property, info);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<v8::Boolean>& info)
            {
                if (impl->m_indexed_handler.deleter) {
                    impl->m_indexed_handler.deleter(index, info);
                }
            });
            if (!*exception && ret == NULL) {
                assert(argumentCount>1);
                return V82JSC::exec(ctx, "delete _1[_2]", 2, arguments, exception);
            }
            return ret;
        });
        handler_func("ownKeys", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<v8::Array,InterceptorOther>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<v8::Array>& info)
            {
                if (impl->m_named_handler.enumerator) {
                    impl->m_named_handler.enumerator(info);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<v8::Array>& info)
            {
                if (impl->m_indexed_handler.enumerator) {
                    impl->m_indexed_handler.enumerator(info);
                }
            });
            if (!*exception && ret == NULL) {
                assert(argumentCount>0);
                return V82JSC::exec(ctx,
                                    "return Array.from(new Set("
                                    "  Object.getOwnPropertyNames(_1)"
                                    "    .concat(Object.getOwnPropertySymbols(_1))"
                                    ")).filter(p => p!==Symbol.for('" GLOBAL_PRIVATE_SYMBOL "'))",
                                    1, arguments, exception);
            }
            return ret;
        });
        handler_func("defineProperty", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>2);
            return V82JSC::exec(ctx, "return Object.defineProperty(_1, _2, _3)", 3, arguments, exception);
        });
        handler_func("getPrototypeOf", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>0);
            InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
            Isolate *isolate = V82JSC::ToIsolate(wrap->m_isolate);
            HandleScope scope(isolate);
            Local<Context> context = ContextImpl::New(isolate, ctx);
            Local<Value> proto = ValueImpl::New(V82JSC::ToContextImpl(context),
                                                arguments[0]).As<Object>()->GetPrototype();
            return V82JSC::ToJSValueRef(proto, context);
        });
        handler_func("setPrototypeOf", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>1);
            InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
            Isolate *isolate = V82JSC::ToIsolate(wrap->m_isolate);
            HandleScope scope(isolate);
            Local<Context> context = ContextImpl::New(isolate, ctx);
            Maybe<bool> r = ValueImpl::New(V82JSC::ToContextImpl(context),arguments[0])
                .As<Object>()->SetPrototype(context, ValueImpl::New(V82JSC::ToContextImpl(context), arguments[1]));
            return JSValueMakeBoolean(ctx, r.FromJust());
        });
        handler_func("getOwnPropertyDescriptor", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>1);
            
            // First, try a descriptor interceptor
            JSValueRef descriptor = PropertyHandler<Value,InterceptorOther>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_named_handler.descriptor) {
                    impl->m_named_handler.descriptor(property, info);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_indexed_handler.descriptor) {
                    impl->m_indexed_handler.descriptor(index, info);
                }
            });
            if (descriptor) return descriptor;
            if (exception && *exception) return NULL;
            
            // Second, see if we have a real property descriptor
            descriptor = V82JSC::exec(ctx, "return Object.getOwnPropertyDescriptor(_1, _2)", 2, arguments, exception);
            if (descriptor && !JSValueIsStrictEqual(ctx, descriptor, JSValueMakeUndefined(ctx))) return descriptor;
            if (exception && *exception) return NULL;

            // Third, try calling the querier to see if the property exists
            JSValueRef attributes = PropertyHandler<Integer,InterceptorOther>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Integer>& info)
            {
                if (impl->m_named_handler.query) {
                    impl->m_named_handler.query(property, info);
                } else {
                    info.GetReturnValue().Set(-1);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Integer>& info)
            {
                if (impl->m_indexed_handler.query) {
                    impl->m_indexed_handler.query(index, info);
                } else {
                    info.GetReturnValue().Set(-1);
                }
            });
            if (exception && *exception) return NULL;

            // attributes can be NULL (has querier, property does not exist), -1 (no querier, defer to value), PropertyAttribute (has property)
            if (attributes == NULL) {
                return JSValueMakeUndefined(ctx);
            }
            int pattr = static_cast<int>(JSValueToNumber(ctx, attributes, 0));

            JSValueRef value = PropertyHandler<Value,InterceptorOther>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_named_handler.getter)
                    impl->m_named_handler.getter(property, info);
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_indexed_handler.getter)
                    impl->m_indexed_handler.getter(index, info);
            });
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
        instance = InitInstance(context, root, exception, m_constructor_template.Get(isolate));
    } else {
        instance = InitInstance(context, root, exception);
    }
    if (instance.IsEmpty()) {
        return instance;
    }

    if (m_need_proxy) {
        JSValueRef args[] = {root, handler};
        JSValueRef proxy_object = V82JSC::exec(ctx->m_ctxRef, "return new Proxy(_1, _2)", 2, args);
        Local<Object> proxy = ValueImpl::New(ctx, proxy_object).As<Object>();
        wrap->m_proxy_security = proxy_object;
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
            InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)arguments[0]);
            assert(wrap && wrap->m_hidden_proxy_security);
            HandleScope scope(V82JSC::ToIsolate(wrap->m_isolate));
            Local<Context> context = ContextImpl::New(V82JSC::ToIsolate(wrap->m_isolate), ctx);
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
        Local<Object> hidden_proxy = ValueImpl::New(ctx, hidden_proxy_object).As<Object>();
        wrap->m_hidden_proxy_security = hidden_proxy_object;
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
    
    ObjAccessor accessor;
    accessor.name = Copyable(Name)(isolate, name);
    accessor.getter = getter;
    accessor.setter = setter;
    accessor.data = Copyable(Value)(isolate, data);
    accessor.settings = settings;
    accessor.attribute = attribute;
    
    // For now, Signature and AccessorSignature are the same
    Local<Signature> sig = * reinterpret_cast<Local<Signature>*>(&signature);
    
    accessor.signature = Copyable(Signature)(isolate, sig);
    
    this_->m_accessors.push_back(accessor);
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
    assert(0);
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
    templ->m_named_handler = configuration;
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
    templ->m_indexed_handler = configuration;
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
    assert(0);
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
    assert(0);
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
    assert(0);
    return false;
}

/**
 * Makes the ObjectTempate for an immutable prototype exotic object, with an
 * immutable __proto__.
 */
void ObjectTemplate::SetImmutableProto()
{
    assert(0);
}

