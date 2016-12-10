//
// JSDataView.java
// AndroidJSCore project
//
// https://github.com/ericwlange/AndroidJSCore/
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
 * A wrapper class for a JavaScript DataView
 * See: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView
 * @since 3.0
 */
public class JSDataView extends JSObjectWrapper {

    /**
     * Creates a new DataView JavaScript object from ArrayBuffer 'buffer' and wraps it for Java
     * @param buffer  the JSArrayBuffer to create a DataView from
     * @since 3.0
     */
    public JSDataView(JSArrayBuffer buffer) {
        super(new JSFunction(buffer.getJSObject().getContext(),
                "_DataView",new String[] {"buffer"},

                "return new DataView(buffer);",
                null, 0).call(null,buffer).toObject());
    }
    /**
     * Creates a new DataView JavaScript object from ArrayBuffer 'buffer' starting from
     * 'byteOffset' and wraps it for Java
     * @param buffer  the JSArrayBuffer to create a DataView from
     * @param byteOffset the byte offset in 'buffer' to create the DataView from
     * @since 3.0
     */
    public JSDataView(JSArrayBuffer buffer, int byteOffset) {
        super(new JSFunction(buffer.getJSObject().getContext(),
                "_DataView1",new String[] {"buffer","byteOffset"},

                "return new DataView(buffer,byteOffset);",
                null, 0).call(null,buffer,byteOffset).toObject());
    }
    /**
     * Creates a new DataView JavaScript object from ArrayBuffer 'buffer' starting from
     * 'byteOffset' and wraps it for Java
     * @param buffer  the JSArrayBuffer to create a DataView from
     * @param byteOffset the byte offset in 'buffer' to create the DataView from
     * @param byteLength the length, in bytes, from 'byteOffset' to use for the DataView
     * @since 3.0
     */
    public JSDataView(JSArrayBuffer buffer, int byteOffset, int byteLength) {
        super(new JSFunction(buffer.getJSObject().getContext(),
                "_DataView2",new String[] {"buffer","byteOffset","byteLength"},

                "return new DataView(buffer,byteOffset,byteLength);",
                null, 0).call(null,buffer,byteOffset,byteLength).toObject());
    }

    /**
     * Treats an existing JSObject as a DataView.  It is up to the user to ensure the
     * underlying JSObject is actually an DataView.
     * @param view  The DataView JSObject to wrap
     * @since 3.0
     */
    public JSDataView(JSObject view) {
        super(view);
    }

    /**
     * JavasScript DataView.prototype.buffer, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/buffer
     * @return the JSArrayBuffer on which the DataView is built
     */
    public JSArrayBuffer buffer() {
        return new JSArrayBuffer(property("buffer").toObject());
    }

    /**
     * JavasScript DataView.prototype.byteLength, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/byteLength
     * @return the length in bytes of the DataView
     */
    public int byteLength() {
        return property("byteLength").toNumber().intValue();
    }

    /**
     * JavasScript DataView.prototype.byteOffset, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/byteOffset
     * @return the byte offset in the JSArrayBuffer where the DataView starts
     */
    public int byteOffset() {
        return property("byteOffset").toNumber().intValue();
    }

    /**
     * JavasScript DataView.prototype.getFloat32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getFloat32
     * @param byteOffset  the byte offset to read from
     * @param littleEndian  whether the value is stored with little endianness
     * @return the value at byteOffset
     */
    public Float getFloat32(int byteOffset, boolean littleEndian) {
        return property("getFloat32").toFunction().call(this,byteOffset,littleEndian)
                .toNumber().floatValue();
    }
    /**
     * JavasScript DataView.prototype.getFloat32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getFloat32
     * @param byteOffset  the byte offset to read from
     * @return the value at byteOffset
     */
    public Float getFloat32(int byteOffset) {
        return property("getFloat32").toFunction().call(this,byteOffset)
                .toNumber().floatValue();
    }

