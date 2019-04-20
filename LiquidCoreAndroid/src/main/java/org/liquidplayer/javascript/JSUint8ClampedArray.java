/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;

/**
 * A convenience class for handling JavaScript's Uint8ClampedArray
 * @since 0.1.0
 */
public class JSUint8ClampedArray extends JSTypedArray<Byte> {
    /**
     * Creates a typed array of length 'length' in JSContext 'context'
     * @param ctx  the JSContext in which to create the typed array
     * @param length  the length of the array in elements
     * @since 0.1.0
     */
    public JSUint8ClampedArray(JSContext ctx, int length) {
        super(ctx,length,"Uint8ClampedArray",Byte.class);
    }

    /**
     * Creates a new JSUint8ClampedArray from the contents of another typed array
     * @param tarr  the typed array from which to create the new array
     * @since 0.1.0
     */
    public JSUint8ClampedArray(JSTypedArray tarr) {
        super(tarr,"Uint8ClampedArray",Byte.class);
    }

    /**
     * Creates new typed array as if by TypedArray.from()
     * @param ctx  The context in which to create the typed array
     * @param object  The object to create the array from
     * @since 0.1.0
     */
    public JSUint8ClampedArray(JSContext ctx, Object object) {
        super(ctx,object,"Uint8ClampedArray",Byte.class);
    }

    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @param byteOffset  The byte offset in the ArrayBuffer to start from
     * @param length  The number of bytes from 'byteOffset' to include in the array
     * @since 0.1.0
     */
    public JSUint8ClampedArray(JSArrayBuffer buffer, int byteOffset, int length) {
        super(buffer,byteOffset,length,"Uint8ClampedArray",Byte.class);
    }
    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @param byteOffset  The byte offset in the ArrayBuffer to start from
     * @since 0.1.0
     */
    public JSUint8ClampedArray(JSArrayBuffer buffer, int byteOffset) {
        super(buffer,byteOffset,"Uint8ClampedArray",Byte.class);
    }
    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @since 0.1.0
     */
    public JSUint8ClampedArray(JSArrayBuffer buffer) {
        super(buffer,"Uint8ClampedArray",Byte.class);
    }

    JSUint8ClampedArray(JNIJSObject valueRef, JSContext ctx) {
        super(valueRef,ctx,Byte.class);
    }

    /**
     * JavaScript: TypedArray.prototype.subarray(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/subarray
     * @param begin  the element to begin at (inclusive)
     * @param end the element to end at (exclusive)
     * @since 0.1.0
     * @return the new typed subarray
     */
    public JSUint8ClampedArray subarray(int begin, int end) {
        return (JSUint8ClampedArray)super.subarray(begin,end);
    }
    /**
     * JavaScript: TypedArray.prototype.subarray(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/subarray
     * @since 0.1.0
     * @param begin  the element to begin at (inclusive)
     * @return the new typed subarray
     */
    public JSUint8ClampedArray subarray(int begin) {
        return (JSUint8ClampedArray)super.subarray(begin);
    }

    private JSUint8ClampedArray(JSUint8ClampedArray superList, int leftBuffer, int rightBuffer) {
        super(superList,leftBuffer,rightBuffer,Byte.class);
    }
    /**
     * @see java.util.List#subList(int, int)
     * @since 0.1.0
     */
    @Override @NonNull
    public JSUint8ClampedArray subList(final int fromIndex, final int toIndex) {
        if (fromIndex < 0 || toIndex > size() || fromIndex > toIndex) {
            throw new IndexOutOfBoundsException();
        }
        return new JSUint8ClampedArray(this,fromIndex,size()-toIndex);
    }
}
