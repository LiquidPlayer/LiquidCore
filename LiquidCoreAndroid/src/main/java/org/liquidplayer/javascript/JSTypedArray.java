//
// JSTypedArray.java
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

import java.util.List;

/**
 * A convenience base class for JavaScript typed arrays.  This is an abstract class, and is
 * subclassed by JSInt8Array, JSInt16Array, JSInt32Array, JSUint8Array, JSUint16Array,
 * JSUint32Array, JSUint8ClampedArray, JSFloat32Array, and JSFloat64Array
 * @since 0.1.0
 * @param <T> Parameterized type of array elements
 */
public abstract class JSTypedArray<T> extends JSBaseArray<T> {

    protected JSTypedArray(JSContext ctx, int length, String jsConstructor, Class<T> cls) {
        super(ctx,cls);
        JSFunction constructor = new JSFunction(context,"_" + jsConstructor,new String[] {"length"},
                "return new " + jsConstructor + "(length);",
                null, 0);
        JSValue newArray = constructor.call(null,length);
        valueRef = newArray.valueRef();
        addJSExports();
        context.persistObject(this);
    }

    protected JSTypedArray(JSTypedArray typedArray, String jsConstructor, Class<T> cls) {
        super(typedArray.context, cls);
        JSFunction constructor = new JSFunction(context,"_" + jsConstructor,new String[] {"tarr"},
                "return new " + jsConstructor + "(tarr);",
                null, 0);
        JSValue newArray = constructor.call(null,typedArray);
        valueRef = newArray.valueRef();
        addJSExports();
        context.persistObject(this);
    }

    protected JSTypedArray(JSContext ctx, Object object, String jsConstructor, Class<T> cls) {
        super(ctx,cls);
        context = ctx;
        JSFunction constructor = new JSFunction(context,"_" + jsConstructor,new String[] {"obj"},
                "return new " + jsConstructor + "(obj);",
                null, 0);
        JSValue newArray = constructor.call(null,object);
        valueRef = newArray.valueRef();
        addJSExports();
        context.persistObject(this);
    }

    protected JSTypedArray(JSArrayBuffer buffer, int byteOffset, int length, String jsConstructor,
                        Class<T> cls) {
        super(buffer.getJSObject().getContext(),cls);
        JSFunction constructor = new JSFunction(context,"_" + jsConstructor,
                new String[] {"buffer,byteOffset,length"},
                "return new " + jsConstructor + "(buffer,byteOffset,length);",
                null, 0);
        JSValue newArray = constructor.call(null,buffer.getJSObject(),byteOffset,length);
        valueRef = newArray.valueRef();
        addJSExports();
        context.persistObject(this);
    }
    protected JSTypedArray(JSArrayBuffer buffer, int byteOffset, String jsConstructor,
                        Class<T> cls) {
        super(buffer.getJSObject().getContext(),cls);
        JSFunction constructor = new JSFunction(context,"_" + jsConstructor,
                new String[] {"buffer,byteOffset"},
                "return new " + jsConstructor + "(buffer,byteOffset);",
                null, 0);
        JSValue newArray = constructor.call(null,buffer.getJSObject(),byteOffset);
        valueRef = newArray.valueRef();
        addJSExports();
        context.persistObject(this);
    }
    protected JSTypedArray(JSArrayBuffer buffer, String jsConstructor, Class<T> cls) {
        super(buffer.getJSObject().getContext(),cls);
        JSFunction constructor = new JSFunction(context,"_" + jsConstructor,
                new String[] {"buffer"},
                "return new " + jsConstructor + "(buffer);",
                null, 0);
        JSValue newArray = constructor.call(null,buffer.getJSObject());
        valueRef = newArray.valueRef();
        addJSExports();
        context.persistObject(this);
    }
    protected JSTypedArray(JNIJSObject objRef, JSContext ctx, Class<T> cls) {
        super(objRef,ctx,cls);
    }
    @SuppressWarnings("unchecked")
    protected JSTypedArray(JSTypedArray superList, int leftBuffer, int rightBuffer, Class<T> cls) {
        super(superList,leftBuffer,rightBuffer,cls);
    }

