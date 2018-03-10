//
// JNIJSPrimitive.java
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
    @Override JNIJSObject toObject() throws JNIJSException { return null; }

}
