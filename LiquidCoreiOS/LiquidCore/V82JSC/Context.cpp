/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "ObjectTemplate.h"
#include "Object.h"
#include "ArrayBuffer.h"
#include "Extension.h"
#include "JSCPrivate.h"

extern "C" unsigned char promise_polyfill_js[];
extern "C" unsigned char typedarray_js[];
extern "C" unsigned char error_polyfill_js[];

using namespace V82JSC;
using v8::Local;
using v8::Object;
using v8::Isolate;
using v8::EscapableHandleScope;
using v8::HandleScope;
using v8::Function;
using v8::Local;
using v8::MaybeLocal;

template<typename T>
static void WriteField(v8::internal::Object* ptr, int offset, T value) {
    uint8_t* addr =
    reinterpret_cast<uint8_t*>(ptr) + offset - v8::internal::kHeapObjectTag;
    *reinterpret_cast<T*>(addr) = value;
}
static JSValueRef DisallowCodeGenFromStrings(JSContextRef ctx, JSObjectRef proxy_function, JSObjectRef thisObject,
                                             size_t argumentCount, const JSValueRef *arguments, JSValueRef *exception);

/**
 * Returns the global proxy object.
 *
 * Global proxy object is a thin wrapper whose prototype points to actual
 * context's global object with the properties like Object, etc. This is done
 * that way for security reasons (for more details see
 * https://wiki.mozilla.org/Gecko:SplitWindow).
 *
 * Please note that changes to global proxy object prototype most probably
 * would break VM---v8 expects only global object as a prototype of global
 * proxy object.
 */
Local<Object> v8::Context::Global()
{
    auto impl = ToContextImpl(this);
    EscapableHandleScope scope(ToIsolate(impl->GetIsolate()));
    Local<Context> context = CreateLocal<Context>(&impl->GetIsolate()->ii, impl);
    Context::Scope context_scope(context);
    
    JSObjectRef glob = JSContextGetGlobalObject(impl->m_ctxRef);
    Local<Value> global = V82JSC::Value::New(impl, glob);
    global = V82JSC::TrackedObject::SecureValue(global);
    
    return scope.Escape(global.As<Object>());
}

/**
 * Detaches the global object from its context before
 * the global object can be reused to create a new context.
 */
void v8::Context::DetachGlobal()
{
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);
    auto impl = ToContextImpl(this);
    Local<Context> context = CreateLocal<Context>(isolate, impl);
    Context::Scope context_scope(context);
    
    JSObjectRef glob = JSContextGetGlobalObject(impl->m_ctxRef);
    auto wrap = V82JSC::TrackedObject::makePrivateInstance(ToIsolateImpl(isolate), impl->m_ctxRef, glob);
    wrap->m_isDetached = true;
}

FixedArray * get_embedder_data(const v8::Context* cx)
{
    typedef v8::internal::Object O;
    typedef v8::internal::Internals I;
    O* ctx = *reinterpret_cast<O* const*>(cx);
    int embedder_data_offset = I::kContextHeaderSize +
        (v8::internal::kApiPointerSize * I::kContextEmbedderDataIndex);
    O* embedder_data = I::ReadField<O*>(ctx, embedder_data_offset);
    if (embedder_data) {
        auto ed = reinterpret_cast<FixedArray*>(reinterpret_cast<uint8_t*>(embedder_data) - v8::internal::kHeapObjectTag);
        return ed;
    }
    return nullptr;
}

Local<v8::Context> LocalContext::New(Isolate *isolate, JSContextRef ctx)
{
    EscapableHandleScope scope(isolate);
    IsolateImpl * i = ToIsolateImpl(isolate);
    auto context = static_cast<Context *>(HeapAllocator::Alloc(i, i->m_context_map));
    context->m_ctxRef = ctx;

    // Copy the embedder data pointer from the global context.  This has a vulnerability if the embedder data
    // is moved (expanded) on the global context.  Unlikely scenario, though.
    Local<v8::Context> global_context = i->m_global_contexts[JSContextGetGlobalContext(context->m_ctxRef)].Get(isolate);
    if (!global_context.IsEmpty()) {
        auto ed = get_embedder_data(*global_context);
        int embedder_data_offset = v8::internal::Internals::kContextHeaderSize +
            (v8::internal::kApiPointerSize * v8::internal::Internals::kContextEmbedderDataIndex);
            v8::internal::Object * ctxi = ToHeapPointer(context);
        v8::internal::Object *embedder_data = reinterpret_cast<v8::internal::Object*>(reinterpret_cast<intptr_t>(ed) + v8::internal::kHeapObjectTag);
        WriteField<v8::internal::Object*>(ctxi, embedder_data_offset, embedder_data);
    }

    return scope.Escape(CreateLocal<v8::Context>(&i->ii, context));
}

void GlobalContext::RemoveContextFromIsolate(IsolateImpl *iso, JSGlobalContextRef ctx)
{
    iso->m_global_contexts.erase(ctx);
    iso->m_exec_maps.erase(ctx);
    // Don't do this.  Sometimes stray callbacks come in from JSC and we will need this after the context
    // is gone.
    //IsolateImpl::s_context_to_isolate_map.erase(ctx);
}

