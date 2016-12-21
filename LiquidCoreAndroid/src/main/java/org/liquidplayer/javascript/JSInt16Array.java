//
// JSInt16Array.java
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

import android.support.annotation.NonNull;

/**
 * A convenience class for handling JavaScript's Int16Array
 */
public class JSInt16Array extends JSTypedArray<Short> {
    /**
     * Creates a typed array of length 'length' in JSContext 'context'
     * @since 0.1.0
     * @param ctx  the JSContext in which to create the typed array
     * @param length  the length of the array in elements
     */
    public JSInt16Array(JSContext ctx, int length) {
        super(ctx,length,"Int16Array",Short.class);
    }

    /**
     * Creates a new JSInt16Array from the contents of another typed array
     * @since 0.1.0
     * @param tarr  the typed array from which to create the new array
     */
    public JSInt16Array(JSTypedArray tarr) {
        super(tarr,"Int16Array",Short.class);
    }

    /**
     * Creates new typed array as if by TypedArray.from()
     * @since 0.1.0
     * @param ctx  The context in which to create the typed array
     * @param object  The object to create the array from
     */
    public JSInt16Array(JSContext ctx, Object object) {
        super(ctx,object,"Int16Array",Short.class);
    }

    /**
     * Creates a typed array from a JSArrayBuffer
     * @since 0.1.0
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @param byteOffset  The byte offset in the ArrayBuffer to start from
     * @param length  The number of bytes from 'byteOffset' to include in the array
     */
    public JSInt16Array(JSArrayBuffer buffer, int byteOffset, int length) {
        super(buffer,byteOffset,length,"Int16Array",Short.class);
    }
    /**
     * Creates a typed array from a JSArrayBuffer
     * @since 0.1.0
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @param byteOffset  The byte offset in the ArrayBuffer to start from
     */
    public JSInt16Array(JSArrayBuffer buffer, int byteOffset) {
        super(buffer,byteOffset,"Int16Array",Short.class);
    }
    /**
     * Creates a typed array from a JSArrayBuffer
     * @since 0.1.0
     * @param buffer  The JSArrayBuffer to create the typed array from
     */
    public JSInt16Array(JSArrayBuffer buffer) {
        super(buffer,"Int16Array",Short.class);
    }

    /**
     * Treats an existing value as a typed array
     * @since 0.1.0
     * @param valueRef  the JavaScriptCore value reference
     * @param ctx  The JSContext of the value
     */
    public JSInt16Array(long valueRef, JSContext ctx) {
        super(valueRef,ctx,Short.class);
    }

    /**
     * JavaScript: TypedArray.prototype.subarray(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/subarray
     * @since 0.1.0
     * @param begin  the element to begin at (inclusive)
     * @param end the element to end at (exclusive)
     * @return the new typed subarray
     */
    public JSInt16Array subarray(int begin, int end) {
        return (JSInt16Array)super.subarray(begin,end);
    }
    /**
     * JavaScript: TypedArray.prototype.subarray(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/subarray
     * @since 0.1.0
     * @param begin  the element to begin at (inclusive)
     * @return the new typed subarray
     */
    public JSInt16Array subarray(int begin) {
        return (JSInt16Array)super.subarray(begin);
    }

    private JSInt16Array(JSInt16Array superList, int leftBuffer, int rightBuffer) {
        super(superList,leftBuffer,rightBuffer,Short.class);
    }
    /**
     * @see java.util.List#subList(int, int)
     * @since 3.0
     */
    @Override @NonNull
    @SuppressWarnings("unchecked")
    public JSInt16Array subList(final int fromIndex, final int toIndex) {
        if (fromIndex < 0 || toIndex > size() || fromIndex > toIndex) {
            throw new IndexOutOfBoundsException();
        }
        return new JSInt16Array(this,fromIndex,size()-toIndex);
    }
}