    /**
     * JavasScript DataView.prototype.setFloat32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setFloat32
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     * @param littleEndian  whether the value is to be stored with little endianness
     */
    public void setFloat32(int byteOffset, Float value, boolean littleEndian) {
        property("setFloat32").toFunction().call(this,byteOffset,value,littleEndian);
    }
    /**
     * JavasScript DataView.prototype.setFloat32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setFloat32
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     */
    public void setFloat32(int byteOffset, Float value) {
        property("setFloat32").toFunction().call(this,byteOffset,value);
    }

    /**
     * JavasScript DataView.prototype.getFloat64(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getFloat64
     * @param byteOffset  the byte offset to read from
     * @param littleEndian  whether the value is stored with little endianness
     * @return the value at byteOffset
     */
    public Double getFloat64(int byteOffset, boolean littleEndian) {
        return property("getFloat64").toFunction().call(this,byteOffset,littleEndian)
                .toNumber();
    }
    /**
     * JavasScript DataView.prototype.getFloat64(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getFloat64
     * @param byteOffset  the byte offset to read from
     * @return the value at byteOffset
     */
    public Double getFloat64(int byteOffset) {
        return property("getFloat64").toFunction().call(this,byteOffset)
                .toNumber();
    }

    /**
     * JavasScript DataView.prototype.setFloat64(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setFloat64
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     * @param littleEndian  whether the value is to be stored with little endianness
     */
    public void setFloat64(int byteOffset, Double value, boolean littleEndian) {
        property("setFloat64").toFunction().call(this,byteOffset,value,littleEndian);
    }
    /**
     * JavasScript DataView.prototype.setFloat64(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setFloat64
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     */
    public void setFloat64(int byteOffset, Double value) {
        property("setFloat64").toFunction().call(this,byteOffset,value);
    }

    /**
     * JavasScript DataView.prototype.getInt32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getInt32
     * @param byteOffset  the byte offset to read from
     * @param littleEndian  whether the value is stored with little endianness
     * @return the value at byteOffset
     */
    public Integer getInt32(int byteOffset, boolean littleEndian) {
        return property("getInt32").toFunction().call(this,byteOffset,littleEndian)
                .toNumber().intValue();
    }
    /**
     * JavasScript DataView.prototype.getInt32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getInt32
     * @param byteOffset  the byte offset to read from
     * @return the value at byteOffset
     */
    public Integer getInt32(int byteOffset) {
        return property("getInt32").toFunction().call(this,byteOffset)
                .toNumber().intValue();
    }

    /**
     * JavasScript DataView.prototype.setInt32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setInt32
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     * @param littleEndian  whether the value is to be stored with little endianness
     */
    public void setInt32(int byteOffset, Integer value, boolean littleEndian) {
        property("setInt32").toFunction().call(this,byteOffset,value,littleEndian);
    }
    /**
     * JavasScript DataView.prototype.setInt32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setInt32
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     */
    public void setInt32(int byteOffset, Integer value) {
        property("setInt32").toFunction().call(this,byteOffset,value);
    }

    /**
     * JavasScript DataView.prototype.getUint32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getUint32
     * @param byteOffset  the byte offset to read from
     * @param littleEndian  whether the value is stored with little endianness
     * @return the value at byteOffset
     */
    public Long getUint32(int byteOffset, boolean littleEndian) {
        return property("getUint32").toFunction().call(this,byteOffset,littleEndian)
                .toNumber().longValue();
    }
    /**
     * JavasScript DataView.prototype.getUint32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getUint32
     * @param byteOffset  the byte offset to read from
     * @return the value at byteOffset
     */
    public Long getUint32(int byteOffset) {
        return property("getUint32").toFunction().call(this,byteOffset)
                .toNumber().longValue();
    }

