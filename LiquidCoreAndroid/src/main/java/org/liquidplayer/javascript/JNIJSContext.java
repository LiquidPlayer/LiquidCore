//
// JNIJSContext.java
//
// AndroidJSCore project
// https://github.com/ericwlange/AndroidJSCore/
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
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
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;

class JNIJSContext extends JNIObject {
    private JNIJSContext(long ref) {
        super(ref);
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        Finalize(reference);
    }

    @NonNull static JNIJSContext createContext()
    {
        return fromRef(create());
    }

    @NonNull static JNIJSContext createContext(JNIJSContextGroup group)
    {
        return fromRef(createInGroup(group.reference));
    }

    JNIJSContextGroup getGroup()
    {
        return JNIJSContextGroup.fromRef(getGroup(reference));
    }

    JNIJSObject getGlobalObject()
    {
        return JNIJSObject.fromRef(getGlobalObject(reference));
    }

    JNIJSValue makeUndefined()
    {
        return JNIJSValue.fromRef(JNIJSValue.ODDBALL_UNDEFINED);
    }

    JNIJSValue makeNull()
    {
        return JNIJSValue.fromRef(JNIJSValue.ODDBALL_NULL);
    }

    JNIJSValue makeBoolean(boolean bool)
    {
        return JNIJSValue.fromRef(bool ? JNIJSValue.ODDBALL_TRUE : JNIJSValue.ODDBALL_FALSE);
    }

    JNIJSValue makeNumber(double number)
    {
        long ref = Double.doubleToLongBits(number);
        if (!JNIJSValue.isReferencePrimitiveNumber(ref)) {
            ref = JNIJSValue.makeNumber(reference, number);
        }
        return JNIJSValue.fromRef(ref);
    }

    JNIJSValue makeString(String string)
    {
        return JNIJSValue.fromRef(JNIJSValue.makeString(reference, string));
    }

    JNIJSValue makeFromJSONString(String string)
    {
        return JNIJSValue.fromRef(JNIJSValue.makeFromJSONString(reference, string));
    }

    JNIJSObject make()
    {
        return JNIJSObject.fromRef(JNIJSObject.make(reference));
    }
    JNIJSObject makeArray(JNIJSValue[] args) throws JNIJSException
    {
        long [] args_ = new long[args.length];
        for (int i=0; i<args.length; i++) {
            args_[i] = args[i].reference;
        }
        return JNIJSObject.fromRef(JNIJSObject.makeArray(reference, args_));
    }
    JNIJSObject makeDate(long[] args)
    {
        return JNIJSObject.fromRef(JNIJSObject.makeDate(reference, args));
    }
    JNIJSObject makeError(String message)
    {
        return JNIJSObject.fromRef(JNIJSObject.makeError(reference, message));
    }
    JNIJSObject makeRegExp(String pattern, String flags) throws JNIJSException
    {
        return JNIJSObject.fromRef(JNIJSObject.makeRegExp(reference, pattern, flags));
    }
    JNIJSObject makeFunction(@NonNull String name, @NonNull String func,
                               @NonNull String sourceURL, int startingLineNumber) throws JNIJSException
    {
        return JNIJSFunction.fromRef(JNIJSObject.makeFunction(reference, name, func, sourceURL,
                startingLineNumber));
    }
    JNIJSObject makeFunctionWithCallback(JSFunction thiz, String name)
    {
        return JNIJSFunction.fromRef(JNIJSFunction.makeFunctionWithCallback(thiz, reference, name));
    }

    JNIJSValue evaluateScript(String script, String sourceURL, int startingLineNumber) throws JNIJSException
    {
        return JNIJSValue.fromRef(evaluateScript(reference, script, sourceURL, startingLineNumber));
    }

    @NonNull
    static JNIJSContext fromRef(long ctxRef)
    {
        return new JNIJSContext(ctxRef);
    }

    /* Natives */
    private static native long create();
    private static native long createInGroup(long group);
    private static native long getGroup(long ctxRef);
    private static native long getGlobalObject(long ctxRef);
    private static native long evaluateScript(long ctxRef, String script, String sourceURL,
                                                 int startingLineNumber) throws JNIJSException;
    private static native void Finalize(long ctxRef);
}
