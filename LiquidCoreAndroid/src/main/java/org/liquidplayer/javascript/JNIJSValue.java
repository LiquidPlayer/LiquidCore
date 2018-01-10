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

public class JNIJSValue extends JNIObject {
    JNIJSValue(long ref) {
        super(ref);
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        Finalize(reference);
    }

    @Override
    public int hashCode() {
        return (int) reference;
    }

    static native JNIJSValue makeUndefined(JNIJSContext ctx);
    static native JNIJSValue makeNull(JNIJSContext ctx);
    static native JNIJSValue makeBoolean(JNIJSContext ctx, boolean bool);
    static native JNIJSValue makeNumber(JNIJSContext ctx, double number);
    static native JNIJSValue makeString(JNIJSContext ctx, String string);
    static native JNIJSValue makeFromJSONString(JNIJSContext ctx, String string);

    native boolean isUndefined();
    native boolean isNull();
    native boolean isBoolean();
    native boolean isNumber();
    native boolean isString();
    native boolean isObject();
    native boolean isArray();
    native boolean isDate();
    native JNIReturnObject isEqual(JNIJSValue b);
    native boolean isStrictEqual(JNIJSValue b);

    native JNIReturnObject createJSONString();
    native boolean toBoolean();
    native JNIReturnObject toNumber();
    native JNIReturnObject toStringCopy();
    native JNIReturnObject toObject();

    native void Finalize(long reference);
}
