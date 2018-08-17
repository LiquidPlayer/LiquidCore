//
//  V82JSC.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "V82JSC.h"

using namespace v8;

JSValueRef V82JSC::GetRealPrototype(v8::Local<v8::Context> context, JSObjectRef obj)
{
    v8::Isolate* isolate = ToIsolate(V82JSC::ToContextImpl(context));
    HandleScope scope(isolate);
    
    v8::Local<v8::Context> global_context = FindGlobalContext(context);
    if (global_context.IsEmpty()) {
        // No worries, it just means this hasn't been set up yet; use the native API
        return JSObjectGetPrototype(ToContextRef(context), obj);
    }
    JSContextRef ctx = ToContextRef(context);
    global_context = ToIsolateImpl(isolate)->m_global_contexts[JSObjectGetGlobalContext(obj)].Get(isolate);
    v8::Local<v8::Function> getPrototype = ToGlobalContextImpl(global_context)->ObjectGetPrototypeOf.Get(isolate);
    if (getPrototype.IsEmpty()) {
        // No worries, it just means this hasn't been set up yet; use the native API
        return JSObjectGetPrototype(ToContextRef(context), obj);
    }
    JSValueRef exception=0;
    JSValueRef our_proto = JSObjectCallAsFunction(ctx, (JSObjectRef)ToJSValueRef(getPrototype, global_context),
                                                  0, 1, &obj, &exception);
    assert(exception==0);
    return our_proto;
}

void V82JSC::SetRealPrototype(v8::Local<v8::Context> context, JSObjectRef obj, JSValueRef proto,
                              bool override_immutable)
{
    v8::Isolate* isolate = ToIsolate(V82JSC::ToContextImpl(context));
    HandleScope scope(isolate);

    v8::Local<v8::Context> global_context = FindGlobalContext(context);    
    TrackedObjectImpl *impl = getPrivateInstance(V82JSC::ToContextRef(context), obj);
    if (!override_immutable && impl && !impl->m_object_template.IsEmpty() &&
        V82JSC::ToImpl<ObjectTemplateImpl>(impl->m_object_template.Get(isolate))->m_is_immutable_proto) {
        
        isolate->ThrowException
        (v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "prototype is immutable",
                                                          v8::NewStringType::kNormal).ToLocalChecked()));
        return;
    }
    
    if (global_context.IsEmpty()) {
        // No worries, it just means this hasn't been set up yet; use the native API
        JSObjectSetPrototype(ToContextRef(context), obj, proto);
        return;
    }
    v8::Local<v8::Function> setPrototype = ToGlobalContextImpl(global_context)->ObjectSetPrototypeOf.Get(isolate);
    CHECK(!setPrototype.IsEmpty());
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        obj,
        proto
    };
    LocalException exception(ToIsolateImpl(isolate));
    JSObjectCallAsFunction(ctx, (JSObjectRef)ToJSValueRef(setPrototype, global_context), 0, 2, args, &exception);
}

LocalException::~LocalException()
{
    v8::HandleScope scope(V82JSC::ToIsolate(isolate_));
    v8::Local<v8::Script> script;
    auto thread = IsolateImpl::PerThreadData::Get(isolate_);
    
    if (!thread->m_running_scripts.empty()) {
        script = thread->m_running_scripts.top();
    }
    
    v8::Local<v8::Context> context = V82JSC::OperatingContext(V82JSC::ToIsolate(isolate_));
    JSContextRef ctx = V82JSC::ToContextRef(context);
    if (exception_) {
        MessageImpl * msgi = MessageImpl::New(isolate_, (JSValueRef)exception_, script,
                                              JSContextCreateBacktrace(ctx, 32));
        if (thread->m_handlers) {
            TryCatchCopy *tcc = reinterpret_cast<TryCatchCopy*>(thread->m_handlers);
            tcc->exception_ = (void*)exception_;
            tcc->message_obj_ = (void*)msgi;
            
            thread->m_scheduled_exception =
                isolate_->ii.heap()->root(v8::internal::Heap::RootListIndex::kTheHoleValueRootIndex);
            if (tcc->is_verbose_ && !tcc->next_) {
                msgi->CallHandlers();
            }
        } else {
            msgi->CallHandlers();
        }
    } else if (thread->m_verbose_exception && !thread->m_handlers) {
        MessageImpl * msgi = MessageImpl::New(isolate_, (JSValueRef)exception_, script,
                                              JSContextCreateBacktrace(ctx, 32));
        msgi->CallHandlers();
        thread->m_verbose_exception = 0;
    }
}
