/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;

/**
 * A convenience class for handling JavaScript's Float32Array
 * @since 0.1.0
 */
@SuppressWarnings("WeakerAccess,SameParameterValue")
public class JSFloat32Array extends JSTypedArray<Float> {
    /**
     * Creates a typed array of length 'length' in JSContext 'context'
     * @param ctx  the JSContext in which to create the typed array
     * @param length  the length of the array in elements
     * @since 0.1.0
     */
    public JSFloat32Array(JSContext ctx, int length) {
        super(ctx,length,"Float32Array",Float.class);
    }

    /**
     * Creates a new JSFloat32Array from the contents of another typed array
     * @param tarr  the typed array from which to create the new array
     * @since 0.1.0
     */
    public JSFloat32Array(JSTypedArray tarr) {
        super(tarr,"Float32Array",Float.class);
    }

    /**
     * Creates new typed array as if by TypedArray.from()
     * @param ctx  The context in which to create the typed array
     * @param object  The object to create the array from
     * @since 0.1.0
     */
    public JSFloat32Array(JSContext ctx, Object object) {
        super(ctx,object,"Float32Array",Float.class);
    }

    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @param byteOffset  The byte offset in the ArrayBuffer to start from
     * @param length  The number of elements (not bytes!) from 'byteOffset' to include in the array
     * @since 0.1.0
     */
    public JSFloat32Array(JSArrayBuffer buffer, int byteOffset, int length) {
        super(buffer,byteOffset,length,"Float32Array",Float.class);
    }
    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @param byteOffset  The byte offset in the ArrayBuffer to start from
     * @since 0.1.0
     */
    public JSFloat32Array(JSArrayBuffer buffer, int byteOffset) {
        super(buffer,byteOffset,"Float32Array",Float.class);
    }
    /**
     * Creates a typed array from a JSArrayBuffer
     * @param buffer  The JSArrayBuffer to create the typed array from
     * @since 0.1.0
     */
    public JSFloat32Array(JSArrayBuffer buffer) {
        super(buffer,"Float32Array",Float.class);
    }

    /**
     * Treats an existing value as a typed array
     * @param valueRef  the JavaScriptCore value reference
     * @param ctx  The JSContext of the value
     * @since 0.1.0
     */
    public JSFloat32Array(JNIJSObject valueRef, JSContext ctx) {
        super(valueRef,ctx,Float.class);
    }

    /**
     * JavaScript: TypedArray.prototype.subarray(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/subarray
     * @since 0.1.0
     * @param begin  the element to begin at (inclusive)
     * @param end the element to end at (exclusive)
     * @return the new typed subarray
     */
    public JSFloat32Array subarray(int begin, int end) {
        return (JSFloat32Array)super.subarray(begin,end);
    }
    /**
     * JavaScript: TypedArray.prototype.subarray(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/subarray
     * @since 0.1.0
     * @param begin  the element to begin at (inclusive)
     * @return the new typed subarray
     */
    public JSFloat32Array subarray(int begin) {
        return (JSFloat32Array)super.subarray(begin);
    }

    private JSFloat32Array(JSFloat32Array superList, int leftBuffer, int rightBuffer) {
        super(superList,leftBuffer,rightBuffer,Float.class);
    }
    /**
     * @see java.util.List#subList(int, int)
     * @since 0.1.0
     */
    @Override @NonNull
    @SuppressWarnings("unchecked")
    public JSFloat32Array subList(final int fromIndex, final int toIndex) {
        if (fromIndex < 0 || toIndex > size() || fromIndex > toIndex) {
            throw new IndexOutOfBoundsException();
        }
        return new JSFloat32Array(this,fromIndex,size()-toIndex);
    }
}
