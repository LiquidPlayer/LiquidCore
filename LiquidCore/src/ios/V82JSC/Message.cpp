/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "Message.h"
#include "Script.h"
#include "JSCPrivate.h"

using namespace V82JSC;
using v8::Local;
using v8::EscapableHandleScope;
using v8::HandleScope;
using v8::Maybe;
using v8::MaybeLocal;
using v8::Isolate;
using v8::TryCatch;
using v8::ScriptOrigin;

Message* Message::New(IsolateImpl* iso,
                      JSValueRef exception,
                      Local<v8::Script> script)
{
    HandleScope scope(ToIsolate(iso));
    
    Local<v8::Context> context = OperatingContext(ToIsolate(iso));
    auto msgi = static_cast<Message*>(HeapAllocator::Alloc(iso, iso->m_message_map));
    msgi->m_value = exception;
    JSValueProtect(ToContextRef(context), msgi->m_value);
    msgi->m_script.Reset(ToIsolate(iso), script);
    return msgi;
}

void Message::CallHandlers()
{
    IsolateImpl * iso = GetIsolate();
    HandleScope scope(ToIsolate(iso));

    Local<v8::Context> context = OperatingContext(ToIsolate(iso));
    Local<v8::Message> msg = CreateLocal<v8::Message>(&iso->ii, this);

    TryCatch try_catch(ToIsolate(iso));
    if (iso->m_message_listeners.empty()) {
        bool abort = false;
        if (iso->m_on_uncaught_exception_callback)
            abort = iso->m_on_uncaught_exception_callback(ToIsolate(iso));
        if (abort) {
            v8::String::Utf8Value str(msg->Get());
            FATAL(*str);
        }
    }
    for (auto i=iso->m_message_listeners.begin(); i!=iso->m_message_listeners.end(); ++i) {
        Local<v8::Value> data;
        if (i->data_) {
            data = Value::New(ToContextImpl(context), i->data_);
        } else {
            data = Value::New(ToContextImpl(context), m_value);
        }
        i->callback_(msg, data);
    }
}

Local<v8::String> v8::Message::Get() const
{
    Isolate *isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    Local<Context> context = OperatingContext(isolate);
    TryCatch try_catch(isolate);
    
    MaybeLocal<String> msg = reinterpret_cast<const v8::Value*>(this)->ToString(context);
    if (msg.IsEmpty()) {
        return scope.Escape(String::NewFromUtf8(isolate, "Uncaught exception", NewStringType::kNormal).ToLocalChecked());
    }
    return scope.Escape(String::Concat(String::NewFromUtf8(isolate, "Uncaught ",
                                              NewStringType::kNormal).ToLocalChecked(), msg.ToLocalChecked()));
}

MaybeLocal<v8::String> v8::Message::GetSourceLine(Local<Context> context) const
{
    // Not supported
    return String::NewFromUtf8(ToIsolate(this), "<Source line not available>", NewStringType::kNormal);
}

/**
 * Returns the origin for the script from where the function causing the
 * error originates.
 */
ScriptOrigin v8::Message::GetScriptOrigin() const
{
    // Do not create a handle scope here.  We are returning more than one handle.
    Isolate *isolate = ToIsolate(this);
    auto impl = ToImpl<V82JSC::Message>(this);
    Local<v8::Context> context = OperatingContext(isolate);
    
    if (impl->m_script.IsEmpty()) {
        return ScriptOrigin(GetScriptResourceName(),
                            Integer::New(isolate, GetLineNumber(context).ToChecked()),
                            Integer::New(isolate, GetStartColumn(context).ToChecked()));
    }

    Local<Script> script = impl->m_script.Get(isolate);
    assert(!script.IsEmpty());
    auto scr = ToImpl<V82JSC::Script>(script);
     
    Local<v8::Value> resource_name = GetScriptResourceName();
    if (resource_name.IsEmpty()) resource_name = Undefined(isolate);
     
    auto unbound = ToImpl<V82JSC::UnboundScript>(scr->m_unbound_script.Get(isolate));
     
    Local<Value> source_map = unbound->m_sourceMappingURL.Get(isolate);
    if (source_map.IsEmpty()) source_map = Undefined(isolate);
    
    return ScriptOrigin(resource_name,
                        Integer::New(isolate, GetLineNumber(context).ToChecked()),
                        Integer::New(isolate, GetStartColumn(context).ToChecked()),
                        Boolean::New(isolate, unbound->m_resource_is_shared_cross_origin),
                        unbound->m_id.Get(isolate),
                        source_map,
                        Boolean::New(isolate, unbound->m_resource_is_opaque),
                        Boolean::New(isolate, unbound->m_is_wasm),
                        Boolean::New(isolate, unbound->m_is_module));
}

