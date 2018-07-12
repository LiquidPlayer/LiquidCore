//
//  TrackedObject.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 5/29/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include "JSObjectRefPrivate.h"

using namespace v8;
#define H V82JSC_HeapObject

TrackedObjectImpl* makePrivateInstance(IsolateImpl* iso, JSContextRef ctx)
{
    TrackedObjectImpl* impl = static_cast<TrackedObjectImpl*>(H::HeapAllocator::Alloc(iso, iso->m_tracked_object_map));
    impl->m_hash = 1 + rand();
    
    return impl;
}

void setPrivateInstance(IsolateImpl* iso, JSContextRef ctx, TrackedObjectImpl* impl, JSObjectRef object)
{
    HandleScope scope(V82JSC::ToIsolate(iso));
    // Keep only a weak reference to m_security to avoid cyclical references
    impl->m_security = object;
    
    Local<V82JSC_HeapObject::TrackedObject> to = V82JSC::CreateLocal<V82JSC_HeapObject::TrackedObject>(&iso->ii, impl);
    void * data = V82JSC::PersistentData<V82JSC_HeapObject::TrackedObject>(V82JSC::ToIsolate(iso), to);
    
    JSClassDefinition def = kJSClassDefinitionEmpty;
    def.attributes = kJSClassAttributeNoAutomaticPrototype;
    def.finalize = [](JSObjectRef object) {
        void * persistent = JSObjectGetPrivate(object);
        assert(persistent);
        JSGlobalContextRef ctx = JSObjectGetGlobalContext(object);
        assert(ctx);
        IsolateImpl *iso;
        {
            std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
            iso = IsolateImpl::s_context_to_isolate_map[ctx];
        }
        if (!iso) return;

        HandleScope scope(V82JSC::ToIsolate(iso));
        Local<TrackedObject> local = V82JSC::FromPersistentData<TrackedObject>(V82JSC::ToIsolate(iso), persistent);
        assert(!local.IsEmpty());
        TrackedObjectImpl *impl = V82JSC::ToImpl<TrackedObjectImpl>(local);
        iso->weakJSObjectFinalized(ctx, (JSObjectRef) impl->m_security);
        
        V82JSC::ReleasePersistentData<V82JSC_HeapObject::TrackedObject>(persistent);
    };
    JSClassRef klass = JSClassCreate(&def);
    JSObjectRef private_object = JSObjectMake(ctx, klass, data);
    JSClassRelease(klass);

    JSValueRef args[] = {
        object, private_object, iso->m_private_symbol
    };
    
    V82JSC::exec(JSContextGetGlobalContext(ctx),
                 "Object.defineProperty(_1, _3, {value: _2, enumerable: false, configurable: true, writable: true})",
                 3, args);
}


TrackedObjectImpl* makePrivateInstance(IsolateImpl* iso, JSContextRef ctx, JSObjectRef object)
{
    TrackedObjectImpl *impl = getPrivateInstance(ctx, object);
    if (impl) return impl;
    
    impl = makePrivateInstance(iso, ctx);
    setPrivateInstance(iso, ctx, impl, object);
    return impl;
}

TrackedObjectImpl* getPrivateInstance(JSContextRef ctx, JSObjectRef object)
{
    IsolateImpl *iso;
    {
        std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
        iso = IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)];
    }
    HandleScope scope(V82JSC::ToIsolate(iso));
    //JSObjectRef maybe_proxy_object = JSObjectGetProxyTarget(object);
    JSValueRef args[] = {
        /*maybe_proxy_object ? maybe_proxy_object :*/ object,
        iso->m_private_symbol
    };
    JSObjectRef private_object = (JSObjectRef) V82JSC::exec(ctx, "return _1[_2]", 2, args);
    if (JSValueIsObject(ctx, private_object)) {
        void * persistent = JSObjectGetPrivate(private_object);
        if (!persistent) return nullptr;

        Local<TrackedObject> local = V82JSC::FromPersistentData<TrackedObject>(V82JSC::ToIsolate(iso), persistent);
        assert(!local.IsEmpty());
        TrackedObjectImpl *impl = V82JSC::ToImpl<TrackedObjectImpl>(local);

        JSValueRef hasargs[] = {
            impl->m_access_proxies,
            impl->m_global_object_access_proxies,
            object
        };
        
        if ((JSValueIsStrictEqual(ctx, object, impl->m_security) ||
             (impl->m_proxy_security && JSValueIsStrictEqual(ctx, object, impl->m_proxy_security)) ||
             (impl->m_hidden_proxy_security && JSValueIsStrictEqual(ctx, object, impl->m_hidden_proxy_security)) ||
             (impl->m_access_proxies && V82JSC::exec(ctx, "return _1.includes(_3)", 3, hasargs)) ||
             (impl->m_global_object_access_proxies && V82JSC::exec(ctx, "return _2.includes(_3)", 3, hasargs)) )) {
        
            return impl;
        } else if (impl->m_isGlobalObject) {
            JSObjectRef proto = object;
            while (JSValueIsObject(ctx, proto)) {
                if (JSValueIsStrictEqual(ctx, proto, impl->m_security) || JSValueIsStrictEqual(ctx, proto, impl->m_proxy_security)) {
                    return impl;
                }
                proto = (JSObjectRef) JSObjectGetPrototype(ctx, proto);
            }
        }
    }
    return nullptr;
}