static Local<v8::Value> GetPrototypeSkipHidden(Local<v8::Context> context, Local<Object> thiz)
{
    JSValueRef obj = ToJSValueRef<v8::Value>(thiz, context);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef our_proto = GetRealPrototype(context, (JSObjectRef)obj);
    
    // If our prototype is hidden, propogate
    if (JSValueIsObject(ctx, our_proto)) {
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)our_proto);
        if (wrap && wrap->m_isHiddenPrototype) {
            return GetPrototypeSkipHidden(context, V82JSC::Value::New(ToContextImpl(context), our_proto).As<Object>());
        }
    }
    // Our prototype is not hidden
    return V82JSC::Value::New(ToContextImpl(context), our_proto);
}

static bool SetPrototypeSkipHidden(Local<v8::Context> context, Local<Object> thiz, Local<v8::Value> prototype)
{
    JSValueRef obj = ToJSValueRef<v8::Value>(thiz, context);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef new_proto = ToJSValueRef(prototype, context);
    JSValueRef our_proto = GetRealPrototype(context, (JSObjectRef)obj);
    // If our prototype is hidden, propogate
    bool isHidden = false;
    if (JSValueIsObject(ctx, our_proto)) {
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)our_proto);
        if (wrap && wrap->m_isHiddenPrototype) {
            return SetPrototypeSkipHidden(context, V82JSC::Value::New(ToContextImpl(context), our_proto).As<Object>(), prototype);
        }
    }
    
    bool new_proto_is_hidden = false;
    if (JSValueIsObject(ctx, new_proto)) {
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)new_proto);
        new_proto_is_hidden = wrap && wrap->m_isHiddenPrototype;
        if (new_proto_is_hidden) {
            if (JSValueIsStrictEqual(ctx, wrap->m_hidden_proxy_security, new_proto)) {
                // Don't put the hidden proxy in the prototype chain, just the underlying target object
                new_proto = wrap->m_proxy_security ? wrap->m_proxy_security : wrap->m_security;
            }
            // Save a reference to this object and propagate our own properties to it
            if (!wrap->m_hidden_children_array) {
                wrap->m_hidden_children_array = JSObjectMakeArray(ctx, 0, nullptr, 0);
            }
            JSValueRef args[] = { wrap->m_hidden_children_array, obj };
            exec(ctx, "_1.push(_2)", 2, args);
            ToImpl<V82JSC::HiddenObject>(V82JSC::Value::New(ToContextImpl(context), new_proto))
            ->PropagateOwnPropertiesToChild(context, (JSObjectRef)obj);
        }
    }
    
    // Our prototype is not hidden
    if (!isHidden) {
        SetRealPrototype(context, (JSObjectRef)obj, new_proto);
    }
    
    return new_proto_is_hidden || GetPrototypeSkipHidden(context, thiz)->StrictEquals(prototype);
}


/**
 * Creates a new context and returns a handle to the newly allocated
 * context.
 *
 * \param isolate The isolate in which to create the context.
 *
 * \param extensions An optional extension configuration containing
 * the extensions to be installed in the newly created context.
 *
 * \param global_template An optional object template from which the
 * global object for the newly created context will be created.
 *
 * \param global_object An optional global object to be reused for
 * the newly created context. This global object must have been
 * created by a previous call to Context::New with the same global
 * template. The state of the global object will be completely reset
 * and only object identify will remain.
 */