/**
 * Returns the resource name for the script from where the function causing
 * the error originates.
 */
Local<v8::Value> v8::Message::GetScriptResourceName() const
{
    auto impl = ToImpl<V82JSC::Message>(this);
    auto iso = impl->GetIsolate();
    auto isolate = ToIsolate(iso);
    EscapableHandleScope scope(isolate);
    
    auto thread = IsolateImpl::PerThreadData::Get(iso);

    Local<v8::StackTrace> trace = V82JSC::StackTrace::New(iso, CreateLocal<Value>(&iso->ii,impl),
                                                  impl->m_script.Get(ToIsolate(iso)));
    if (trace->GetFrameCount()) {
        Local<String> name = trace->GetFrame(0)->GetScriptName();
        if (!name.IsEmpty() && name->Equals(OperatingContext(isolate),
                         String::NewFromUtf8(isolate, "undefined", NewStringType::kNormal).ToLocalChecked()).ToChecked()) {
            return scope.Escape(Undefined(isolate));
        }

        if (!name.IsEmpty()) return scope.Escape(name);
    }

    Local<Script> script;
    if (!thread->m_running_scripts.empty()) {
        script = thread->m_running_scripts.top();
    }
    if (!script.IsEmpty()) {
        auto scr = ToImpl<V82JSC::UnboundScript>(script->GetUnboundScript());
        return scope.Escape(scr->m_resource_name.Get(ToIsolate(iso)));
    }

    return Local<Value>();
}

/**
 * Exception stack trace. By default stack traces are not captured for
 * uncaught exceptions. SetCaptureStackTraceForUncaughtExceptions allows
 * to change this option.
 */
Local<v8::StackTrace> v8::Message::GetStackTrace() const
{
    auto message = ToImpl<V82JSC::Message>(this);
    auto iso = message->GetIsolate();
    EscapableHandleScope scope(ToIsolate(iso));

    if (iso->m_capture_stack_trace_for_uncaught_exceptions) {
        return scope.Escape(V82JSC::StackTrace::New(iso, CreateLocal<Value>(&iso->ii, message),
                                   message->m_script.Get(ToIsolate(iso))));
    } else {
        return Local<StackTrace>();
    }
}

/**
 * Returns the number, 1-based, of the line where the error occurred.
 */
Maybe<int> v8::Message::GetLineNumber(Local<Context> context) const
{
    auto impl = ToImpl<V82JSC::Message>(this);
    auto iso = impl->GetIsolate();
    HandleScope scope(ToIsolate(iso));
    
    Local<StackTrace> trace = V82JSC::StackTrace::New(iso, CreateLocal<Value>(&iso->ii, impl),
                                                  impl->m_script.Get(ToIsolate(iso)));
    if (trace->GetFrameCount()) {
        return _maybe<int>(trace->GetFrame(0)->GetLineNumber()).toMaybe();
    }
    return _maybe<int>(1).toMaybe();
}

/**
 * Returns the index within the script of the first character where
 * the error occurred.
 */
int v8::Message::GetStartPosition() const
{
    // Not supported
    NOT_IMPLEMENTED;
}

/**
 * Returns the index within the script of the last character where
 * the error occurred.
 */
int v8::Message::GetEndPosition() const
{
    // Not supported
    NOT_IMPLEMENTED;
}

/**
 * Returns the error level of the message.
 */
int v8::Message::ErrorLevel() const
{
    // We only support error messages
    return Isolate::MessageErrorLevel::kMessageError;
}

/**
 * Returns the index within the line of the first character where
 * the error occurred.
 */
