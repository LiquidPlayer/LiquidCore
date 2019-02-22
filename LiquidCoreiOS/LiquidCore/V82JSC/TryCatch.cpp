/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "Message.h"

using namespace V82JSC;
using namespace v8;
#define THE_HOLE reinterpret_cast<IsolateImpl*>(isolate_)->ii.heap()->root(v8::internal::Heap::RootListIndex::kTheHoleValueRootIndex)
/**
 * Creates a new try/catch block and registers it with v8.  Note that
 * all TryCatch blocks should be stack allocated because the memory
 * location itself is compared against JavaScript try/catch blocks.
 */
TryCatch::TryCatch(Isolate* isolate)
{
    IsolateImpl *i = reinterpret_cast<IsolateImpl *>(isolate);
    isolate_ = reinterpret_cast<internal::Isolate *>(isolate);
    auto thread = IsolateImpl::PerThreadData::Get(i);
    
    next_ = thread->m_handlers;
    thread->m_handlers = this;
    
    exception_ = nullptr;
    message_obj_ = nullptr;
    js_stack_comparable_address_ = nullptr;
    is_verbose_ = false;
    rethrow_ = false;
}

/**
 * Unregisters and deletes this try/catch block.
 */
TryCatch::~TryCatch()
{
    auto thread = IsolateImpl::PerThreadData::Get(reinterpret_cast<IsolateImpl*>(isolate_));
    thread->m_handlers = next_;
    
    if (is_verbose_) {
        thread->m_verbose_exception = (JSValueRef) exception_;
    }

    if (rethrow_ && !Exception().IsEmpty()) {
        reinterpret_cast<v8::Isolate*>(isolate_)->ThrowException(Exception());
    } else {
        thread->m_scheduled_exception = THE_HOLE;
    }
}

/**
 * Returns true if an exception has been caught by this try/catch block.
 */
bool TryCatch::HasCaught() const
{
    auto thread = IsolateImpl::PerThreadData::Get(reinterpret_cast<IsolateImpl*>(isolate_));
    return exception_ != nullptr || thread->m_scheduled_exception != THE_HOLE;
}

/**
 * For certain types of exceptions, it makes no sense to continue execution.
 *
 * If CanContinue returns false, the correct action is to perform any C++
 * cleanup needed and then return.  If CanContinue returns false and
 * HasTerminated returns true, it is possible to call
 * CancelTerminateExecution in order to continue calling into the engine.
 */
bool TryCatch::CanContinue() const
{
    assert(0);
    return false;
}

/**
 * Returns true if an exception has been caught due to script execution
 * being terminated.
 *
 * There is no JavaScript representation of an execution termination
 * exception.  Such exceptions are thrown when the TerminateExecution
 * methods are called to terminate a long-running script.
 *
 * If such an exception has been thrown, HasTerminated will return true,
 * indicating that it is possible to call CancelTerminateExecution in order
 * to continue calling into the engine.
 */
bool TryCatch::HasTerminated() const
{
    return reinterpret_cast<IsolateImpl*>(isolate_)->m_terminate_execution;
}

/**
 * Throws the exception caught by this TryCatch in a way that avoids
 * it being caught again by this same TryCatch.  As with ThrowException
 * it is illegal to execute any JavaScript operations after calling
 * ReThrow; the caller must return immediately to where the exception
 * is caught.
 */
Local<v8::Value> TryCatch::ReThrow()
{
    rethrow_ = true;
    return Exception();
}

/**
 * Returns the exception caught by this try/catch block.  If no exception has
 * been caught an empty handle is returned.
 *
 * The returned handle is valid until this TryCatch block has been destroyed.
 */
Local<v8::Value> TryCatch::Exception() const
{
    EscapableHandleScope scope(reinterpret_cast<v8::Isolate*>(isolate_));
    auto thread = IsolateImpl::PerThreadData::Get(reinterpret_cast<IsolateImpl*>(isolate_));
    Local<v8::Context> context = reinterpret_cast<Isolate*>(this->isolate_)->GetCurrentContext();
    
    internal::Object *sched = thread->m_scheduled_exception;
    JSValueRef excep = (JSValueRef) exception_;
    if (sched != THE_HOLE) {
        excep = ToJSValueRef_<V82JSC::Value>(sched, context);
    }

    if (excep) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), (JSValueRef)excep));
    }
    return Local<Value>();
}

/**
 * Returns the .stack property of the thrown object.  If no .stack
 * property is present an empty handle is returned.
 */
MaybeLocal<v8::Value> TryCatch::StackTrace(Local<Context> context) const
{
    EscapableHandleScope scope(reinterpret_cast<v8::Isolate*>(isolate_));
    Local<Value> exception = Exception();
    if (!exception.IsEmpty() && exception->IsObject()) {
        MaybeLocal<Value> stack = exception.As<Object>()->
            Get(context,
                String::NewFromUtf8(reinterpret_cast<v8::Isolate*>(isolate_), "stack", NewStringType::kNormal).ToLocalChecked());
        if (!stack.IsEmpty() && !stack.ToLocalChecked()->IsUndefined()) {
            return scope.Escape(stack.ToLocalChecked());
        }
    }
    return MaybeLocal<Value>();
}

/**
 * Returns the message associated with this exception.  If there is
 * no message associated an empty handle is returned.
 *
 * The returned handle is valid until this TryCatch block has been
 * destroyed.
 */
Local<v8::Message> TryCatch::Message() const
{
    EscapableHandleScope scope(reinterpret_cast<v8::Isolate*>(isolate_));

    if (message_obj_) {
        auto impl = reinterpret_cast<V82JSC::Message*>(message_obj_);
        return scope.Escape(CreateLocal<v8::Message>(isolate_, impl));
    }
    return Local<v8::Message>();
}

/**
 * Clears any exceptions that may have been caught by this try/catch block.
 * After this method has been called, HasCaught() will return false. Cancels
 * the scheduled exception if it is caught and ReThrow() is not called before.
 *
 * It is not necessary to clear a try/catch block before using it again; if
 * another exception is thrown the previously caught exception will just be
 * overwritten.  However, it is often a good idea since it makes it easier
 * to determine which operation threw a given exception.
 */
void TryCatch::Reset()
{
    exception_ = nullptr;
    message_obj_ = nullptr;
    js_stack_comparable_address_ = nullptr;
    rethrow_ = false;
    auto thread = IsolateImpl::PerThreadData::Get(reinterpret_cast<IsolateImpl*>(isolate_));
    thread->m_scheduled_exception = THE_HOLE;
}

/**
 * Set verbosity of the external exception handler.
 *
 * By default, exceptions that are caught by an external exception
 * handler are not reported.  Call SetVerbose with true on an
 * external exception handler to have exceptions caught by the
 * handler reported as if they were not caught.
 */
void TryCatch::SetVerbose(bool value)
{
    is_verbose_ = true;
}

/**
 * Returns true if verbosity is enabled.
 */
bool TryCatch::IsVerbose() const
{
    return is_verbose_;
}

/**
 * Set whether or not this TryCatch should capture a Message object
 * which holds source information about where the exception
 * occurred.  True by default.
 */
void TryCatch::SetCaptureMessage(bool value)
{
    assert(0);
}
