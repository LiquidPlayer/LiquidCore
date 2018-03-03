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
import android.support.annotation.Nullable;
import android.util.LongSparseArray;

import java.lang.ref.WeakReference;

class JNIJSContext extends JNIObject {
    private JNIJSContext(long ref) {
        super(ref);
        m_contexts.put(ref, new WeakReference<>(this));
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        m_contexts.remove(reference);
        Finalize(reference);
    }

    @NonNull static JNIJSContext createContext()
    {
        return new JNIJSContext(create());
    }

    @NonNull static JNIJSContext createContext(JNIJSContextGroup group)
    {
        return new JNIJSContext(createInGroup(group.reference));
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
        return JNIJSValue.fromRef(JNIJSValue.makeUndefined(reference));
    }

    JNIJSValue makeNull()
    {
        return JNIJSValue.fromRef(JNIJSValue.makeNull(reference));
    }

    JNIJSValue makeBoolean(boolean bool)
    {
        return JNIJSValue.fromRef(JNIJSValue.makeBoolean(reference, bool));
    }

    JNIJSValue makeNumber(double number)
    {
        return JNIJSValue.fromRef(JNIJSValue.makeNumber(reference, number));
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
    JNIJSFunction makeFunction(String name, String func, String sourceURL, int startingLineNumber) throws JNIJSException
    {
        return new JNIJSFunction(JNIJSObject.makeFunction(reference, name, func, sourceURL,
                startingLineNumber));
    }
    JNIJSFunction makeFunctionWithCallback(JSFunction thiz, String name)
    {
        return new JNIJSFunction(JNIJSFunction.makeFunctionWithCallback(thiz, reference, name));
    }

    JNIJSValue evaluateScript(String script, String sourceURL, int startingLineNumber) throws JNIJSException
    {
        return JNIJSValue.fromRef(evaluateScript(reference, script, sourceURL, startingLineNumber));
    }

    @Nullable
    static JNIJSContext fromRef(long ctxRef)
    {
        if (ctxRef == 0) return null;
        WeakReference<JNIJSContext> wr = m_contexts.get(ctxRef);
        if (wr == null || wr.get() == null) {
            return new JNIJSContext(ctxRef);
        } else {
            return wr.get();
        }
    }
    private static LongSparseArray<WeakReference<JNIJSContext>> m_contexts =
            new LongSparseArray<>();

    /* Natives */
    private static native long create();
    private static native long createInGroup(long group);
    private static native long getGroup(long ctxRef);
    private static native long getGlobalObject(long ctxRef);
    private static native long evaluateScript(long ctxRef, String script, String sourceURL,
                                                 int startingLineNumber) throws JNIJSException;
    private static native void Finalize(long ctxRef);
}