Maybe<int> v8::Message::GetStartColumn(Local<v8::Context> context) const
{
    // Incompatibility alert: JSC doesn't return start and end coumns, only
    // the end column.
    auto impl = ToImpl<V82JSC::Message>(this);
    auto iso = impl->GetIsolate();
    HandleScope scope(ToIsolate(iso));
    
    Local<StackTrace> trace = V82JSC::StackTrace::New(iso, CreateLocal<Value>(&iso->ii, impl),
                                                  impl->m_script.Get(ToIsolate(iso)));
    if (trace->GetFrameCount()) {
        return _maybe<int>(trace->GetFrame(0)->GetColumn()).toMaybe();
    }
    return _maybe<int>(0).toMaybe();
}

/**
 * Returns the index within the line of the last character where
 * the error occurred.
 */
Maybe<int> v8::Message::GetEndColumn(Local<v8::Context> context) const
{
    auto impl = ToImpl<V82JSC::Message>(this);
    auto iso = impl->GetIsolate();
    HandleScope scope(ToIsolate(iso));
    
    Local<StackTrace> trace = V82JSC::StackTrace::New(iso, CreateLocal<Value>(&iso->ii, impl),
                                                  impl->m_script.Get(ToIsolate(iso)));
    if (trace->GetFrameCount()) {
        return _maybe<int>(trace->GetFrame(0)->GetColumn()).toMaybe();
    }
    return Nothing<int>();
}

/**
 * Passes on the value set by the embedder when it fed the script from which
 * this Message was generated to V8.
 */
bool v8::Message::IsSharedCrossOrigin() const
{
    return GetScriptOrigin().Options().IsSharedCrossOrigin();
}

bool v8::Message::IsOpaque() const
{
    return GetScriptOrigin().Options().IsOpaque();
}

void v8::Message::PrintCurrentStackTrace(Isolate* isolate, FILE* out)
{
    HandleScope scope(isolate);
    Local<v8::Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    JSStringRef trace = JSCPrivate::JSContextCreateBacktrace(ctx, 64);
    char buffer[JSStringGetMaximumUTF8CStringSize(trace)];
    JSStringGetUTF8CString(trace, buffer, JSStringGetMaximumUTF8CStringSize(trace));
    fprintf(out, "%s", buffer);
    JSStringRelease(trace);
}

/**
 * Returns the number, 1-based, of the line for the associate function call.
 * This method will return Message::kNoLineNumberInfo if it is unable to
 * retrieve the line number, or if kLineNumber was not passed as an option
 * when capturing the StackTrace.
 */
int v8::StackFrame::GetLineNumber() const
{
    auto impl = ToImpl<V82JSC::StackFrame>(this);
    return impl->m_line_number;
}

/**
 * Returns the 1-based column offset on the line for the associated function
 * call.
 * This method will return Message::kNoColumnInfo if it is unable to retrieve
 * the column number, or if kColumnOffset was not passed as an option when
 * capturing the StackTrace.
 */
int v8::StackFrame::GetColumn() const
{
    auto impl = ToImpl<V82JSC::StackFrame>(this);
    return impl->m_column_number;
}

/**
 * Returns the id of the script for the function for this StackFrame.
 * This method will return Message::kNoScriptIdInfo if it is unable to
 * retrieve the script id, or if kScriptId was not passed as an option when
 * capturing the StackTrace.
 */
int v8::StackFrame::GetScriptId() const
{
    auto impl = ToImpl<V82JSC::StackFrame>(this);
    Isolate *isolate = ToIsolate(impl->GetIsolate());
    HandleScope scope(isolate);
    auto trace = ToImpl<V82JSC::StackTrace>(impl->m_stack_trace.Get(isolate));
    auto scr = ToImpl<V82JSC::UnboundScript>(trace->m_script.Get(isolate)->GetUnboundScript());
    Local<Integer> id = scr->m_id.Get(isolate);
    if (id.IsEmpty()) {
        return Message::kNoScriptIdInfo;
    }
    return (int) id->Value();
}

/**
 * Returns the name of the resource that contains the script for the
 * function for this StackFrame.
 */
Local<v8::String> v8::StackFrame::GetScriptName() const
{
    auto impl = ToImpl<V82JSC::StackFrame>(this);
    Isolate *isolate = ToIsolate(impl->GetIsolate());
    EscapableHandleScope scope(isolate);
    Local<String> name = impl->m_script_name.Get(isolate);
    return scope.Escape(name);
}

