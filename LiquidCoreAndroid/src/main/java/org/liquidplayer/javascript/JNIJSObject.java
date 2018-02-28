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

import java.lang.ref.WeakReference;

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
    JNIJSValue getProperty(String propertyName) throws JNIJSValue
    {
        JNIReturnObject r = getProperty(reference, propertyName);
        if (r.exception != 0) {
            throw JNIJSValue.fromRef(r.exception);
        }
        return JNIJSValue.fromRef(r.reference);
    }
    void setProperty(String propertyName, JNIJSValue value, int attributes) throws JNIJSValue
    {
        JNIReturnObject r = setProperty(reference, propertyName, value.reference, attributes);
        if (r.exception != 0) {
            throw JNIJSValue.fromRef(r.exception);
        }
    }
    boolean deleteProperty(String propertyName) throws JNIJSValue
    {
        JNIReturnObject r = deleteProperty(reference, propertyName);
        if (r.exception != 0) {
            throw JNIJSValue.fromRef(r.exception);
        }
        return r.bool;
    }
    JNIJSValue getPropertyAtIndex(int propertyIndex) throws JNIJSValue
    {
        JNIReturnObject r = getPropertyAtIndex(reference, propertyIndex);
        if (r.exception != 0) {
            throw JNIJSValue.fromRef(r.exception);
        }
        return JNIJSValue.fromRef(r.reference);
    }
    void setPropertyAtIndex(int propertyIndex, JNIJSValue value) throws JNIJSValue
    {
        JNIReturnObject r = setPropertyAtIndex(reference, propertyIndex, value.reference);
        if (r.exception != 0) {
            throw JNIJSValue.fromRef(r.exception);
        }
    }
    boolean isFunction()
    {
        return isFunction(reference);
    }
    JNIJSValue callAsFunction(JNIJSObject thisObject, JNIJSValue[] args) throws JNIJSValue
    {
        long [] args_ = new long[args.length];
        for (int i=0; i<args.length; i++) {
            args_[i] = args[i].reference;
        }
        JNIReturnObject r = callAsFunction(reference, thisObject==null?0:thisObject.reference, args_);
        if (r.exception != 0) {
            throw JNIJSValue.fromRef(r.exception);
        }
        return JNIJSValue.fromRef(r.reference);
    }
    boolean isConstructor()
    {
        return isConstructor(reference);
    }
    JNIJSValue callAsConstructor(JNIJSValue[] args) throws JNIJSValue
    {
        long [] args_ = new long[args.length];
        for (int i=0; i<args.length; i++) {
            args_[i] = args[i].reference;
        }
        JNIReturnObject r = callAsConstructor(reference, args_);
        if (r.exception != 0) {
            throw JNIJSValue.fromRef(r.exception);
        }
        return JNIJSValue.fromRef(r.reference);
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
    static native JNIReturnObject makeArray(long ctxRef, long[] args);
    static native long makeDate(long ctxRef, long[] args);
    static native long makeError(long ctxRef, String message);
    static native JNIReturnObject makeRegExp(long ctxRef, String pattern, String flags);
    static native JNIReturnObject makeFunction(long ctxRef, String name, String func, String sourceURL,
                                               int startingLineNumber);

    private static native long getPrototype(long objRef);
    private static native void setPrototype(long objRef, long value);
    private static native boolean hasProperty(long objRef, String propertyName);
    private static native JNIReturnObject getProperty(long objRef, String propertyName);
    private static native JNIReturnObject setProperty(long objRef, String propertyName, long valueRef, int attributes);
    private static native JNIReturnObject deleteProperty(long objRef, String propertyName);
    private static native JNIReturnObject getPropertyAtIndex(long objRef, int propertyIndex);
    private static native JNIReturnObject setPropertyAtIndex(long objRef, int propertyIndex, long valueRef);
    private static native boolean isFunction(long objRef);
    private static native JNIReturnObject callAsFunction(long objRef, long thisObject, long[] args);
    private static native boolean isConstructor(long objRef);
    private static native JNIReturnObject callAsConstructor(long objRef, long []args);
    private static native String[] copyPropertyNames(long objRef);
}
