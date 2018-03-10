//
// JSArrayBuffer.java
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
 Copyright (c) 2014-2016 Eric Lange. All rights reserved.

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

/**
 * A wrapper class for a JavaScript ArrayBuffer
 * See: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer
 * Note, experimental ArrayBuffer.transfer() is not supported by this JavaScriptCore version
 * @since 0.1.0
 */
@SuppressWarnings("WeakerAccess,SameParameterValue")
public class JSArrayBuffer extends JSObjectWrapper {
    /**
     * Creates a new array buffer of 'length' bytes
     * @param ctx  the JSContext in which to create the ArrayBuffer
     * @param length  the length in bytes of the ArrayBuffer
     * @since 0.1.0
     */
    public JSArrayBuffer(JSContext ctx, int length) {
        super(new JSFunction(ctx,"_ArrayBuffer",new String[] {"length"},
                "return new ArrayBuffer(length);",
                null, 0).call(null,length).toObject());
    }

    /**
     * Treats an existing JSObject as an ArrayBuffer.  It is up to the user to ensure the
     * underlying JSObject is actually an ArrayBuffer.
     * @param buffer  The ArrayBuffer JSObject to wrap
     * @since 0.1.0
     */
    public JSArrayBuffer(JSObject buffer) {
        super(buffer);
    }

    /**
     * JavaScript: ArrayBuffer.prototype.byteLength, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer/byteLength
     * @return length of ArrayBuffer in bytes
     * @since 0.1.0
     */
    public int byteLength() {
        return property("byteLength").toNumber().intValue();
    }

    /**
     * JavaScript: ArrayBuffer.isView(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer/isView
     * @param arg the argument to be checked
     * @return true if arg is one of the ArrayBuffer views, such as typed array objects or
     * a DataView; false otherwise
     * @since 0.1.0
     */
    public static boolean isView(JSValue arg) {
        return arg.getContext().property("ArrayBuffer").toObject().property("isView").toFunction()
                .call(null,arg).toBoolean();
    }

    /**
     * JavaScript: ArrayBuffer.prototype.slice(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer/slice
     * @param begin Zero-based byte index at which to begin slicing
     * @param end Byte index to end slicing
     * @return new ArrayBuffer with sliced contents copied
     * @since 0.1.0
     */
    public JSArrayBuffer slice(int begin, int end) {
        return new JSArrayBuffer(
                property("slice").toFunction().call(this,begin,end).toObject());
    }
    /**
     * JavaScript: ArrayBuffer.prototype.slice(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer/slice
     * @param begin Zero-based byte index at which to begin slicing
     * @return new ArrayBuffer with sliced contents copied
     * @since 0.1.0
     */
    public JSArrayBuffer slice(int begin) {
        return new JSArrayBuffer(
                property("slice").toFunction().call(this,begin).toObject());
    }
}
