/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import androidx.annotation.NonNull;

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
    JNIJSObject callAsConstructor(JNIJSValue[] args) throws JNIJSException
    {
        long [] args_ = new long[args.length];
        for (int i=0; i<args.length; i++) {
            args_[i] = args[i].reference;
        }
        return JNIJSObject.fromRef(callAsConstructor(reference, args_));
    }
    String[] copyPropertyNames()
    {
        return copyPropertyNames(reference);
    }

    static JNIJSObject fromRef(long valueRef)
    {
        return (JNIJSObject) JNIJSValue.fromRef(valueRef);
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
