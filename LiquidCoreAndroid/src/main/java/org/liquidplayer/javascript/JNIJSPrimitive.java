/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

abstract class JNIJSPrimitive extends JNIJSValue {
    JNIJSPrimitive(long ref) {
        super(ref);
    }

    @Override boolean isUndefined()          { return false; }
    @Override boolean isNull()               { return false; }
    @Override boolean isBoolean()            { return false; }
    @Override boolean isNumber()             { return false; }
    @Override boolean isString()             { return false; }
    @Override boolean isObject()             { return false; }
    @Override boolean isArray()              { return false; }
    @Override boolean isDate()               { return false; }
    @Override boolean isTypedArray()         { return false; }
    @Override boolean isInt8Array()          { return false; }
    @Override boolean isInt16Array()         { return false; }
    @Override boolean isInt32Array()         { return false; }
    @Override boolean isUint8Array()         { return false; }
    @Override boolean isUint8ClampedArray()  { return false; }
    @Override boolean isUint32Array()        { return false; }
    @Override boolean isUint16Array()        { return false; }
    @Override boolean isFloat32Array()       { return false; }
    @Override boolean isFloat64Array()       { return false; }

    protected boolean primitiveEqual(JNIJSPrimitive b) { return false; }
    protected boolean primitiveStrictEqual(JNIJSPrimitive b) { return false; }

    @Override boolean isEqual(JNIJSValue b) throws JNIJSException
    {
        if (b instanceof JNIJSPrimitive) {
            return primitiveEqual((JNIJSPrimitive)b);
        } else {
            return b.isEqual(this);
        }
    }
    @Override boolean isStrictEqual(JNIJSValue b)
    {
        if (b instanceof JNIJSPrimitive) {
            return primitiveStrictEqual((JNIJSPrimitive)b);
        } else {
            return b.isStrictEqual(this);
        }
    }
    @Override JNIJSValue createJSONString() throws JNIJSException { return null; }
    @Override boolean toBoolean() { return false; }
    @Override double toNumber() throws JNIJSException { return 0; }
    @Override String toStringCopy() throws JNIJSException { return null; }
    @Override JNIJSObject toObject() { return null; }

}