Local<v8::Context> v8::Context::New(Isolate* isolate, ExtensionConfiguration* extensions,
                          MaybeLocal<ObjectTemplate> global_template,
                          MaybeLocal<Value> global_object)
{
    EscapableHandleScope scope(isolate);
    IsolateImpl * i = ToIsolateImpl(isolate);
    auto context = static_cast<GlobalContext *>(HeapAllocator::Alloc(i, i->m_global_context_map));
    
    assert(ToHeapPointer(context)->IsContext());

    Local<Context> ctx = CreateLocal<Context>(isolate, context);
    int hash = 0;
    
    V82JSC::TrackedObject *global_wrap = nullptr;
    if (!global_object.IsEmpty()) {
        hash = global_object.ToLocalChecked().As<Object>()->GetIdentityHash();
        global_wrap = V82JSC::TrackedObject::getPrivateInstance(context->GetNullContext(),
                                         (JSObjectRef) ToJSValueRef(global_object.ToLocalChecked(), ctx));
        global_template = global_wrap->m_object_template.Get(isolate);
    }

    Context::Scope context_scope(ctx);

    if (!global_template.IsEmpty()) {
        auto impl = ToImpl<V82JSC::ObjectTemplate>(*global_template.ToLocalChecked());
        LocalException exception(ToIsolateImpl(isolate));
        
        // Temporarily disable access checks until we are done setting up the object
        DisableAccessChecksScope disable_scope(i, impl);
        
        JSClassDefinition def = kJSClassDefinitionEmpty;
        JSClassRef claz = JSClassCreate(&def);
        context->m_ctxRef = JSGlobalContextCreateInGroup(i->m_group, claz);
        JSClassRelease(claz);
        JSContextRef ctxRef = context->m_ctxRef;
        {
            std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
            IsolateImpl::s_context_to_isolate_map[(JSGlobalContextRef)context->m_ctxRef] = i;
        }
        i->m_exec_maps[(JSGlobalContextRef)context->m_ctxRef] = std::map<const char*, JSObjectRef>();

        def = kJSClassDefinitionEmpty;
        def.className = "GlobalObject";
        MaybeLocal<Object> thiz = impl->NewInstance(ctx, 0, false, &def, 0);
        JSObjectRef instance = (JSObjectRef) ToJSValueRef(thiz.ToLocalChecked(), ctx);
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(context->m_ctxRef, instance);
        wrap->m_isGlobalObject = true;
        if (hash) {
            wrap->m_hash = hash;
        }
        // Don't use the ES6 proxy, if there is one.  JSC won't allow it in the global prototype chain
        instance = (JSObjectRef)wrap->m_security;
        thiz = V82JSC::Value::New(reinterpret_cast<V82JSC::Context*>(context), instance).As<Object>();

        auto ctortempl = Local<FunctionTemplate>::New(isolate, impl->m_constructor_template);
        if (!ctortempl.IsEmpty()) {
            MaybeLocal<Function> ctor = ctortempl->GetFunction(ctx);
            if (!ctor.IsEmpty()) {
                JSObjectRef ctor_func = (JSObjectRef) ToJSValueRef(ctor.ToLocalChecked(), ctx);
                JSStringRef sprototype = JSStringCreateWithUTF8CString("prototype");
                JSStringRef sconstructor = JSStringCreateWithUTF8CString("constructor");
                JSValueRef excp = 0;
                JSValueRef prototype = JSObjectGetProperty(ctxRef, ctor_func, sprototype, &excp);
                assert(excp == 0);
                JSObjectSetPrototype(ctxRef, instance, prototype);
                JSObjectSetProperty(ctxRef, instance, sconstructor, ctor_func, kJSPropertyAttributeDontEnum, &excp);
                assert(excp == 0);
                JSStringRelease(sprototype);
                JSStringRelease(sconstructor);
            }
        }

        // Don't use the access control proxy on the global object.  It is always accessible to the current context
        // and proxies in the global prototype chain are not allowed in JSC.  We will use it, however, if we pass the
        // object to another context.
        if (wrap->m_access_proxies) {
            thiz = V82JSC::Value::New(reinterpret_cast<V82JSC::Context*>(context),
                                  wrap->m_proxy_security).As<Object>();
        }
        
        // Don't use SetRealPrototype() or GetRealPrototype() here because we haven't set them up yet,
        // but otherwise, always use them over the JSC equivalents.  The JSC API legacy functions get confused
        // by proxies
        JSObjectRef global = (JSObjectRef) JSObjectGetPrototype(context->m_ctxRef, JSContextGetGlobalObject(context->m_ctxRef));
        JSObjectSetPrototype(context->m_ctxRef, global, ToJSValueRef(thiz.ToLocalChecked(), ctx));

        if (exception.ShouldThrow()) {
            return scope.Escape(Local<Context>());
        }
    } else {
        JSClassDefinition def = kJSClassDefinitionEmpty;
        JSClassRef claz = JSClassCreate(&def);
        context->m_ctxRef = JSGlobalContextCreateInGroup(i->m_group, claz);
        {
            std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
            IsolateImpl::s_context_to_isolate_map[(JSGlobalContextRef)context->m_ctxRef] = i;
        }
        JSClassRelease(claz);
    }
    
    JSObjectRef global_o = JSContextGetGlobalObject(context->m_ctxRef);

    // Set a reference back to our context so we can find our way back to the creation context
    if (i->m_creation_contexts) {
        JSClassDefinition def = kJSClassDefinitionEmpty;
        JSClassRef claz = JSClassCreate(&def);
        context->m_creation_context = JSObjectMake(context->m_ctxRef, claz, (void*)context->m_ctxRef);
        JSValueProtect(context->m_ctxRef, context->m_creation_context);
        JSValueRef args[] = {
            i->m_creation_contexts,
            global_o,
            context->m_creation_context
        };
        exec(context->m_ctxRef, "_1.set(_2, _3)", 3, args);
        JSClassRelease(claz);
    }

    if (!global_object.IsEmpty()) {
        global_wrap->m_isDetached = false;
        global_wrap->m_reattached_global = global_o;
        JSValueProtect(context->m_ctxRef, global_wrap->m_reattached_global);
    }

    {
        v8::Persistent<v8::Context> persistent(isolate, ctx);
        persistent.SetWeak();
        i->m_global_contexts[(JSGlobalContextRef)context->m_ctxRef].Reset(isolate,persistent);
    }
    
    context->m_security_token.Reset();

    // Only set a single proxy target map so it can be accessed from any context
    if (i->m_nullContext.IsEmpty()) {
        context->m_proxy_targets = exec(context->m_ctxRef, "return new WeakMap()", 0, 0);
    } else {
        context->m_proxy_targets = ToGlobalContextImpl(i->m_nullContext.Get(isolate))->m_proxy_targets;
        JSValueProtect(context->m_ctxRef, context->m_proxy_targets);
    }
    
    // Don't do anything fancy if we are setting up the default context
    if (!i->m_nullContext.IsEmpty()) {
        // Capture all proxy targets in a WeakMap
        // The irony of proxying Proxy is not lost on me
        exec(context->m_ctxRef,
             "const handler = { " \
             "  construct(t,a,n) { " \
             "    let proxy = Reflect.construct(t,a,n); " \
             "    _1.set(proxy,t); " \
             "    return proxy; " \
             "  } " \
             "}; " \
             "Proxy = new Proxy(Proxy, handler)",
             1, &context->m_proxy_targets);
        
        proxyArrayBuffer(context);
        
        // Proxy Object.getPrototypeOf and Object.setPrototypeOf
        Local<FunctionTemplate> setPrototypeOf = FunctionTemplate::New(
            isolate,
            [](const FunctionCallbackInfo<Value>& info) {
                Local<Object> obj = info[0].As<Object>();
                Local<Value> proto = info[1];
                JSContextRef ctx = ToContextRef(info.GetIsolate());
                Local<Context> context = info.GetIsolate()->GetCurrentContext();
                JSValueRef o = ToJSValueRef(info[0], context);

                if (JSValueIsObject(ctx, o)) {
                    JSValueRef args[] = {
                        ToIsolateImpl(info.GetIsolate())->m_creation_contexts,
                        o
                    };
                    JSValueRef cc = exec(ctx, "return _1.get(_2)", 2, args);
                    JSGlobalContextRef gctx = 0;
                    if (JSValueIsObject(ctx, cc)) gctx = (JSGlobalContextRef)JSObjectGetPrivate((JSObjectRef)cc);
                    
                    if (gctx && ToIsolateImpl(info.GetIsolate())->m_global_contexts.count(gctx)) {
                        Local<Context> other = ToIsolateImpl(info.GetIsolate())->
                            m_global_contexts[gctx].Get(info.GetIsolate());
                        bool isGlobal = other->Global()->StrictEquals(info[0]);
                        if (isGlobal && !context->Global()->StrictEquals(info[0])) {
                            info.GetReturnValue().Set(False(info.GetIsolate()));
                            return;
                        } else if (isGlobal) {
                            obj = obj->GetPrototype().As<Object>();
                        }
                    }
                }
                
                bool in_global_prototype_chain = false;
                JSObjectRef global = JSContextGetGlobalObject(ctx);
                while (JSValueIsObject(ctx, global) && !in_global_prototype_chain) {
                    in_global_prototype_chain = JSValueIsStrictEqual(ctx, global, o);
                    global = (JSObjectRef) JSObjectGetPrototype(ctx, global);
                }
                if (in_global_prototype_chain) {
                    // We can't have proxies in the global prototype chain.  If this is proxied,
                    // remove the ES6 proxy.  We will rely on the legacy proxy instead.
                    JSObjectRef maybe_proxy = (JSObjectRef) ToJSValueRef(proto, context);
                    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, maybe_proxy);
                    if (wrap) {
                        proto = V82JSC::Value::New(ToContextImpl(context), wrap->m_security);
                    }
                }

                if (SetPrototypeSkipHidden(info.GetIsolate()->GetCurrentContext(), obj, proto)) {
                    info.GetReturnValue().Set(True(info.GetIsolate()));
                } else {
                    info.GetReturnValue().Set(False(info.GetIsolate()));
                }
            });
        Local<FunctionTemplate> getPrototypeOf = FunctionTemplate::New(
            isolate,
            [](const FunctionCallbackInfo<Value>& info) {
                JSContextRef ctx = ToContextRef(info.GetIsolate());
                Local<Context> context = info.GetIsolate()->GetCurrentContext();
                JSValueRef o = ToJSValueRef(info[0], context);
                
                if (JSValueIsObject(ctx, o)) {
                    JSValueRef args[] = {
                        ToIsolateImpl(info.GetIsolate())->m_creation_contexts,
                        o
                    };
                    JSValueRef cc = exec(ctx, "return _1.get(_2)", 2, args);
                    JSGlobalContextRef gctx = 0;
                    if (JSValueIsObject(ctx, cc)) gctx = (JSGlobalContextRef)JSObjectGetPrivate((JSObjectRef)cc);

                    if (gctx && ToIsolateImpl(info.GetIsolate())->m_global_contexts.count(gctx)) {
                        Local<Context> other = ToIsolateImpl(info.GetIsolate())->m_global_contexts[gctx].Get(info.GetIsolate());
                        bool isGlobal = other->Global()->StrictEquals(info[0]);
                        if (isGlobal && !context->Global()->StrictEquals(info[0])) {
                            info.GetReturnValue().Set(Null(info.GetIsolate()));
                            return;
                        }
                    }
                }

                Local<Object> obj = info[0].As<Object>();
                info.GetReturnValue().Set(GetPrototypeSkipHidden(info.GetIsolate()->GetCurrentContext(), obj));
            });
        // Don't use ctx->Global() here because it may return a proxy, which we can't use in setting up the global object
        Local<Object> global = V82JSC::Value::New(reinterpret_cast<V82JSC::Context*>(context),
                                              JSContextGetGlobalObject(context->m_ctxRef)).As<Object>();
        Local<Object> object = global->Get(ctx,
            String::NewFromUtf8(isolate, "Object", NewStringType::kNormal).ToLocalChecked()).ToLocalChecked().As<Object>();
        Local<String> SsetPrototypeOf = String::NewFromUtf8(isolate, "setPrototypeOf", NewStringType::kNormal).ToLocalChecked();
        Local<String> SgetPrototypeOf = String::NewFromUtf8(isolate, "getPrototypeOf", NewStringType::kNormal).ToLocalChecked();

        context->ObjectSetPrototypeOf.Reset(isolate, object->Get(ctx, SsetPrototypeOf).ToLocalChecked().As<Function>());
        context->ObjectGetPrototypeOf.Reset(isolate, object->Get(ctx, SgetPrototypeOf).ToLocalChecked().As<Function>());
        CHECK(object->Set(ctx,
                          SsetPrototypeOf,
                          setPrototypeOf->GetFunction(ctx).ToLocalChecked()).ToChecked());
        CHECK(object->Set(ctx,
                          SgetPrototypeOf,
                          getPrototypeOf->GetFunction(ctx).ToLocalChecked()).ToChecked());
        
        // ... and capture all attempts to set the prototype through __proto__
        exec(context->m_ctxRef,
                     "Object.defineProperty( Object.prototype, '__proto__',"
                     "{"
                     "  get() { return Object.getPrototypeOf(this); },"
                     "  set(p) { return Object.setPrototypeOf(this, p); },"
                     "  enumerable: false,"
                     "  configurable: false"
                     "});", 0, nullptr);
        
        // Filter out our private symbol.  Nobody needs to see that.
        exec(context->m_ctxRef,
                     "var old = Object.getOwnPropertySymbols; "
                     "Object.getOwnPropertySymbols = "
                     "    (o) => old(o).filter( (s)=> s!= _1 )",
                     1, &i->m_private_symbol);
        
        auto ctximpl = reinterpret_cast<V82JSC::Context*>(context);
        // Save a reference to original Object.prototype.toString()
        JSValueRef toString = exec(context->m_ctxRef, "return Object.prototype.toString", 0, nullptr);
        context->ObjectPrototypeToString.Reset(isolate, V82JSC::Value::New(ctximpl, toString).As<Function>());
        
        // Override Function.prototype.bind()
        // All we are doing intercepting calls to bind and then calling the original bind and returning the
        // value.  But we need to ensure that the creation context of the bound object is the same as the
        // creation context of the function.
        JSValueRef bind = exec(context->m_ctxRef, "return Function.prototype.bind", 0, nullptr);
        context->FunctionPrototypeBind.Reset(isolate, V82JSC::Value::New(ctximpl, bind).As<Function>());
        Local<FunctionTemplate> bind_template = FunctionTemplate::New(isolate, [](const FunctionCallbackInfo<Value>& info) {
            Local<Context> context = info.This()->CreationContext();
            Context::Scope context_scope(context);
            Local<Value> args[info.Length()];
            for (int i=0; i<info.Length(); i++) {
                args[i] = info[i];
            }
            MaybeLocal<Value> bound = ToGlobalContextImpl(context)->FunctionPrototypeBind.Get(info.GetIsolate())
                ->Call(context, info.This(), info.Length(), args);
            if (!bound.IsEmpty()) {
                JSObjectRef bf = (JSObjectRef) ToJSValueRef(bound.ToLocalChecked(), context);
                JSObjectRef bound_function = (JSObjectRef) ToJSValueRef(info.This(), context);
                auto wrap = V82JSC::TrackedObject::makePrivateInstance(ToIsolateImpl(info.GetIsolate()),
                                                              ToContextRef(context), bf);
                wrap->m_bound_function = bound_function;
                JSValueProtect(ToContextRef(context), wrap->m_bound_function);
                
                info.GetReturnValue().Set(bound.ToLocalChecked());
            }
        });
        JSValueRef new_bind = ToJSValueRef(bind_template->GetFunction(ctx).ToLocalChecked(), ctx);
        exec(context->m_ctxRef, "Function.prototype.bind = _1", 1, &new_bind);
        
        JSValueRef eval = exec(context->m_ctxRef, "return eval", 0, nullptr);
        context->Eval.Reset(isolate, V82JSC::Value::New(reinterpret_cast<V82JSC::Context*>(context), eval).As<v8::Function>());
        
        JSStringRef e = JSStringCreateWithUTF8CString("Function");
        JSClassDefinition def = kJSClassDefinitionEmpty;
        def.attributes |= kJSClassAttributeNoAutomaticPrototype;
        def.callAsFunction = [](JSContextRef ctx, JSObjectRef proxy_function, JSObjectRef thisObject,
                                size_t argumentCount, const JSValueRef *arguments, JSValueRef *exception) ->JSValueRef
        {
            IsolateImpl* iso =IsolateFromCtx(ctx);
            Isolate *isolate = ToIsolate(ToIsolate(iso));
            
            HandleScope scope(isolate);
            
            Local<Context> global_context = iso->m_global_contexts[JSContextGetGlobalContext(ctx)].Get(isolate);
            auto context = ToImpl<V82JSC::GlobalContext>(global_context);
            bool allow;
            if (iso->m_allow_code_gen_callback) {
                Local<String> source = V82JSC::Value::New(reinterpret_cast<V82JSC::Context*>(context),
                                                      arguments[0]).As<String>();
                allow = iso->m_allow_code_gen_callback(global_context, source);
            } else {
                allow = !context->m_code_eval_from_strings_disallowed;
            }
            
            if (!allow) {
                return DisallowCodeGenFromStrings(ctx, proxy_function, thisObject, argumentCount, arguments, exception);
            } else {
                return exec(ctx, "return Reflect.construct(_1, _2, _3)",
                                    (int)argumentCount, arguments, exception);
            }
        };
        JSClassRef klass = JSClassCreate(&def);
        JSValueRef FunctionCtor = JSObjectMake(context->m_ctxRef, klass, 0);
        JSStringRelease(e);
        /* FIXME: This is some esoteric functionality in V8.  The solution breaks React Native
         * because (class A {}).constructor !== Function, which it should be.  We should only
         * do this if the isolate explicity requires it.
         */
        /*
        exec(context->m_ctxRef,
                     "const handler = { construct: _1 };"
                     "Function = new Proxy(Function, handler);",
                     1, &FunctionCtor);
        */
        
        JSStringRef zGlobal = JSStringCreateWithUTF8CString("global");
        JSStringRef zSetTimeout = JSStringCreateWithUTF8CString("setTimeout");
        JSStringRef zPromise = JSStringCreateWithUTF8CString("Promise");
        JSStringRef zPromisePolyfill = JSStringCreateWithUTF8CString((const char*)promise_polyfill_js);

        JSObjectRef setTimeout = JSObjectMakeFunctionWithCallback
        (context->m_ctxRef, zSetTimeout,
         [](JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
            size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) ->JSValueRef {
             
             IsolateImpl* iso = IsolateFromCtx(ctx);
             Isolate *isolate = ToIsolate(ToIsolate(iso));
             HandleScope scope(isolate);
             Local<Context> ctxt = LocalContext::New(isolate, ctx);
             
             isolate->EnqueueMicrotask(V82JSC::Value::New(ToContextImpl(ctxt), arguments[0]).As<Function>());
             return JSValueMakeUndefined(ctx);
         });
        JSValueRef excp = 0;
        JSObjectSetProperty(context->m_ctxRef, global_o, zSetTimeout, setTimeout, 0, &excp);
        assert(excp == 0);
        JSObjectSetProperty(context->m_ctxRef, global_o, zGlobal, global_o, 0, &excp);
        assert(excp == 0);
/*
        JSObjectDeleteProperty(context->m_ctxRef, global_o, zPromise, &excp);
        assert(excp == 0);
        JSEvaluateScript(context->m_ctxRef, zPromisePolyfill, global_o, 0, 0, &excp);
        assert(excp == 0);
*/

        JSStringRef zTypedArrayPolyfill = JSStringCreateWithUTF8CString((const char*)typedarray_js);
        JSEvaluateScript(context->m_ctxRef, zTypedArrayPolyfill, global_o, 0, 0, &excp);
        assert(excp == 0);

        JSStringRef zErrorPolyfill = JSStringCreateWithUTF8CString((const char*)error_polyfill_js);
        JSEvaluateScript(context->m_ctxRef, zErrorPolyfill, global_o, 0, 0, &excp);
        assert(excp == 0);

        JSObjectDeleteProperty(context->m_ctxRef, global_o, zSetTimeout, &excp);
        assert(excp == 0);

        JSStringRelease(zPromise);
        JSStringRelease(zPromisePolyfill);
        JSStringRelease(zTypedArrayPolyfill);
        JSStringRelease(zErrorPolyfill);
        JSStringRelease(zSetTimeout);
        JSStringRelease(zGlobal);
        
        std::map<std::string, bool> loaded_extensions;

        InstallAutoExtensions(ctx, loaded_extensions);
        if (extensions) {
            for (const char **extension = extensions->begin(); extension != extensions->end(); extension++) {
                if (!InstallExtension(ctx, *extension, loaded_extensions)) {
                    return scope.Escape(Local<Context>());
                }
            }
        }
    }
    
    return scope.Escape(ctx);
}