/**
 * Returns the name of the resource that contains the script for the
 * function for this StackFrame or sourceURL value if the script name
 * is undefined and its source ends with //# sourceURL=... string or
 * deprecated //@ sourceURL=... string.
 */
Local<v8::String> v8::StackFrame::GetScriptNameOrSourceURL() const
{
    return GetScriptName();
}

/**
 * Returns the name of the function associated with this stack frame.
 */
Local<v8::String> v8::StackFrame::GetFunctionName() const
{
    auto impl = ToImpl<V82JSC::StackFrame>(this);
    EscapableHandleScope scope(ToIsolate(impl->GetIsolate()));
    return scope.Escape(impl->m_function_name.Get(ToIsolate(impl->GetIsolate())));
}

/**
 * Returns whether or not the associated function is compiled via a call to
 * eval().
 */
bool v8::StackFrame::IsEval() const
{
    auto impl = ToImpl<V82JSC::StackFrame>(this);
    return impl->m_is_eval;
}

/**
 * Returns whether or not the associated function is called as a
 * constructor via "new".
 */
bool v8::StackFrame::IsConstructor() const
{
    // Not supported
    return false;
}

/**
 * Returns whether or not the associated functions is defined in wasm.
 */
bool v8::StackFrame::IsWasm() const
{
    // Not supported
    return false;
}

v8::Local<v8::StackTrace> StackTrace::New(IsolateImpl* iso, Local<v8::Value> value,
                                          Local<v8::Script> script)
{
    EscapableHandleScope scope(ToIsolate(iso));
    JSContextRef ctx = ToContextRef(ToIsolate(iso));
    Local<v8::Context> context = OperatingContext(ToIsolate(iso));
    JSValueRef v = ToJSValueRef(value, context);

    const char *parse_error_frames =
    "var frames = _1.split('\\n'); "
    "var frame_array = []; "
    "var skipnext = false; "
    "for (var i=0; frames != '' && i<frames.length; i++) { "
    "    const re_loc = /:*([0-9]*):*([0-9]*)$/ig; "
    "    const re_names = /([^@]*)@*(.*)/ig; "
    "    var loc = re_loc.exec(frames[i]) || ['']; "
    "    var names_string = loc.length > 1 ? frames[i].substring(0,loc.index) : frames[i]; "
    "    var names = re_names.exec(names_string); "
    "    var array = names.slice(1).concat(loc.slice(1)); "
    "    if (array[0] == '') array[0] = ''; "
    "    if (array[1] == '') array[1] = ''; "
    "    if (array[2] == '') array[2] = 1; else array[2] = parseInt(array[2]); "
    "    if (array[3] == '') array[3] = 0; else array[3] = parseInt(array[3]); "
    "    if (/*array[0] != 'Function' &&*/!skipnext && array[0] != '[native code]' && array[1] != '[native code]' && !(array[0] == 'global code' && array[1] == '')) "
    "        frame_array.push(array); "
    "    skipnext = array[0] == '[native code]';"
    "} "
    "return frame_array;";
    
    auto stack_trace = static_cast<StackTrace*>(
               HeapAllocator::Alloc(iso, iso->m_stack_trace_map));
    stack_trace->m_error = (JSObjectRef) v;
    JSValueProtect(ctx, stack_trace->m_error);
    stack_trace->m_script.Reset(ToIsolate(iso), script);
    Local<v8::StackTrace> local = CreateLocal<v8::StackTrace>(&iso->ii, stack_trace);

    JSValueRef stack;
    if (v && value->IsNativeError()) {
        JSStringRef stack_ = JSStringCreateWithUTF8CString("stack");
        stack = JSObjectGetProperty(ctx, stack_trace->m_error, stack_, 0);
        JSStringRelease(stack_);
    } else {
        JSStringRef s = JSStringCreateWithUTF8CString("");
        stack = JSValueMakeString(ctx, s);
        JSStringRelease(s);
    }
    
    stack_trace->m_stack_frame_array = (JSObjectRef) exec(ctx, parse_error_frames, 1, &stack);
    JSValueProtect(ctx, stack_trace->m_stack_frame_array);

    return scope.Escape(local);
}

