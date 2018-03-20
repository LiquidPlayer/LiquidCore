//
//  TryCatch.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/**
 * Creates a new try/catch block and registers it with v8.  Note that
 * all TryCatch blocks should be stack allocated because the memory
 * location itself is compared against JavaScript try/catch blocks.
 */
TryCatch::TryCatch(Isolate* isolate)
{
    
}

/**
 * Unregisters and deletes this try/catch block.
 */
TryCatch::~TryCatch()
{
    
}

/**
 * Returns true if an exception has been caught by this try/catch block.
 */
bool TryCatch::HasCaught() const
{
    return false;
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
    return false;
}

/**
 * Throws the exception caught by this TryCatch in a way that avoids
 * it being caught again by this same TryCatch.  As with ThrowException
 * it is illegal to execute any JavaScript operations after calling
 * ReThrow; the caller must return immediately to where the exception
 * is caught.
 */
Local<Value> TryCatch::ReThrow()
{
    return Local<Value>();
}

/**
 * Returns the exception caught by this try/catch block.  If no exception has
 * been caught an empty handle is returned.
 *
 * The returned handle is valid until this TryCatch block has been destroyed.
 */
Local<Value> TryCatch::Exception() const
{
    return Local<Value>();
}

/**
 * Returns the .stack property of the thrown object.  If no .stack
 * property is present an empty handle is returned.
 */
MaybeLocal<Value> TryCatch::StackTrace(Local<Context> context) const
{
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
    
}

/**
 * Returns true if verbosity is enabled.
 */
bool TryCatch::IsVerbose() const
{
    return false;
}

/**
 * Set whether or not this TryCatch should capture a Message object
 * which holds source information about where the exception
 * occurred.  True by default.
 */
void TryCatch::SetCaptureMessage(bool value)
{
    
}