Local<Value> TrackedObjectImpl::SecureValue(Local<Value> in, Local<Context> toContext)
{
    /*
     V8::AccessControl::ALL_CAN_READ = 1;       (1)
     V8::AccessControl::ALL_CAN_WRITE = 1 << 1; (2)
    */
    
    static const char *security_proxy =
    "var handler = {"
    "    get : (t, p, r) => { "
    "        try { return Reflect.get(_2('get',_1,1,p), p); }"
    "        catch(e) {"
    "            if (p===Symbol.isConcatSpreadable || p===Symbol.toStringTag) "
    "                return undefined;"
    "            else throw e; } },"
    "    set : (t, p, v, r) => (_2('set',_1,2,p,r)[p] = v) == v,"
    "    setPrototypeOf : (t, p) => {_2('setPrototypeOf',_1); return Object.setPrototypeOf(_1,p); },"
    "    getPrototypeOf : (t) => { try{_2('getPrototypeOf',_1); return Object.getPrototypeOf(_1);} catch(e) {return null;}},"
    "    getOwnPropertyDescriptor : (t, p) => _2('getOwnPropertyDescriptor',_1,1,p,null,true),"
    "    defineProperty : (t,p,d) => Object.defineProperty(_2('defineProperty',_1),p,d),"
    "    has : (t, p) => p in _2('has',_1,1,p),"
    "    deleteProperty : (t, p) => Reflect.deleteProperty(_2('deleteProperty',_1),p),"
    "    ownKeys : (t) => _2('ownKeys',_1),"
    "    apply : (t, z, a) => Reflect.apply(_2('apply',_1),z,a),"
    "    construct : (t, a, n) => Reflect.construct(_2('construct',_1),a,n),"
    "    preventExtensions : (t) => { Object.preventExtensions(_2('preventExtensions',_1)); return Object.preventExtensions(t); },"
    "    isExtensible : (t) => { var is = Object.isExtensible(/*_2('isExtensible',_1)*/_1); return is; }"
    "};"
    "var o = (typeof _1 === 'function') ? function(){} : {};"
    "return new Proxy(o, handler);";
    
    Isolate* isolate = Isolate::GetCurrent();
    EscapableHandleScope scope(isolate);
    
    Local<Context> context = isolate->GetCurrentContext();
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSGlobalContextRef gctx = JSContextGetGlobalContext(ctx);
    JSValueRef in_value = V82JSC::ToJSValueRef(in, context);
    
    if (!JSValueIsObject(ctx, in_value)) return in;
    
    JSGlobalContextRef orig_context = JSObjectGetGlobalContext((JSObjectRef)in_value);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    if (orig_context == V82JSC::ToImpl<ContextImpl>(iso->m_nullContext.Get(isolate))->m_ctxRef) {
        return in;
    }
    JSGlobalContextRef toGlobalContext = 0;
    if (!toContext.IsEmpty()) {
        toGlobalContext = JSContextGetGlobalContext(V82JSC::ToContextImpl(toContext)->m_ctxRef);
    }

    TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef)in_value);
    JSObjectRef check_proxies = 0;
    JSObjectRef global_object = JSContextGetGlobalObject(orig_context);
    bool isActualGlobalObject = JSValueIsStrictEqual(ctx, global_object, in_value);
    if (isActualGlobalObject && !wrap) {
        wrap = makePrivateInstance(iso, ctx, (JSObjectRef)in_value);
        wrap->m_isGlobalObject = true;
    }
    
    if (!isActualGlobalObject && wrap && wrap->m_global_object_access_proxies) {
        JSValueRef args[] = {
            wrap->m_global_object_access_proxies,
            in_value
        };
        isActualGlobalObject = JSValueToBoolean(ctx, V82JSC::exec(ctx, "return _1.includes(_2)", 2, args));
    }

    // If we are putting the global object back into its own context, remove the proxy
    if (isActualGlobalObject && orig_context == toGlobalContext) {
        return ValueImpl::New(V82JSC::ToContextImpl(toContext), JSContextGetGlobalObject(orig_context));
    }

    if (wrap && wrap->m_isGlobalObject && wrap->m_global_object_access_proxies && isActualGlobalObject) {
        check_proxies = wrap->m_global_object_access_proxies;
    } else if (wrap && wrap->m_access_proxies && !isActualGlobalObject) {
        check_proxies = wrap->m_access_proxies;
    }
    if (check_proxies) {
        int length = static_cast<int>(JSValueToNumber(ctx, V82JSC::exec(ctx, "return _1.length", 1, &check_proxies), 0));
        for (int i=0; i<length; i++) {
            JSObjectRef maybe_proxy = (JSObjectRef) JSObjectGetPropertyAtIndex(ctx, check_proxies, i, 0);
            if (JSObjectGetGlobalContext(maybe_proxy) == gctx) {
                return scope.Escape(ValueImpl::New(V82JSC::ToContextImpl(context), maybe_proxy));
            }
        }
    }
    
    bool install_proxy = JSObjectGetGlobalContext((JSObjectRef)in_value) != gctx;
    if (!install_proxy) {
        if (wrap && !wrap->m_object_template.IsEmpty()) {
            Local<ObjectTemplate> ot = wrap->m_object_template.Get(isolate);
            if (!ot.IsEmpty()) {
                ObjectTemplateImpl *oi = V82JSC::ToImpl<ObjectTemplateImpl>(ot);
                install_proxy = oi->m_access_check != nullptr;
            }
        }
    }
    
    if (!install_proxy) {
        if (!wrap || !wrap->m_isGlobalObject) {
            TrackedObjectImpl *global_wrap = getPrivateInstance(ctx, JSContextGetGlobalObject(ctx));
            install_proxy = global_wrap && (global_wrap->m_isDetached || global_wrap->m_reattached_global);
        }
    }

    if (install_proxy) {
        JSClassDefinition def = kJSClassDefinitionEmpty;
        def.attributes = kJSClassAttributeNoAutomaticPrototype;
        def.callAsFunction = [](JSContextRef ctx, JSObjectRef proxy_function, JSObjectRef thisObject,
                                size_t argumentCount, const JSValueRef *arguments, JSValueRef *exception) ->JSValueRef
        {
            IsolateImpl* iso;
            {
                std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
                iso = IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)];
            }
            Isolate* isolate = V82JSC::ToIsolate(iso);
            HandleScope scope(isolate);
            
            JSStringRef method = JSValueToStringCopy(ctx, arguments[0], 0);
            char buffer[JSStringGetMaximumUTF8CStringSize(method)];
            JSStringGetUTF8CString(method, buffer, JSStringGetMaximumUTF8CStringSize(method));
            JSStringRelease(method);
            /* DEBUG
            if (argumentCount > 3) {
                JSStringRef prop = JSValueToStringCopy(ctx, arguments[3], 0);
                if (!prop) { prop = JSStringCreateWithUTF8CString("[symbol]"); }
                char prop_buf[JSStringGetMaximumUTF8CStringSize(prop)];
                JSStringGetUTF8CString(prop, prop_buf, JSStringGetMaximumUTF8CStringSize(prop));
                JSStringRelease(prop);
                printf ("method: %s, property: %s\n", buffer, prop_buf);
            } else {
                printf ("method: %s\n", buffer);
            }
            */
            
            // Always let the private symbol pass through without restriction
            if (!strcmp(buffer,"get") && JSValueIsStrictEqual(ctx, arguments[3], iso->m_private_symbol)) {
                return arguments[1];
            }
            if (JSContextGetGlobalContext(ctx) == V82JSC::ToContextRef(iso->m_nullContext.Get(isolate))) {
                return arguments[1];
            }

            JSObjectRef in_value = (JSObjectRef)arguments[1];
            JSGlobalContextRef orig_context = JSObjectGetGlobalContext(in_value);
            Local<Context> accessing_context = LocalContextImpl::New(isolate, ctx);
            Local<Object> accessing_object = ValueImpl::New(V82JSC::ToContextImpl(accessing_context), in_value).As<Object>();
            bool allow = false;
            bool detached_behavior = false;

            if (argumentCount>4 && !JSValueIsNull(ctx,arguments[4])) {
                JSObjectRef target = JSObjectGetProxyTarget((JSObjectRef)arguments[4]);
                /* FIXME! Not sure this will always work correctly
                if (!JSValueIsStrictEqual(ctx, target, in_value)) {
                    return in_value;
                }
                */
                if (target == 0) {
                    return in_value;
                }
            }
            
            ObjectTemplateImpl* oi = nullptr;
            TrackedObjectImpl *wrap = getPrivateInstance(ctx, in_value);
            AccessControl access_granted = DEFAULT;
            if (wrap) {
                Local<ObjectTemplate> ot = wrap->m_object_template.Get(isolate);
                oi = ot.IsEmpty() ? nullptr : V82JSC::ToImpl<ObjectTemplateImpl>(ot);
            }
            
            if (wrap && wrap->m_isDetached) {
                allow = false;
                detached_behavior = true;
            } else if (wrap && wrap->m_reattached_global) {
                Local<Value> reattached = SecureValue(ValueImpl::New(V82JSC::ToContextImpl(accessing_context),
                                                                     wrap->m_reattached_global));
                in_value = (JSObjectRef) V82JSC::ToJSValueRef(reattached, accessing_context);
                allow = true;
            } else {
                if (argumentCount > 3) {
                    auto access_requested = static_cast<v8::AccessControl>(JSValueToNumber(ctx, arguments[2], 0));
                    JSObjectRef proto = in_value;
                    TrackedObjectImpl *proto_wrap = wrap;
                    while (JSValueIsObject(ctx, proto)) {
                        if (proto_wrap && proto_wrap->m_access_control) {
                            JSValueRef args[] = {
                                proto_wrap->m_access_control,
                                arguments[3]
                            };
                            JSValueRef access_granted_value = V82JSC::exec(ctx, "return _1[_2]", 2, args);
                            if (JSValueIsNumber(ctx, access_granted_value)) {
                                access_granted = static_cast<AccessControl>(JSValueToNumber(ctx, access_granted_value, 0));
                                break;
                            }
                        }
                        proto = (JSObjectRef) V82JSC::GetRealPrototype(accessing_context, proto);
                        if (JSValueIsObject(ctx, proto)) {
                            proto_wrap = getPrivateInstance(ctx, proto);
                        }
                    }
                    allow = (access_granted & access_requested) != 0;
                }
                
                // If we don't have our own access check, inherit from our global object
                TrackedObjectImpl *global_wrap = nullptr;
                AccessCheckCallback access_check = oi ? oi->m_access_check : nullptr;
                JSValueRef access_check_data = oi ? oi->m_access_check_data : 0;
                if (!access_check && (!wrap || !wrap->m_isGlobalObject)) {
                    JSObjectRef global = JSContextGetGlobalObject(orig_context);
                    global_wrap = getPrivateInstance(orig_context, global);
                    if (global_wrap && !global_wrap->m_object_template.IsEmpty()) {
                        ObjectTemplateImpl *otmpl = V82JSC::ToImpl<ObjectTemplateImpl>(global_wrap->m_object_template.Get(isolate));
                        access_check = otmpl->m_access_check;
                        access_check_data = otmpl->m_access_check_data;
                    }
                }

                if (global_wrap && global_wrap->m_isDetached) {
                    allow = !(JSContextGetGlobalContext(ctx) == orig_context);
                    detached_behavior = true;
                } else {
                    // STEP 1: If there is an access check callback, use that to decide
                    if (!allow && access_check) {
                        if (wrap->m_isGlobalObject && (JSContextGetGlobalContext(ctx) == orig_context)) {
                            allow = true;
                        } else {
                            Local<Value> accessing_data = ValueImpl::New(V82JSC::ToContextImpl(accessing_context), access_check_data);
                            allow = access_check(accessing_context, accessing_object, accessing_data);
                        }
                    // STEP 2: If no access check callback, see if security tokens match
                    } else if (!allow) {
                        Local<Value> accessing_token = accessing_context->GetSecurityToken();
                        Local<Context> orig_global_context = iso->m_global_contexts[orig_context].Get(isolate);
                        Local<Value> orig_token = orig_global_context->GetSecurityToken();
                        if (orig_token.IsEmpty() && (!wrap || !wrap->m_isGlobalObject)) {
                            allow = true;
                        } else if (orig_token.IsEmpty()) {
                            allow = false;
                        } else if (accessing_token.IsEmpty()) {
                            allow = orig_token.IsEmpty();
                        } else {
                            allow = accessing_token->StrictEquals(orig_token);
                        }
                    }
                }
            }
            if (allow && !strcmp(buffer, "apply") && detached_behavior) {
                return V82JSC::exec(ctx, "return function(){}", 0, nullptr);
            }
            if (allow && !strcmp(buffer, "getOwnPropertyDescriptor")) {
                JSValueRef proto = JSObjectGetPrototype(ctx, in_value);
                if (wrap && wrap->m_isGlobalObject && JSValueIsObject(ctx, proto)) {
                    proto = JSObjectGetPrototype(ctx, (JSObjectRef)proto);
                }
                JSValueRef args[] = {
                    in_value,
                    arguments[3],
                    proto,
                    JSValueMakeNumber(ctx, access_granted)
                };
                JSValueRef desc = 0;
                if (wrap && wrap->m_isGlobalObject) {
                    desc = V82JSC::exec(ctx,
                                        "var d = Object.getOwnPropertyDescriptor(_1, _2); "
                                        "if (d!==undefined) { d.configurable = true; return d; }"
                                        "d = Object.getOwnPropertyDescriptor(_3, _2); "
                                        "if (d!==undefined) {return {"
                                        "   value: _1[_2],"
                                        "   writable: (_4&2)==2,"
                                        "   enumerable: d.enumerable,"
                                        "   configurable: true"
                                        "}} else { return undefined; }",
                                        4, args);
                } else {
                    desc = V82JSC::exec(ctx, "return Object.getOwnPropertyDescriptor(_1, _2)", 2, args);
                }
                return desc;
            }
            if (!strcmp(buffer, "ownKeys")) {
                JSValueRef keys;
                if (allow) {
                    keys = V82JSC::exec(ctx, "return Reflect.ownKeys(_1)", 1, &in_value);
                } else if (wrap && wrap->m_access_control) {
                    keys = V82JSC::exec(ctx, "var keys=[]; for (e in _1) { if ((_1[e] & 1)==1) keys.push(e); } return keys;", 1, &wrap->m_access_control);
                } else {
                    keys = JSObjectMakeArray(ctx, 0, nullptr, 0);
                }
                return keys;
            }
            if (!allow) {
                int at =
                !strcmp(buffer, "get") ? AccessType::ACCESS_GET:
                !strcmp(buffer, "set") ? AccessType::ACCESS_SET:
                !strcmp(buffer, "has") ? AccessType::ACCESS_HAS:
                !strcmp(buffer, "getOwnPropertyDescriptor") ? AccessType::ACCESS_HAS:
                !strcmp(buffer, "ownKeys") ? AccessType::ACCESS_KEYS:
                !strcmp(buffer, "deleteProperty") ? AccessType::ACCESS_DELETE: -1;
                if (at>=0) {
                    if (iso->m_failed_access_check_callback) {
                        iso->m_failed_access_check_callback(accessing_object, static_cast<AccessType>(at), Local<Value>());
                    }
                }
                
                *exception = V82JSC::ToJSValueRef(Exception::TypeError
                                                  (String::NewFromUtf8(isolate, "access denied",
                                                                       NewStringType::kNormal).ToLocalChecked()), accessing_context);
                return NULL;
            }

            return in_value;
        };
        JSClassRef klass = JSClassCreate(&def);
        JSObjectRef accessor_func = JSObjectMake(ctx, klass, 0);
        
        JSValueRef args[] = {
            in_value,
            accessor_func
        };
        JSValueRef out_value = V82JSC::exec(ctx, security_proxy, 2, args);
        wrap = makePrivateInstance(V82JSC::ToIsolateImpl(isolate), ctx, (JSObjectRef)in_value);
        
        if (isActualGlobalObject) {
            if (!wrap->m_global_object_access_proxies) {
                wrap->m_global_object_access_proxies = JSObjectMakeArray(ctx, 1, &out_value, 0);
                JSValueProtect(ctx, wrap->m_global_object_access_proxies);
            } else {
                JSValueRef args[] = {
                    wrap->m_global_object_access_proxies,
                    out_value
                };
                V82JSC::exec(ctx, "_1.push(_2)", 2, args);
            }
        } else {
            if (!wrap->m_access_proxies) {
                wrap->m_access_proxies = JSObjectMakeArray(ctx, 1, &out_value, 0);
                JSValueProtect(ctx, wrap->m_access_proxies);
            } else {
                JSValueRef args[] = {
                    wrap->m_access_proxies,
                    out_value
                };
                V82JSC::exec(ctx, "_1.push(_2)", 2, args);
            }
            if (!wrap->m_proxy_security) {
                wrap->m_proxy_security = in_value;
            }
        }
        Local<Value> out = ValueImpl::New(V82JSC::ToContextImpl(context), out_value);
        return scope.Escape(out);
    }
    
    return in;
}