/**
 * Create a new context from a (non-default) context snapshot. There
 * is no way to provide a global object template since we do not create
 * a new global object from template, but we can reuse a global object.
 *
 * \param isolate See v8::Context::New.
 *
 * \param context_snapshot_index The index of the context snapshot to
 * deserialize from. Use v8::Context::New for the default snapshot.
 *
 * \param embedder_fields_deserializer Optional callback to deserialize
 * internal fields. It should match the SerializeInternalFieldCallback used
 * to serialize.
 *
 * \param extensions See v8::Context::New.
 *
 * \param global_object See v8::Context::New.
 */

MaybeLocal<v8::Context> v8::Context::FromSnapshot(
                                        Isolate* isolate, size_t context_snapshot_index,
                                        DeserializeInternalFieldsCallback embedder_fields_deserializer,
                                        ExtensionConfiguration* extensions,
                                                  MaybeLocal<v8::Value> global_object)
{
    // Snashots are ignored
    Local<Context> ctx = New(isolate, extensions, MaybeLocal<ObjectTemplate>(), global_object);
    return MaybeLocal<Context>(ctx);
}

/**
 * Returns an global object that isn't backed by an actual context.
 *
 * The global template needs to have access checks with handlers installed.
 * If an existing global object is passed in, the global object is detached
 * from its context.
 *
 * Note that this is different from a detached context where all accesses to
 * the global proxy will fail. Instead, the access check handlers are invoked.
 *
 * It is also not possible to detach an object returned by this method.
 * Instead, the access check handlers need to return nothing to achieve the
 * same effect.
 *
 * It is possible, however, to create a new context from the global object
 * returned by this method.
 */