    /**
     * JavaScript: TypedArray.from(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/from
     * @param obj source object
     * @return a new typed array
     * @since 0.1.0
     */
    public static JSTypedArray from(JSObject obj) {
        JSTypedArray arr = null;
        if     (obj.isInt8Array())  arr = new JSInt8Array(obj.toObject().JNI(),obj.getContext());
        else if(obj.isUint8Array()) arr = new JSUint8Array(obj.toObject().JNI(),obj.getContext());
        else if(obj.isUint8ClampedArray()) arr = new JSUint8ClampedArray(obj.toObject().JNI(),obj.getContext());
        else if(obj.isInt16Array()) arr = new JSInt16Array(obj.toObject().JNI(),obj.getContext());
        else if(obj.isUint16Array())arr = new JSUint16Array(obj.toObject().JNI(),obj.getContext());
        else if(obj.isInt32Array()) arr = new JSInt32Array(obj.toObject().JNI(),obj.getContext());
        else if(obj.isUint32Array())arr = new JSUint32Array(obj.toObject().JNI(),obj.getContext());
        else if(obj.isFloat32Array())arr = new JSFloat32Array(obj.toObject().JNI(),obj.getContext());
        else if(obj.isFloat64Array())arr = new JSFloat64Array(obj.toObject().JNI(),obj.getContext());
        else {
            throw new JSException(obj.getContext(), "Object not a typed array");
        }
        return arr;
    }

    /**
     * JavaScript: TypedArray.prototype.buffer, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/buffer
     * @return the underlying ArrayBuffer of this typed array
     * @since 0.1.0
     */
    public JSArrayBuffer buffer() {
        return new JSArrayBuffer(property("buffer").toObject());
    }

    /**
     * JavaScript: TypedArray.prototype.buffer, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/byteLength
     * @return the length in bytes of the underlying ArrayBuffer
     * @since 0.1.0
     */
    public int byteLength() {
        return property("byteLength").toNumber().intValue();
    }

    /**
     * JavaScript: TypedArray.prototype.buffer, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/byteOffset
     * @return the byte offset of the typed array in the underlying ArrayBuffer
     * @since 0.1.0
     */
    public int byteOffset() {
        return property("byteOffset").toNumber().intValue();
    }

    @Override
    protected JSValue arrayElement(final int index) {
        JSFunction getElement = new JSFunction(context,"_getElement",new String[]{"thiz","index"},
                "return thiz[index]",
                null, 0);
        return getElement.call(null,this,index);
    }

    @Override
    protected void arrayElement(final int index, final T value) {
        JSFunction setElement = new JSFunction(context,"_setElement",
                new String[]{"thiz","index","value"},
                "thiz[index] = value",
                null, 0);
        setElement.call(null,this,index,value);
    }

    /**
     * Always throws UnsupportedOperationException.  Typed Arrays operate on a fixed
     * JSArrayBuffer.  Items cannot be added, inserted or removed, only modified.
     * @since 0.1.0
     * @param val  The value to add to the array
     * @return nothing
     * @throws UnsupportedOperationException always
     */
    @Override
    public boolean add(final T val) throws JSException {
        throw new UnsupportedOperationException();
    }

    @SuppressWarnings("unchecked")
    protected JSTypedArray<T> subarray(int begin, int end) {
        JSValue subarray = property("subarray").toFunction().call(this,begin,end).toObject();
        return (JSTypedArray<T>) subarray.toJSArray();
    }
    @SuppressWarnings("unchecked")
    protected JSTypedArray<T> subarray(int begin) {
        JSValue subarray = property("subarray").toFunction().call(this,begin).toObject();
        return (JSTypedArray<T>) subarray.toJSArray();
    }
}
