//
// JNIJSObject.java
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

import java.lang.ref.Reference;

class JNIJSObject extends JNIJSValue {
    protected JNIJSObject(long ref) {
        super(ref);
    }

    JNIJSValue getPrototype()
    {
        return JNIJSValue.fromRef(getPrototype(reference));
    }
    void setPrototype(@NonNull JNIJSValue value)
    {
        setPrototype(reference, value.reference);
    }
    boolean hasProperty(String propertyName)
    {
        return hasProperty(reference, propertyName);
    }
    JNIJSValue getProperty(String propertyName) throws JNIJSException
    {
        return JNIJSValue.fromRef(getProperty(reference, propertyName));
    }
    void setProperty(String propertyName, JNIJSValue value, int attributes) throws JNIJSException
    {
        setProperty(reference, propertyName, value.reference, attributes);
    }
    boolean deleteProperty(String propertyName) throws JNIJSException
    {
        return deleteProperty(reference, propertyName);
    }
    JNIJSValue getPropertyAtIndex(int propertyIndex) throws JNIJSException
    {
        return JNIJSValue.fromRef(getPropertyAtIndex(reference, propertyIndex));
    }
    void setPropertyAtIndex(int propertyIndex, JNIJSValue value) throws JNIJSException
    {
        setPropertyAtIndex(reference, propertyIndex, value.reference);
    }
    boolean isFunction()
    {
        return isFunction(reference);
    }
    JNIJSValue callAsFunction(JNIJSObject thisObject, JNIJSValue[] args) throws JNIJSException
    {
        long [] args_ = new long[args.length];
        for (int i=0; i<args.length; i++) {
            args_[i] = args[i].reference;
        }
        return JNIJSValue.fromRef(callAsFunction(reference,
                thisObject==null?0:thisObject.reference, args_));
    }
    boolean isConstructor()
    {
        return isConstructor(reference);
    }
    JNIJSValue callAsConstructor(JNIJSValue[] args) throws JNIJSException
    {
        long [] args_ = new long[args.length];
        for (int i=0; i<args.length; i++) {
            args_[i] = args[i].reference;
        }
        return JNIJSValue.fromRef(callAsConstructor(reference, args_));
    }
    String[] copyPropertyNames()
    {
        return copyPropertyNames(reference);
    }

    @Nullable static JNIJSObject fromRef(long valueRef)
    {
        JNIJSValue v = JNIJSValue.fromRef(valueRef);
        if (v instanceof JNIJSObject) return (JNIJSObject)v;
        else return null;
    }

    /* Native Methods */

    static native long make(long ctxRef);
    static native long makeArray(long ctxRef, long[] args) throws JNIJSException;
    static native long makeDate(long ctxRef, long[] args);
    static native long makeError(long ctxRef, String message);
    static native long makeRegExp(long ctxRef, String pattern, String flags) throws JNIJSException;
    static native long makeFunction(long ctxRef, String name, String func, String sourceURL,
                                               int startingLineNumber) throws JNIJSException;
    static native boolean isFunction(long objRef);

    private static native long getPrototype(long objRef);
    private static native void setPrototype(long objRef, long value);
    private static native boolean hasProperty(long objRef, String propertyName);
    private static native long getProperty(long objRef, String propertyName) throws JNIJSException;
    private static native void setProperty(long objRef, String propertyName, long valueRef, int attributes) throws JNIJSException;
    private static native boolean deleteProperty(long objRef, String propertyName) throws JNIJSException;
    private static native long getPropertyAtIndex(long objRef, int propertyIndex) throws JNIJSException;
    private static native void setPropertyAtIndex(long objRef, int propertyIndex, long valueRef) throws JNIJSException;
    private static native long callAsFunction(long objRef, long thisObject, long[] args) throws JNIJSException;
    private static native boolean isConstructor(long objRef);
    private static native long callAsConstructor(long objRef, long []args) throws JNIJSException;
    private static native String[] copyPropertyNames(long objRef);
}