MaybeLocal<Object> v8::Context::NewRemoteContext(
                                           Isolate* isolate, Local<ObjectTemplate> global_template,
                                           MaybeLocal<Value> global_object)
{
    assert(0);
    return MaybeLocal<Object>();
}

/**
 * Sets the security token for the context.  To access an object in
 * another context, the security tokens must match.
 */
void v8::Context::SetSecurityToken(Local<Value> token)
{
    auto ctximpl = ToContextImpl(this);
    Isolate* isolate = ToIsolate(ctximpl);
    IsolateImpl* iso = ToIsolateImpl(isolate);
    HandleScope scope(isolate);
    
    JSGlobalContextRef gctx = JSContextGetGlobalContext(ctximpl->m_ctxRef);
    Local<Context> global_context = iso->m_global_contexts[gctx].Get(isolate);
    auto impl = ToImpl<V82JSC::GlobalContext>(global_context);
    
    impl->m_security_token.Reset(isolate, token);
}

/** Restores the security token to the default value. */
void v8::Context::UseDefaultSecurityToken()
{
    assert(0);
}

/** Returns the security token of this context.*/
Local<v8::Value> v8::Context::GetSecurityToken()
{
    auto ctximpl = ToContextImpl(this);
    Isolate* isolate = ToIsolate(ctximpl);
    IsolateImpl* iso = ToIsolateImpl(isolate);
    EscapableHandleScope scope(isolate);
    
    JSGlobalContextRef gctx = JSContextGetGlobalContext(ctximpl->m_ctxRef);
    Local<Context> global_context = iso->m_global_contexts[gctx].Get(isolate);
    auto impl = ToImpl<V82JSC::GlobalContext>(global_context);
    
    return scope.Escape(impl->m_security_token.Get(isolate));
}

