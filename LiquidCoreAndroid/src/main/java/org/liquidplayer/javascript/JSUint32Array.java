/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;

/**
 * A convenience class for handling JavaScript's Uint32Array
 * @since 0.1.0
 */
@SuppressWarnings("WeakerAccess,SameParameterValue")
public class JSUint32Array extends JSTypedArray<Long> {
    /**
     * Creates a typed array of length 'length' in JSContext 'context'
     * @param ctx  the JSContext in which to create the typed array
     * @param length  the length of the array in elements
     * @since 0.1.0
     */
    public JSUint32Array(JSContext ctx, int length) {
        super(ctx,length,"Uint32Array",Long.class);
    }

    /**
     * Creates a new JSUint32Array from the contents of another typed array
     * @param tarr  the typed array from which to create the new array
     * @since 0.1.0
     */
    public JSUint32Array(JSTypedArray tarr) {
        super(tarr,"Uint32Array",Long.class);
    }

    /**
     * Creates new typed array as if by TypedArray.from()
     * @param ctx  The context in which to create the typed array
     * @param object  The object to create the array from
     * @since 0.1.0
     */
    public JSUint32Array(JSContext ctx, Object object) {
        super(ctx,object,"Uint32Array",Long.class);
    }

    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @param byteOffset  The byte offset in the ArrayBuffer to start from
     * @param length  The number of bytes from 'byteOffset' to include in the array
     * @since 0.1.0
     */
    public JSUint32Array(JSArrayBuffer buffer, int byteOffset, int length) {
        super(buffer,byteOffset,length,"Uint32Array",Long.class);
    }
    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @param byteOffset  The byte offset in the ArrayBuffer to start from
     * @since 0.1.0
     */
    public JSUint32Array(JSArrayBuffer buffer, int byteOffset) {
        super(buffer,byteOffset,"Uint32Array",Long.class);
    }
    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @since 0.1.0
     */
    public JSUint32Array(JSArrayBuffer buffer) {
        super(buffer,"Uint32Array",Long.class);
    }

    /**
     * Treats an existing value as a typed array
     * @param valueRef  the JavaScriptCore value reference
     * @param ctx  The JSContext of the value
     * @since 0.1.0
     */
    public JSUint32Array(JNIJSObject valueRef, JSContext ctx) {
        super(valueRef,ctx,Long.class);
    }

    /**
     * JavaScript: TypedArray.prototype.subarray(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/subarray
     * @since 0.1.0
     * @param begin  the element to begin at (inclusive)
     * @param end the element to end at (exclusive)
     * @return the new typed subarray
     */
    public JSUint32Array subarray(int begin, int end) {
        return (JSUint32Array)super.subarray(begin,end);
    }
    /**
     * JavaScript: TypedArray.prototype.subarray(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/subarray
     * @since 0.1.0
     * @param begin  the element to begin at (inclusive)
     * @return the new typed subarray
     */
    public JSUint32Array subarray(int begin) {
        return (JSUint32Array)super.subarray(begin);
    }

    private JSUint32Array(JSUint32Array superList, int leftBuffer, int rightBuffer) {
        super(superList,leftBuffer,rightBuffer,Long.class);
    }
    /**
     * @see java.util.List#subList(int, int)
     * @since 0.1.0
     */
    @Override @NonNull
    @SuppressWarnings("unchecked")
    public JSUint32Array subList(final int fromIndex, final int toIndex) {
        if (fromIndex < 0 || toIndex > size() || fromIndex > toIndex) {
            throw new IndexOutOfBoundsException();
        }
        return new JSUint32Array(this,fromIndex,size()-toIndex);
    }
}
