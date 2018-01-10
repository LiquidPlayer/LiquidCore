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

class JNIJSObject extends JNIJSValue {
    JNIJSObject(long ref) {
        super(ref);
    }

    /* Native Methods */

    static native JNIJSObject make(JNIJSContext ctx);
    static native JNIReturnObject makeArray(JNIJSContext ctx, JNIJSValue [] args);
    static native JNIJSObject makeDate(JNIJSContext ctx, long[] args);
    static native JNIJSObject makeError(JNIJSContext ctx, String message);
    static native JNIReturnObject makeRegExp(JNIJSContext ctx, String pattern, String flags);
    static native JNIReturnObject makeFunction(JNIJSContext ctx, String name, String func, String sourceURL,
                                               int startingLineNumber);

    native JNIJSValue getPrototype();
    native void setPrototype(JNIJSValue value);
    native boolean hasProperty(String propertyName);
    native JNIReturnObject getProperty(String propertyName);
    native JNIReturnObject setProperty(String propertyName, JNIJSValue value, int attributes);
    native JNIReturnObject deleteProperty(String propertyName);
    native JNIReturnObject getPropertyAtIndex(int propertyIndex);
    native JNIReturnObject setPropertyAtIndex(int propertyIndex, JNIJSValue value);
    native boolean isFunction();
    native JNIReturnObject callAsFunction(JNIJSObject thisObject, JNIJSValue [] args);
    native boolean isConstructor();
    native JNIReturnObject callAsConstructor(JNIJSValue [] args);
    native String[] copyPropertyNames();
}
