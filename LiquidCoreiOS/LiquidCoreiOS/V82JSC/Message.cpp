//
//  Message.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 6/5/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include "JSContextRefPrivate.h"

using namespace v8;
#define H V82JSC_HeapObject

MessageImpl* MessageImpl::New(IsolateImpl* iso, JSValueRef exception, Local<Script> script, JSStringRef back_trace)
{
    HandleScope scope(V82JSC::ToIsolate(iso));
    
    Local<Context> context = V82JSC::OperatingContext(V82JSC::ToIsolate(iso));
    MessageImpl * msgi = static_cast<MessageImpl*>(V82JSC_HeapObject::HeapAllocator::Alloc(iso, iso->m_message_map));
    msgi->m_value = exception;
    JSValueProtect(V82JSC::ToContextRef(context), msgi->m_value);
    msgi->m_back_trace = back_trace;
    msgi->m_script.Reset(V82JSC::ToIsolate(iso), script);
    return msgi;
}

void MessageImpl::CallHandlers()
{
    IsolateImpl * iso = GetIsolate();
    HandleScope scope(V82JSC::ToIsolate(iso));
    
    Local<Context> context = V82JSC::OperatingContext(V82JSC::ToIsolate(iso));
    Local<v8::Message> msg = V82JSC::CreateLocal<v8::Message>(&iso->ii, this);

    TryCatch try_catch(V82JSC::ToIsolate(iso));
    if (iso->m_message_listeners.empty()) {
        bool abort = false;
        if (iso->m_on_uncaught_exception_callback)
            abort = iso->m_on_uncaught_exception_callback(V82JSC::ToIsolate(iso));
        if (abort) {
            String::Utf8Value str(msg->Get());
            FATAL(*str);
        }
    }
    for (auto i=iso->m_message_listeners.begin(); i!=iso->m_message_listeners.end(); ++i) {
        Local<v8::Value> data;
        if (i->data_) {
            data = ValueImpl::New(V82JSC::ToContextImpl(context), i->data_);
        } else {
            data = ValueImpl::New(V82JSC::ToContextImpl(context), m_value);
        }
        i->callback_(msg, data);
    }
}

Local<String> Message::Get() const
{
    Isolate *isolate = V82JSC::ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    Local<Context> context = V82JSC::OperatingContext(isolate);
    TryCatch try_catch(isolate);
    
    MaybeLocal<String> msg = reinterpret_cast<const v8::Value*>(this)->ToString(context);
    if (msg.IsEmpty()) {
        return scope.Escape(String::NewFromUtf8(isolate, "Uncaught exception", NewStringType::kNormal).ToLocalChecked());
    }
    return scope.Escape(String::Concat(String::NewFromUtf8(isolate, "Uncaught ",
                                              NewStringType::kNormal).ToLocalChecked(), msg.ToLocalChecked()));
}

MaybeLocal<String> Message::GetSourceLine(Local<Context> context) const
{
    // Not supported
    return MaybeLocal<String>();
}

/**
 * Returns the origin for the script from where the function causing the
 * error originates.
 */