Local<v8::StackFrame> v8::StackTrace::GetFrame(Isolate* isolate, uint32_t index) const
{
    auto impl = ToImpl<V82JSC::StackTrace>(this);
    IsolateImpl *iso = ToIsolateImpl(isolate);
    EscapableHandleScope scope(ToIsolate(iso));
    
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    
    if (impl->m_stack_frame_array) {
        Local<Array> stack_frames = V82JSC::Value::New(ToContextImpl(context), impl->m_stack_frame_array).As<Array>();
        if (index < stack_frames->Length()) {
            Local<Value> frame = stack_frames->Get(context, index).ToLocalChecked();
            JSObjectRef array = (JSObjectRef) ToJSValueRef(frame, context);
            Local<String> function_name = V82JSC::Value::New(ToContextImpl(context),
                                                         JSObjectGetPropertyAtIndex(ctx, array, 0, 0)).As<String>();
            Local<String> script_name = V82JSC::Value::New(ToContextImpl(context),
                                                       JSObjectGetPropertyAtIndex(ctx, array, 1, 0)).As<String>();
            if (script_name->Equals(context, String::NewFromUtf8(ToIsolate(iso),
                                                        "[undefined]", NewStringType::kNormal).ToLocalChecked()).FromJust() ||
                script_name->Equals(context, String::NewFromUtf8(ToIsolate(iso),
                                                        "[eval]", NewStringType::kNormal).ToLocalChecked()).FromJust()) {
                script_name = Local<String>();
            }
            int line_number = static_cast<int>(JSValueToNumber(ctx, JSObjectGetPropertyAtIndex(ctx, array, 2, 0), 0));
            int col_number = static_cast<int>(JSValueToNumber(ctx, JSObjectGetPropertyAtIndex(ctx, array, 3, 0), 0));
            bool is_eval = JSValueToBoolean(ctx, JSObjectGetPropertyAtIndex(ctx, array, 4, 0));
            
            auto stack_frame = static_cast<V82JSC::StackFrame*>(HeapAllocator::Alloc(iso, iso->m_stack_frame_map));
            stack_frame->m_function_name.Reset(ToIsolate(iso), function_name);
            stack_frame->m_script_name.Reset(ToIsolate(iso), script_name);
            stack_frame->m_line_number = line_number;
            stack_frame->m_column_number = col_number;
            stack_frame->m_is_eval = is_eval;
            stack_frame->m_stack_trace.Reset(isolate, CreateLocal<StackTrace>(&iso->ii, impl));
            
            return scope.Escape(CreateLocal<v8::StackFrame>(&iso->ii, stack_frame));
        }
    }

    return Local<v8::StackFrame>();
}

/**
 * Returns the number of StackFrames.
 */
int v8::StackTrace::GetFrameCount() const
{
    auto impl = ToImpl<V82JSC::StackTrace>(this);
    HandleScope scope(ToIsolate(impl->GetIsolate()));
    Local<Context> context = OperatingContext(ToIsolate(impl->GetIsolate()));

    if (impl->m_stack_frame_array) {
        Local<Array> stack_frames = V82JSC::Value::New(ToContextImpl(context), impl->m_stack_frame_array).As<Array>();
        return stack_frames->Length();
    }
    
    return 0;
}

/**
 * Grab a snapshot of the current JavaScript execution stack.
 *
 * \param frame_limit The maximum number of stack frames we want to capture.
 * \param options Enumerates the set of things we will capture for each
 *   StackFrame.
 */
Local<v8::StackTrace> v8::StackTrace::CurrentStackTrace(Isolate* isolate, int frame_limit,
                                                       StackTraceOptions options)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    Local<Script> script;
    auto thread = IsolateImpl::PerThreadData::Get(ToIsolateImpl(isolate));

    if (!thread->m_running_scripts.empty()) {
        script = thread->m_running_scripts.top();
    }

    JSStringRef e = JSStringCreateWithUTF8CString("new Error()");
    JSValueRef error = JSEvaluateScript(ToContextRef(context), e, 0, 0, 0, 0);
    Local<Value> err = V82JSC::Value::New(ToContextImpl(context), error);
    return scope.Escape(V82JSC::StackTrace::New(ToIsolateImpl(isolate), err, script));
}