/**
 * Enter this context.  After entering a context, all code compiled
 * and run is compiled and run in this context.  If another context
 * is already entered, this old context is saved so it can be
 * restored when the new context is exited.
 */
void v8::Context::Enter()
{
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> thiz = Local<Context>::New(isolate, this);
    ToIsolateImpl(isolate)->EnterContext(thiz);
}

/**
 * Exit this context.  Exiting the current context restores the
 * context that was in place when entering the current context.
 */
void v8::Context::Exit()
{
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> thiz = Local<Context>::New(isolate, this);
    ToIsolateImpl(isolate)->ExitContext(thiz);
}

/** Returns an isolate associated with a current context. */
Isolate* v8::Context::GetIsolate()
{
    return ToIsolate(this);
}

/**
 * Gets the binding object used by V8 extras. Extra natives get a reference
 * to this object and can use it to "export" functionality by adding
 * properties. Extra natives can also "import" functionality by accessing
 * properties added by the embedder using the V8 API.
 */
Local<Object> v8::Context::GetExtrasBindingObject()
{
    assert(0);
    return Local<Object>();
}

/**
 * Sets the embedder data with the given index, growing the data as
 * needed. Note that index 0 currently has a special meaning for Chrome's
 * debugger.
 */
void v8::Context::SetEmbedderData(int index, Local<Value> value)
{
    Isolate *isolate = ToIsolate(this);
    IsolateImpl *i = ToIsolateImpl(isolate);
    auto ctximpl = ToContextImpl(this);

    if ((JSContextRef)JSContextGetGlobalContext(ctximpl->m_ctxRef) != ctximpl->m_ctxRef) {
        i->m_global_contexts[JSContextGetGlobalContext(ctximpl->m_ctxRef)].Get(isolate)->SetEmbedderData(index,value);
        return;
    }
    
    auto ed = get_embedder_data(this);
    
    if (!ed || ed->m_size <= index) {
        int buffer = ReserveSize((index+1) * sizeof(v8::internal::Object*) + sizeof(FixedArray));
        int size = (buffer - sizeof(FixedArray)) / sizeof(v8::internal::Object*);
        auto  newed = reinterpret_cast<FixedArray *>
            (HeapAllocator::Alloc(i, i->m_fixed_array_map,
                                     sizeof(FixedArray) + size*sizeof(v8::internal::Object *)));
        if (ed) {
            memcpy(newed->m_elements, ed->m_elements, ed->m_size * sizeof(v8::internal::Object*));
        }
        newed->m_size = size;
        ed = newed;
    }
    
    v8::internal::Object * val = *reinterpret_cast<v8::internal::Object* const*>(*value);
    ed->m_elements[index] = val;
    
    Local<v8::EmbeddedFixedArray> fa = CreateLocal<v8::EmbeddedFixedArray>(isolate, ed);
    auto context = ToGlobalContextImpl(this);
    context->m_embedder_data.Reset(isolate, fa);
    
    int embedder_data_offset = v8::internal::Internals::kContextHeaderSize +
        (v8::internal::kApiPointerSize * v8::internal::Internals::kContextEmbedderDataIndex);
    v8::internal::Object * ctx = *reinterpret_cast<v8::internal::Object* const*>(this);
    v8::internal::Object *embedder_data = reinterpret_cast<v8::internal::Object*>(reinterpret_cast<intptr_t>(ed) + v8::internal::kHeapObjectTag);
    WriteField<v8::internal::Object*>(ctx, embedder_data_offset, embedder_data);
}

