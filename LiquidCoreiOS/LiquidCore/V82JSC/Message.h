/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_Message_h
#define V82JSC_Message_h

#include "Value.h"

namespace V82JSC {

struct Message : Value {
    v8::Persistent<v8::Script> m_script;
    
    static void Constructor(Message *obj) {}
    static int Destructor(HeapContext& context, Message *obj)
    {
        int freed=0;
        freed += SmartReset<v8::Script>(context, obj->m_script);
        return freed + Value::Destructor(context, obj);
    }
    
    static Message* New(IsolateImpl* iso, JSValueRef exception,
                        v8::Local<v8::Script> script);
    void CallHandlers();
};
    
struct StackTrace : HeapObject {
    v8::Persistent<v8::Script> m_script;
    JSObjectRef m_error;
    JSObjectRef m_stack_frame_array;
    
    static void Constructor(StackTrace *obj) {}
    static int Destructor(HeapContext& context, StackTrace *obj)
    {
        if (obj->m_error) JSValueUnprotect(obj->GetNullContext(), obj->m_error);
        if (obj->m_stack_frame_array) JSValueUnprotect(obj->GetNullContext(), obj->m_stack_frame_array);
        int freed=0;
        freed += SmartReset<v8::Script>(context, obj->m_script);
        return freed;
    }
    
    static v8::Local<v8::StackTrace> New(IsolateImpl* iso,
                                         v8::Local<v8::Value> error,
                                         v8::Local<v8::Script> script);
};

struct StackFrame : HeapObject {
    v8::Persistent<v8::String> m_function_name;
    v8::Persistent<v8::String> m_script_name;
    v8::Persistent<v8::StackTrace> m_stack_trace;
    int m_line_number;
    int m_column_number;
    bool m_is_eval;
    
    static void Constructor(StackFrame *obj) {}
    static int Destructor(HeapContext& context, StackFrame *obj)
    {
        int freed=0;
        freed += SmartReset<v8::String>(context, obj->m_function_name);
        freed += SmartReset<v8::String>(context, obj->m_script_name);
        freed += SmartReset<v8::StackTrace>(context, obj->m_stack_trace);
        return freed;
    }
};

} /* namespace V82JSC */

#endif /* V82JSC_Message_h */