    /**
     * JavasScript DataView.prototype.setUint32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setUint32
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     * @param littleEndian  whether the value is to be stored with little endianness
     */
    public void setUint32(int byteOffset, Long value, boolean littleEndian) {
        property("setUint32").toFunction().call(this,byteOffset,value,littleEndian);
    }
    /**
     * JavasScript DataView.prototype.setUint32(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setUint32
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     */
    public void setUint32(int byteOffset, Long value) {
        property("setUint32").toFunction().call(this,byteOffset,value);
    }

    /**
     * JavasScript DataView.prototype.getInt16(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getInt16
     * @param byteOffset  the byte offset to read from
     * @param littleEndian  whether the value is stored with little endianness
     * @return the value at byteOffset
     */
    public Short getInt16(int byteOffset, boolean littleEndian) {
        return property("getInt16").toFunction().call(this,byteOffset,littleEndian)
                .toNumber().shortValue();
    }
    /**
     * JavasScript DataView.prototype.getInt16(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getInt16
     * @param byteOffset  the byte offset to read from
     * @return the value at byteOffset
     */
    public Short getInt16(int byteOffset) {
        return property("getInt16").toFunction().call(this,byteOffset)
                .toNumber().shortValue();
    }

    /**
     * JavasScript DataView.prototype.setInt16(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setInt16
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     * @param littleEndian  whether the value is to be stored with little endianness
     */
    public void setInt16(int byteOffset, Short value, boolean littleEndian) {
        property("setInt16").toFunction().call(this,byteOffset,value,littleEndian);
    }
    /**
     * JavasScript DataView.prototype.setInt16(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setInt16
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     */
    public void setInt16(int byteOffset, Short value) {
        property("setInt16").toFunction().call(this,byteOffset,value);
    }

    /**
     * JavasScript DataView.prototype.getUint16(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getUint16
     * @param byteOffset  the byte offset to read from
     * @param littleEndian  whether the value is stored with little endianness
     * @return the value at byteOffset
     */
    public Short getUint16(int byteOffset, boolean littleEndian) {
        return property("getUint16").toFunction().call(this,byteOffset,littleEndian)
                .toNumber().shortValue();
    }
    /**
     * JavasScript DataView.prototype.getUint16(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getUint16
     * @param byteOffset  the byte offset to read from
     * @return the value at byteOffset
     */
    public Short getUint16(int byteOffset) {
        return property("getUint16").toFunction().call(this,byteOffset)
                .toNumber().shortValue();
    }

    /**
     * JavasScript DataView.prototype.setUint16(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setUint16
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     * @param littleEndian  whether the value is to be stored with little endianness
     */
    public void setUint16(int byteOffset, Short value, boolean littleEndian) {
        property("setUint16").toFunction().call(this,byteOffset,value,littleEndian);
    }
    /**
     * JavasScript DataView.prototype.setUint16(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setUint16
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     */
    public void setUint16(int byteOffset, Short value) {
        property("setUint16").toFunction().call(this,byteOffset,value);
    }

    /**
     * JavasScript DataView.prototype.getInt8(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getInt8
     * @param byteOffset  the byte offset to read from
     * @return the value at byteOffset
     */
    public Byte getInt8(int byteOffset) {
        return property("getInt8").toFunction().call(this,byteOffset)
                .toNumber().byteValue();
    }

    /**
     * JavasScript DataView.prototype.setInt8(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setInt8
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     */
    public void setInt8(int byteOffset, Byte value) {
        property("setInt8").toFunction().call(this,byteOffset,value);
    }

    /**
     * JavasScript DataView.prototype.getUint8(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/getUint8
     * @param byteOffset  the byte offset to read from
     * @return the value at byteOffset
     */
    public Byte getUint8(int byteOffset) {
        return property("getUint8").toFunction().call(this,byteOffset)
                .toNumber().byteValue();
    }

    /**
     * JavasScript DataView.prototype.setUint8(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView/setUint8
     * @param byteOffset  the byte offset to write to
     * @param value the value to store at 'byteOffset'
     */
    public void setUint8(int byteOffset, Byte value) {
        property("setUint8").toFunction().call(this,byteOffset,value);
    }

}