/**
 * Sets a 2-byte-aligned native pointer in the embedder data with the given
 * index, growing the data as needed. Note that index 0 currently has a
 * special meaning for Chrome's debugger.
 */
void v8::Context::SetAlignedPointerInEmbedderData(int index, void* value)
{
    value = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(value) & ~1);
    class FakeLocal {
    public:
        Value* val_;
    };
    FakeLocal fl;
    
    fl.val_ = reinterpret_cast<Value*>(&value);

    Local<Value> * v = reinterpret_cast<Local<Value>*>(&fl);
    SetEmbedderData(index, *v);
}

static JSValueRef DisallowCodeGenFromStrings(JSContextRef ctx, JSObjectRef proxy_function, JSObjectRef thisObject,
                                             size_t argumentCount, const JSValueRef *arguments, JSValueRef *exception)
{
    IsolateImpl* iso = IsolateFromCtx(ctx);
    Isolate *isolate = ToIsolate(ToIsolate(iso));
    
    HandleScope scope(isolate);
    
    Local<v8::Context> global_context = iso->m_global_contexts[JSContextGetGlobalContext(ctx)].Get(isolate);
    auto context = ToImpl<V82JSC::GlobalContext>(global_context);
    Local<v8::String> error = context->m_code_gen_error.Get(isolate);
    if (error.IsEmpty()) {
        error = v8::String::NewFromUtf8(isolate, "code generation from strings not allowed",
                                        v8::NewStringType::kNormal).ToLocalChecked();
    }
    JSValueRef msg = ToJSValueRef(error, global_context);
    *exception = exec(ctx, "return new EvalError(_1)", 1, &msg);

    return NULL;
}

