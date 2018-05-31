//
//  Isolate.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 5/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Isolate_h
#define Isolate_h

#include "HeapObjects.h"

namespace v8 {
namespace internal {
    
using v8::InterruptCallback;
using v8::ExtensionCallback;
using v8::LogEventCallback;
using v8::AllowCodeGenerationFromStringsCallback;
using v8::ApiImplementationCallback;
using v8::PromiseRejectCallback;
using v8::FatalErrorCallback;
using v8::OOMErrorCallback;
using v8::BeforeCallEnteredCallback;
using v8::CallCompletedCallback;
using v8::MicrotasksCompletedCallback;
using v8::internal::Isolate;
    
#define kJSRegexpStaticOffsetsVectorSize v8::internal::Isolate::kJSRegexpStaticOffsetsVectorSize
#define kBMMaxShift v8::internal::Isolate::kBMMaxShift
#define kUC16AlphabetSize v8::internal::Isolate::kUC16AlphabetSize

#define H V82JSC_HeapObject
    
struct IsolateImpl {
    v8::internal::Isolate ii;
    
    JSContextGroupRef m_group;
    Copyable(v8::Context) m_nullContext;
    JSValueRef m_negative_zero;
    JSValueRef m_empty_string;
    JSValueRef m_private_symbol;
    
    // Maps
    H::Map<H::TrackedObject> *m_tracked_object_map;
    H::Map<H::Value> *m_array_buffer_map;
    H::Map<H::Context> *m_context_map;
    H::Map<H::FixedArray> *m_fixed_array_map;
    H::Map<H::String> *m_one_byte_string_map;
    H::Map<H::String> *m_string_map;
    H::Map<H::String> *m_external_string_map;
    H::Map<H::String> *m_external_one_byte_string_map;
    H::Map<H::String> *m_internalized_string_map;
    H::Map<H::Value> *m_value_map;
    H::Map<H::Value> *m_number_map;
    H::Map<H::Signature> *m_signature_map;
    H::Map<H::FunctionTemplate> *m_function_template_map;
    H::Map<H::ObjectTemplate> *m_object_template_map;
    H::Map<H::Prop> *m_property_map;
    H::Map<H::PropAccessor> *m_property_accessor_map;
    H::Map<H::IntrinsicProp> *m_intrinsic_property_map;
    H::Map<H::Accessor> *m_accessor_map;
    H::Map<H::ObjAccessor> *m_object_accessor_map;
    H::Map<H::Script> *m_script_map;

    v8::TryCatch *m_handlers;
    
    std::map<std::string, JSValueRef> m_global_symbols;
    std::map<std::string, JSValueRef> m_private_symbols;
    
    v8::Isolate::CreateParams m_params;
    
    std::stack<Copyable(v8::Context)> m_context_stack;
    std::stack<v8::HandleScope*> m_scope_stack;
    
    std::map<JSGlobalContextRef, Copyable(v8::Context)> m_global_contexts;
    static std::map<JSGlobalContextRef, IsolateImpl*> s_context_to_isolate_map;
    
    std::map<JSGlobalContextRef, std::map<const char *, JSObjectRef>> m_exec_maps;
    
    void EnterContext(v8::Local<v8::Context> ctx);
    void ExitContext(v8::Local<v8::Context> ctx);
    void GetActiveLocalHandles(std::map<v8::internal::Object*, bool>& dontDeleteMap);
};
}} // namespaces

#undef H

#endif /* Isolate_h */
