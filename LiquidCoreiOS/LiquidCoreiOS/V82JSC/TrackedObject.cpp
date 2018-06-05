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
        IsolateImpl *iso = IsolateImpl::s_context_to_isolate_map[ctx];
        assert(iso);

        HandleScope scope(V82JSC::ToIsolate(iso));
        Local<TrackedObject> local = V82JSC::FromPersistentData<TrackedObject>(V82JSC::ToIsolate(iso), persistent);
        assert(!local.IsEmpty());
        TrackedObjectImpl *impl = V82JSC::ToImpl<TrackedObjectImpl>(local);
        iso->weakGoneInactive(ctx, (JSObjectRef) impl->m_security, impl->m_embedder_data);
        
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
    IsolateImpl *iso = IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)];
    JSValueRef args[] = {
        object,
        iso->m_private_symbol
    };
    JSObjectRef private_object = (JSObjectRef) V82JSC::exec(ctx, "return _1[_2]", 2, args);
    if (JSValueIsObject(ctx, private_object)) {
        void * persistent = JSObjectGetPrivate(private_object);
        if (!persistent) return nullptr;

        Local<TrackedObject> local = V82JSC::FromPersistentData<TrackedObject>(V82JSC::ToIsolate(iso), persistent);
        assert(!local.IsEmpty());
        TrackedObjectImpl *impl = V82JSC::ToImpl<TrackedObjectImpl>(local);

        if ((JSValueIsStrictEqual(ctx, object, impl->m_security) ||
             (impl->m_proxy_security && JSValueIsStrictEqual(ctx, object, impl->m_proxy_security)) ||
             (impl->m_hidden_proxy_security && JSValueIsStrictEqual(ctx, object, impl->m_hidden_proxy_security)) )) {
        
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
