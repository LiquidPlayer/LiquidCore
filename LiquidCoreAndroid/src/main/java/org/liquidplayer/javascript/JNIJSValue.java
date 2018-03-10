//
// JNIJSValue.java
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

class JNIJSValue extends JNIObject {
    protected JNIJSValue(long ref) {
        super(ref);
    }

    static final long ODDBALL_UNDEFINED = 0x2;
    static final long ODDBALL_NULL = 0x6;
    static final long ODDBALL_FALSE = 0xa;
    static final long ODDBALL_TRUE = 0xe;

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        if ((reference & 0x1) == 1) {
            Finalize(reference);
        }
    }

    @Override
    public int hashCode() {
        return (int) reference;
    }

    boolean isUndefined()
    {
        return isUndefined(reference);
    }
    boolean isNull()
    {
        return isNull(reference);
    }
    boolean isBoolean()
    {
        return isBoolean(reference);
    }
    boolean isNumber()
    {
        return isNumber(reference);
    }
    boolean isString()
    {
        return isString(reference);
    }
    boolean isObject()
    {
        //return isObject(reference);
        return (reference & 0x3) == 0x3;
    }
    boolean isArray()
    {
        return isArray(reference);
    }
    boolean isDate()
    {
        return isDate(reference);
    }
    boolean isTypedArray()
    {
        return isTypedArray(reference);
    }
    boolean isInt8Array()
    {
        return isInt8Array(reference);
    }
    boolean isInt16Array()
    {
        return isInt16Array(reference);
    }
    boolean isInt32Array()
    {
        return isInt32Array(reference);
    }
    boolean isUint8Array()
    {
        return isUint8Array(reference);
    }
    boolean isUint8ClampedArray()
    {
        return isUint8ClampedArray(reference);
    }
    boolean isUint32Array()
    {
        return isUint32Array(reference);
    }
    boolean isUint16Array()
    {
        return isUint16Array(reference);
    }
    boolean isFloat32Array()
    {
        return isFloat32Array(reference);
    }
    boolean isFloat64Array()
    {
        return isFloat64Array(reference);
    }

    boolean isEqual(JNIJSValue b) throws JNIJSException
    {
        return isEqual(reference, b.reference);
    }
    boolean isStrictEqual(JNIJSValue b)
    {
        return isStrictEqual(reference, b.reference);
    }

    JNIJSValue createJSONString() throws JNIJSException
    {
        return JNIJSValue.fromRef(createJSONString(reference));
    }
    boolean toBoolean()
    {
        return toBoolean(reference);
    }
    double toNumber() throws JNIJSException
    {
        return toNumber(reference);
    }
    String toStringCopy() throws JNIJSException
    {
        return toStringCopy(reference);
    }
    JNIJSObject toObject() throws JNIJSException
    {
        if (this instanceof JNIJSObject)
            return (JNIJSObject) this;

        return JNIJSObject.fromRef(toObject(reference));
    }

    static JNIJSValue fromRef(long valueRef)
    {
        if ((valueRef & 0x1L) == 0) {
            return (valueRef == ODDBALL_FALSE || valueRef == ODDBALL_TRUE) ? new JNIJSBoolean(valueRef) :
                    (valueRef == ODDBALL_NULL) ? new JNIJSNull(valueRef) :
                    (valueRef == ODDBALL_UNDEFINED) ? new JNIJSUndefined(valueRef) :
                        new JNIJSNumber(valueRef);
        }
        if ((valueRef & 0x3) == 0x3) {
            return new JNIJSObject(valueRef);
        }
        return new JNIJSValue(valueRef);
    }

    /* Natives */

    static native long makeUndefined(long ctxRef);
    static native long makeNull(long ctxRef);
    static native long makeBoolean(long ctxRef, boolean bool);
    static native long makeNumber(long ctxRef, double number);
    static native long makeString(long ctxRef, String string);
    static native long makeFromJSONString(long ctxRef, String string);

    private static native boolean isUndefined(long valueRef);
    private static native boolean isNull(long valueRef);
    private static native boolean isBoolean(long valueRef);
    private static native boolean isNumber(long valueRef);
    private static native boolean isString(long valueRef);
    private static native boolean isObject(long valueRef);
    private static native boolean isArray(long valueRef);
    private static native boolean isDate(long valueRef);
    private static native boolean isTypedArray(long valueRef);
    private static native boolean isInt8Array(long valueRef);
    private static native boolean isInt16Array(long valueRef);
    private static native boolean isInt32Array(long valueRef);
    private static native boolean isUint8Array(long valueRef);
    private static native boolean isUint8ClampedArray(long valueRef);
    private static native boolean isUint32Array(long valueRef);
    private static native boolean isUint16Array(long valueRef);
    private static native boolean isFloat32Array(long valueRef);
    private static native boolean isFloat64Array(long valueRef);

    private static native boolean isEqual(long valueRef, long b) throws JNIJSException;
    private static native boolean isStrictEqual(long valueRef, long b);

    private static native long createJSONString(long valueRef) throws JNIJSException;
    private static native boolean toBoolean(long valueRef);
    private static native double toNumber(long valueRef) throws JNIJSException;
    private static native String toStringCopy(long valueRef) throws JNIJSException;
    private static native long toObject(long valueRef) throws JNIJSException;

    private native void Finalize(long valueRef);
}