/**
 * Control whether code generation from strings is allowed. Calling
 * this method with false will disable 'eval' and the 'Function'
 * constructor for code running in this context. If 'eval' or the
 * 'Function' constructor are used an exception will be thrown.
 *
 * If code generation from strings is not allowed the
 * V8::AllowCodeGenerationFromStrings callback will be invoked if
 * set before blocking the call to 'eval' or the 'Function'
 * constructor. If that callback returns true, the call will be
 * allowed, otherwise an exception will be thrown. If no callback is
 * set an exception will be thrown.
 */
void v8::Context::AllowCodeGenerationFromStrings(bool allow)
{
    auto local_ctx = ToContextImpl(this);
    Isolate *isolate = ToIsolate(local_ctx);
    
    HandleScope scope(isolate);
    
    Local<Context> global_context = ToIsolateImpl(isolate)->
        m_global_contexts[JSContextGetGlobalContext(local_ctx->m_ctxRef)].Get(isolate);
    auto context = ToImpl<V82JSC::GlobalContext>(global_context);
    
    JSStringRef e = JSStringCreateWithUTF8CString("eval");
    JSObjectRef evalf = (JSObjectRef) ToJSValueRef(context->Eval.Get(isolate), global_context);
    if (!allow) {
        JSClassDefinition def = kJSClassDefinitionEmpty;
        def.attributes |= kJSClassAttributeNoAutomaticPrototype;
        def.callAsFunction = [](JSContextRef ctx, JSObjectRef proxy_function, JSObjectRef thisObject,
                                size_t argumentCount, const JSValueRef *arguments, JSValueRef *exception) ->JSValueRef
        {
            IsolateImpl* iso = IsolateFromCtx(ctx);
            Isolate *isolate = ToIsolate(ToIsolate(iso));
            
            HandleScope scope(isolate);
            
            Local<Context> global_context = iso->m_global_contexts[JSContextGetGlobalContext(ctx)].Get(isolate);
            auto context = ToImpl<V82JSC::GlobalContext>(global_context);
            bool allow;
            if (iso->m_allow_code_gen_callback) {
                Local<v8::String> source = V82JSC::Value::New(reinterpret_cast<V82JSC::Context*>(context),
                                                      arguments[0]).As<String>();
                allow = iso->m_allow_code_gen_callback(global_context, source);
            } else {
                allow = !context->m_code_eval_from_strings_disallowed;
            }
            
            if (!allow) {
                return DisallowCodeGenFromStrings(ctx, proxy_function, thisObject, argumentCount, arguments, exception);
            } else {
                JSObjectRef eval = (JSObjectRef) ToJSValueRef(context->Eval.Get(isolate), global_context);
                return JSObjectCallAsFunction(ctx, eval, thisObject, argumentCount, arguments, exception);
            }
        };
        JSClassRef klass = JSClassCreate(&def);
        evalf = JSObjectMake(context->m_ctxRef, klass, 0);
    }
    JSObjectSetProperty(context->m_ctxRef, JSContextGetGlobalObject(context->m_ctxRef), e, evalf, 0, 0);
    JSStringRelease(e);
    context->m_code_eval_from_strings_disallowed = !allow;
}

/**
 * Returns true if code generation from strings is allowed for the context.
 * For more details see AllowCodeGenerationFromStrings(bool) documentation.
 */
bool v8::Context::IsCodeGenerationFromStringsAllowed()
{
    auto local_ctx = ToContextImpl(this);
    Isolate *isolate = ToIsolate(local_ctx);
    
    HandleScope scope(isolate);
    
    Local<Context> global_context = ToIsolateImpl(isolate)->
    m_global_contexts[JSContextGetGlobalContext(local_ctx->m_ctxRef)].Get(isolate);
    auto context = ToImpl<V82JSC::GlobalContext>(global_context);
    return !context->m_code_eval_from_strings_disallowed;
}

/**
 * Sets the error description for the exception that is thrown when
 * code generation from strings is not allowed and 'eval' or the 'Function'
 * constructor are called.
 */
void v8::Context::SetErrorMessageForCodeGenerationFromStrings(Local<String> message)
{
    auto local_ctx = ToContextImpl(this);
    Isolate *isolate = ToIsolate(local_ctx);
    
    HandleScope scope(isolate);
    
    Local<Context> global_context = ToIsolateImpl(isolate)->
        m_global_contexts[JSContextGetGlobalContext(local_ctx->m_ctxRef)].Get(isolate);
    auto context = ToImpl<V82JSC::GlobalContext>(global_context);
    context->m_code_gen_error.Reset(isolate, message);
}
