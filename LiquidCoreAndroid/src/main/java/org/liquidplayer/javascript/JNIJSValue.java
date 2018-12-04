/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import android.support.v4.util.LongSparseArray;

import java.lang.ref.WeakReference;

class JNIJSValue extends JNIObject {
    protected JNIJSValue(long ref) { super(ref); }

    /*
     * In order to limit back-and-forth through JNI, for those primitive values that can be
     * represented by a jlong (64-bit integer), we will pass the actual value.  We use the following
     * encoding (2 least significant bits):
     *
     * xxx 00 = 62-bit double
     * xxx 10 = oddball value
     * xxx 01 = 4-byte aligned pointer to non-Object JSValue (63/64-bit double or String)
     * xxx 11 = 4-byte aligned pointer to Object JSValue
     *
     * Oddball values (ending in 10):
     * 0010 (0x2) = Undefined
     * 0110 (0x6) = Null
     * 1010 (0xa) = False
     * 1110 (0xe) = True
     *
     * See src/main/cpp/JNI/SharedWrap.h for native mirror
     */
    static final long ODDBALL_UNDEFINED = 0x2;
    static final long ODDBALL_NULL = 0x6;
    static final long ODDBALL_FALSE = 0xa;
    static final long ODDBALL_TRUE = 0xe;
    static boolean isReferencePrimitiveNumber(long ref) { return ((ref&3)==0); }
    private static boolean isReferenceJNI(long ref) { return ((ref&1)==1); }
    static boolean isReferenceObject(long ref) { return ((ref&3)==3); }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        if (isReferenceJNI(reference)) {
            Finalize(reference);
        }
    }

    @Override
    public int hashCode() { return (int) reference; }

    boolean isUndefined() { return isUndefined(reference); }
    boolean isNull() { return isNull(reference); }
    boolean isBoolean() { return isBoolean(reference); }
    boolean isNumber() { return isNumber(reference); }
    boolean isString() { return isString(reference); }
    boolean isObject() { return isReferenceObject(reference); }
    boolean isArray() { return isArray(reference); }
    boolean isDate() { return isDate(reference); }
    boolean isTypedArray() { return isTypedArray(reference); }
    boolean isInt8Array() { return isInt8Array(reference); }
    boolean isInt16Array() { return isInt16Array(reference); }
    boolean isInt32Array() { return isInt32Array(reference); }
    boolean isUint8Array() { return isUint8Array(reference); }
    boolean isUint8ClampedArray() { return isUint8ClampedArray(reference); }
    boolean isUint32Array() { return isUint32Array(reference); }
    boolean isUint16Array() { return isUint16Array(reference); }
    boolean isFloat32Array() { return isFloat32Array(reference); }
    boolean isFloat64Array() { return isFloat64Array(reference); }
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
    boolean toBoolean() { return toBoolean(reference); }
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
        if (!isReferenceJNI(valueRef)) {
            return (valueRef == ODDBALL_FALSE || valueRef == ODDBALL_TRUE) ? new JNIJSBoolean(valueRef) :
                    (valueRef == ODDBALL_NULL) ? new JNIJSNull(valueRef) :
                    (valueRef == ODDBALL_UNDEFINED) ? new JNIJSUndefined(valueRef) :
                        new JNIJSNumber(valueRef);
        }
        WeakReference<JNIJSValue> wr = objectsHash.get(valueRef);
        if (wr != null) {
            JNIJSValue v = wr.get();
            if (v != null) {
                return v;
            }
        }

        JNIJSValue v;
        if (isReferenceObject(valueRef)) {
            v = new JNIJSObject(valueRef);
        } else {
            v = new JNIJSValue(valueRef);
        }
        objectsHash.put(valueRef, new WeakReference<>(v));
        return v;
    }
    static private LongSparseArray<WeakReference<JNIJSValue>> objectsHash = new LongSparseArray<>();

    long canonicalReference()
    {
        return canonicalReference(reference);
    }

    /* Natives */

    static native long makeNumber(long ctxRef, double number);
    static native long makeString(long ctxRef, String string);
    static native long makeFromJSONString(long ctxRef, String string);

    private static native boolean isUndefined(long valueRef);
    private static native boolean isNull(long valueRef);
    private static native boolean isBoolean(long valueRef);
    private static native boolean isNumber(long valueRef);
    private static native boolean isString(long valueRef);
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

    private static native long canonicalReference(long valueRef);
    private native void Finalize(long valueRef);
}