ScriptOrigin Message::GetScriptOrigin() const
{
    // Do not create a handle scope here.  We are returning more than one handle.
    Isolate *isolate = V82JSC::ToIsolate(this);
    MessageImpl *impl = V82JSC::ToImpl<MessageImpl>(this);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    
    if (impl->m_script.IsEmpty()) {
        return ScriptOrigin(GetScriptResourceName(),
                            Integer::New(isolate, GetLineNumber(context).ToChecked()),
                            Integer::New(isolate, GetStartColumn(context).ToChecked()));
    }

    Local<Script> script = impl->m_script.Get(isolate);
    assert(!script.IsEmpty());
    ScriptImpl *scr = V82JSC::ToImpl<ScriptImpl>(script);
     
    Local<Value> resource_name = GetScriptResourceName();
    if (resource_name.IsEmpty()) resource_name = Undefined(isolate);
     
    UnboundScriptImpl *unbound = V82JSC::ToImpl<UnboundScriptImpl>(scr->m_unbound_script.Get(isolate));
     
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
Local<Value> Message::GetScriptResourceName() const
{
    MessageImpl *impl = V82JSC::ToImpl<MessageImpl>(this);
    IsolateImpl *iso = impl->GetIsolate();
    Isolate* isolate = V82JSC::ToIsolate(iso);
    EscapableHandleScope scope(isolate);
    
    auto thread = IsolateImpl::PerThreadData::Get(iso);

    Local<StackTrace> trace = StackTraceImpl::New(iso, V82JSC::CreateLocal<Value>(&iso->ii, impl),
                                                  impl->m_script.Get(V82JSC::ToIsolate(iso)), impl->m_back_trace);
    if (trace->GetFrameCount()) {
        Local<String> name = trace->GetFrame(0)->GetScriptName();
        if (!name.IsEmpty() && name->Equals(V82JSC::OperatingContext(isolate),
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
        UnboundScriptImpl* scr = V82JSC::ToImpl<UnboundScriptImpl>(script->GetUnboundScript());
        return scope.Escape(scr->m_resource_name.Get(V82JSC::ToIsolate(iso)));
    }

    return Local<Value>();
}

/**
 * Exception stack trace. By default stack traces are not captured for
 * uncaught exceptions. SetCaptureStackTraceForUncaughtExceptions allows
 * to change this option.
 */
Local<StackTrace> Message::GetStackTrace() const
{
    MessageImpl *message = V82JSC::ToImpl<MessageImpl>(this);
    IsolateImpl *iso = message->GetIsolate();
    EscapableHandleScope scope(V82JSC::ToIsolate(iso));

    if (iso->m_capture_stack_trace_for_uncaught_exceptions) {
        return scope.Escape(StackTraceImpl::New(iso, V82JSC::CreateLocal<Value>(&iso->ii, message),
                                   message->m_script.Get(V82JSC::ToIsolate(iso)), message->m_back_trace));
    } else {
        return Local<StackTrace>();
    }
}

/**
 * Returns the number, 1-based, of the line where the error occurred.
 */
Maybe<int> Message::GetLineNumber(Local<Context> context) const
{
    MessageImpl *impl = V82JSC::ToImpl<MessageImpl>(this);
    IsolateImpl *iso = impl->GetIsolate();
    HandleScope scope(V82JSC::ToIsolate(iso));
    
    Local<StackTrace> trace = StackTraceImpl::New(iso, V82JSC::CreateLocal<Value>(&iso->ii, impl),
                                                  impl->m_script.Get(V82JSC::ToIsolate(iso)), impl->m_back_trace);
    if (trace->GetFrameCount()) {
        return _maybe<int>(trace->GetFrame(0)->GetLineNumber()).toMaybe();
    }
    return _maybe<int>(1).toMaybe();
}

/**
 * Returns the index within the script of the first character where
 * the error occurred.
 */
int Message::GetStartPosition() const
{
    // Not supported
    return 0;
}

/**
 * Returns the index within the script of the last character where
 * the error occurred.
 */
int Message::GetEndPosition() const
{
    // Not supported
    return 0;
}

/**
 * Returns the error level of the message.
 */
int Message::ErrorLevel() const
{
    // We only support error messages
    return Isolate::MessageErrorLevel::kMessageError;
}

/**
 * Returns the index within the line of the first character where
 * the error occurred.
 */
Maybe<int> Message::GetStartColumn(Local<Context> context) const
{
    // Incompatibility alert: JSC doesn't return start and end coumns, only
    // the end column.
    MessageImpl *impl = V82JSC::ToImpl<MessageImpl>(this);
    IsolateImpl *iso = impl->GetIsolate();
    HandleScope scope(V82JSC::ToIsolate(iso));
    
    Local<StackTrace> trace = StackTraceImpl::New(iso, V82JSC::CreateLocal<Value>(&iso->ii, impl),
                                                  impl->m_script.Get(V82JSC::ToIsolate(iso)), impl->m_back_trace);
    if (trace->GetFrameCount()) {
        return _maybe<int>(trace->GetFrame(0)->GetColumn()).toMaybe();
    }
    return _maybe<int>(0).toMaybe();
}

/**
 * Returns the index within the line of the last character where
 * the error occurred.
 */
Maybe<int> Message::GetEndColumn(Local<Context> context) const
{
    MessageImpl *impl = V82JSC::ToImpl<MessageImpl>(this);
    IsolateImpl *iso = impl->GetIsolate();
    HandleScope scope(V82JSC::ToIsolate(iso));
    
    Local<StackTrace> trace = StackTraceImpl::New(iso, V82JSC::CreateLocal<Value>(&iso->ii, impl),
                                                  impl->m_script.Get(V82JSC::ToIsolate(iso)), impl->m_back_trace);
    if (trace->GetFrameCount()) {
        return _maybe<int>(trace->GetFrame(0)->GetColumn()).toMaybe();
    }
    return Nothing<int>();
}

/**
 * Passes on the value set by the embedder when it fed the script from which
 * this Message was generated to V8.
 */
bool Message::IsSharedCrossOrigin() const
{
    return GetScriptOrigin().Options().IsSharedCrossOrigin();
}

bool Message::IsOpaque() const
{
    return GetScriptOrigin().Options().IsOpaque();
}

void Message::PrintCurrentStackTrace(Isolate* isolate, FILE* out)
{
    HandleScope scope(isolate);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSStringRef trace = JSContextCreateBacktrace(ctx, 64);
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
int StackFrame::GetLineNumber() const
{
    StackFrameImpl *impl = V82JSC::ToImpl<StackFrameImpl>(this);
    return impl->m_line_number;
}

/**
 * Returns the 1-based column offset on the line for the associated function
 * call.
 * This method will return Message::kNoColumnInfo if it is unable to retrieve
 * the column number, or if kColumnOffset was not passed as an option when
 * capturing the StackTrace.
 */
int StackFrame::GetColumn() const
{
    StackFrameImpl *impl = V82JSC::ToImpl<StackFrameImpl>(this);
    return impl->m_column_number;
}

/**
 * Returns the id of the script for the function for this StackFrame.
 * This method will return Message::kNoScriptIdInfo if it is unable to
 * retrieve the script id, or if kScriptId was not passed as an option when
 * capturing the StackTrace.
 */
int StackFrame::GetScriptId() const
{
    StackFrameImpl *impl = V82JSC::ToImpl<StackFrameImpl>(this);
    Isolate *isolate = V82JSC::ToIsolate(impl->GetIsolate());
    HandleScope scope(isolate);
    StackTraceImpl *trace = V82JSC::ToImpl<StackTraceImpl>(impl->m_stack_trace.Get(isolate));
    UnboundScriptImpl* scr = V82JSC::ToImpl<UnboundScriptImpl>(trace->m_script.Get(isolate)->GetUnboundScript());
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
Local<String> StackFrame::GetScriptName() const
{
    StackFrameImpl *impl = V82JSC::ToImpl<StackFrameImpl>(this);
    Isolate *isolate = V82JSC::ToIsolate(impl->GetIsolate());
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
Local<String> StackFrame::GetScriptNameOrSourceURL() const
{
    return GetScriptName();
}

/**
 * Returns the name of the function associated with this stack frame.
 */
Local<String> StackFrame::GetFunctionName() const
{
    StackFrameImpl *impl = V82JSC::ToImpl<StackFrameImpl>(this);
    EscapableHandleScope scope(V82JSC::ToIsolate(impl->GetIsolate()));
    return scope.Escape(impl->m_function_name.Get(V82JSC::ToIsolate(impl->GetIsolate())));
}

/**
 * Returns whether or not the associated function is compiled via a call to
 * eval().
 */
bool StackFrame::IsEval() const
{
    StackFrameImpl *impl = V82JSC::ToImpl<StackFrameImpl>(this);
    return impl->m_is_eval;
}

/**
 * Returns whether or not the associated function is called as a
 * constructor via "new".
 */
bool StackFrame::IsConstructor() const
{
    // Not supported
    return false;
}

/**
 * Returns whether or not the associated functions is defined in wasm.
 */
bool StackFrame::IsWasm() const
{
    // Not supported
    return false;
}

v8::Local<v8::StackTrace> StackTraceImpl::New(IsolateImpl* iso, Local<Value> value,
                                              Local<v8::Script> script, JSStringRef back_trace)
{
    EscapableHandleScope scope(V82JSC::ToIsolate(iso));
    JSContextRef ctx = V82JSC::ToContextRef(V82JSC::ToIsolate(iso));
    Local<Context> context = V82JSC::OperatingContext(V82JSC::ToIsolate(iso));
    JSValueRef v = V82JSC::ToJSValueRef(value, context);

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
    "var frames = _2.split('\\n'); "
    /*
    "var back_frame_array = []; "
    "for (var i=0; frames != '' && i<frames.length; i++) { "
    "    var re = /#[0-9]* ([^\\(]*)\\(\\) at ([^:\\n]*):*([0-9]*)/g; "
    "    var match = re.exec(frames[i]); "
    "    if (match != null) { "
    "        var array = [ match[1], match[2], match[3]=='' ? 1 : parseInt(match[3]), 0]; "
    "        if (array[1] != '[native code]' && array[0] != 'Function') { "
    "            back_frame_array.push(array); "
    "        }"
    "    } "
    "} "
    "if (frame_array.length == back_frame_array.length) {"
    "   for (var i=0; i<frame_array.length; i++) { if (frame_array[i][2] == 1) frame_array[i][2] = back_frame_array[i][2]; }"
    "}"
     */
    "for (var i=frame_array.length; i>0; --i) { "
    "    if (i < frame_array.length && frame_array[i-1][0] != 'eval code' && frame_array[i-1][1] == '') "
    "        frame_array[i-1][1] = frame_array[i][1]; "
    "    if (frame_array[i-1][0] == 'eval code') {"
    "        frame_array[i-1][4] = true; "
    "        if (frame_array[i-1][1] == '') frame_array[i-1][1] = '[eval]'; "
    "    } "
    "    else if (i==frame_array.length) frame_array[i-1][4] = false;"
    "    else if (frame_array[i-1][1] == frame_array[i][1]) frame_array[i-1][4] = frame_array[i][4]; "
    "    else frame_array[i-1][4] = false; "
    "} "
    "return frame_array;";
    
    StackTraceImpl *stack_trace = static_cast<StackTraceImpl*>(H::HeapAllocator::Alloc(iso, iso->m_stack_trace_map));
    stack_trace->m_error = (JSObjectRef) v;
    JSValueProtect(ctx, stack_trace->m_error);
    stack_trace->m_script.Reset(V82JSC::ToIsolate(iso), script);
    Local<v8::StackTrace> local = V82JSC::CreateLocal<v8::StackTrace>(&iso->ii, stack_trace);

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
    
    JSValueRef back_stack_trace;
    if (back_trace) {
         back_stack_trace = JSValueMakeString(ctx, back_trace);
    } else {
        JSStringRef s = JSStringCreateWithUTF8CString("");
        back_stack_trace = JSValueMakeString(ctx, s);
        JSStringRelease(s);
    }
    
    JSValueRef args[] = { stack, back_stack_trace };
    stack_trace->m_stack_frame_array = (JSObjectRef) V82JSC::exec(ctx, parse_error_frames, 2, args);
    JSValueProtect(ctx, stack_trace->m_stack_frame_array);

    /*
    JSValueRef s = V82JSC::exec(ctx, "return JSON.stringify(_1)", 1, &stack_trace->m_stack_frame_array);
    JSStringRef ss = JSValueToStringCopy(ctx, s, 0);
    char foo[JSStringGetMaximumUTF8CStringSize(ss)];
    JSStringGetUTF8CString(ss, foo, JSStringGetMaximumUTF8CStringSize(ss));
    printf ("Captured stack = %s\n", foo);

    JSStringRef stacks = JSValueToStringCopy(ctx, stack, 0);
    char bar[JSStringGetMaximumUTF8CStringSize(stacks)];
    JSStringGetUTF8CString(stacks, bar, JSStringGetMaximumUTF8CStringSize(stacks));
    printf (".stack = %s\n", bar);

    if (back_trace) {
        char mutt[JSStringGetMaximumUTF8CStringSize(back_trace)];
        JSStringGetUTF8CString(back_trace, mutt, JSStringGetMaximumUTF8CStringSize(back_trace));
        printf ("back_trace = %s\n", mutt);
    }
    */

    return scope.Escape(local);
}

Local<StackFrame> StackTrace::GetFrame(uint32_t index) const
{
    StackTraceImpl *impl = V82JSC::ToImpl<StackTraceImpl>(this);
    IsolateImpl *iso = impl->GetIsolate();
    EscapableHandleScope scope(V82JSC::ToIsolate(iso));
    
    Local<Context> context = V82JSC::OperatingContext(V82JSC::ToIsolate(iso));
    JSContextRef ctx = V82JSC::ToContextRef(context);
    
    if (impl->m_stack_frame_array) {
        Local<Array> stack_frames = ValueImpl::New(V82JSC::ToContextImpl(context), impl->m_stack_frame_array).As<Array>();
        if (index < stack_frames->Length()) {
            Local<Value> frame = stack_frames->Get(context, index).ToLocalChecked();
            JSObjectRef array = (JSObjectRef) V82JSC::ToJSValueRef(frame, context);
            Local<String> function_name = ValueImpl::New(V82JSC::ToContextImpl(context),
                                                         JSObjectGetPropertyAtIndex(ctx, array, 0, 0)).As<String>();
            Local<String> script_name = ValueImpl::New(V82JSC::ToContextImpl(context),
                                                       JSObjectGetPropertyAtIndex(ctx, array, 1, 0)).As<String>();
            if (script_name->Equals(context, String::NewFromUtf8(V82JSC::ToIsolate(iso),
                                                        "[undefined]", NewStringType::kNormal).ToLocalChecked()).FromJust() ||
                script_name->Equals(context, String::NewFromUtf8(V82JSC::ToIsolate(iso),
                                                        "[eval]", NewStringType::kNormal).ToLocalChecked()).FromJust()) {
                script_name = Local<String>();
            }
            int line_number = static_cast<int>(JSValueToNumber(ctx, JSObjectGetPropertyAtIndex(ctx, array, 2, 0), 0));
            int col_number = static_cast<int>(JSValueToNumber(ctx, JSObjectGetPropertyAtIndex(ctx, array, 3, 0), 0));
            bool is_eval = JSValueToBoolean(ctx, JSObjectGetPropertyAtIndex(ctx, array, 4, 0));
            
            StackFrameImpl *stack_frame = static_cast<StackFrameImpl*>(H::HeapAllocator::Alloc(iso, iso->m_stack_frame_map));
            stack_frame->m_function_name.Reset(V82JSC::ToIsolate(iso), function_name);
            stack_frame->m_script_name.Reset(V82JSC::ToIsolate(iso), script_name);
            stack_frame->m_line_number = line_number;
            stack_frame->m_column_number = col_number;
            stack_frame->m_is_eval = is_eval;
            stack_frame->m_stack_trace.Reset(V82JSC::ToIsolate(iso), V82JSC::CreateLocal<StackTrace>(&iso->ii, impl));
            
            return scope.Escape(V82JSC::CreateLocal<v8::StackFrame>(&iso->ii, stack_frame));
        }
    }

    return Local<StackFrame>();
}

/**
 * Returns the number of StackFrames.
 */
int StackTrace::GetFrameCount() const
{
    StackTraceImpl *impl = V82JSC::ToImpl<StackTraceImpl>(this);
    HandleScope scope(V82JSC::ToIsolate(impl->GetIsolate()));
    Local<Context> context = V82JSC::OperatingContext(V82JSC::ToIsolate(impl->GetIsolate()));

    if (impl->m_stack_frame_array) {
        Local<Array> stack_frames = ValueImpl::New(V82JSC::ToContextImpl(context), impl->m_stack_frame_array).As<Array>();
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
Local<StackTrace> StackTrace::CurrentStackTrace(Isolate* isolate, int frame_limit,
                                                StackTraceOptions options)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    Local<Script> script;
    auto thread = IsolateImpl::PerThreadData::Get(V82JSC::ToIsolateImpl(isolate));

    if (!thread->m_running_scripts.empty()) {
        script = thread->m_running_scripts.top();
    }

    JSStringRef e = JSStringCreateWithUTF8CString("new Error()");
    JSValueRef error = JSEvaluateScript(V82JSC::ToContextRef(context), e, 0, 0, 0, 0);
    Local<Value> err = ValueImpl::New(V82JSC::ToContextImpl(context), error);
    return scope.Escape(StackTraceImpl::New(V82JSC::ToIsolateImpl(isolate), err, script,
                               JSContextCreateBacktrace(V82JSC::ToContextRef(context), frame_limit)));
}
