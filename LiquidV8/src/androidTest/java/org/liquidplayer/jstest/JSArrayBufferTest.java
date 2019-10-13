/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.jstest;

import org.junit.Test;
import org.liquidplayer.javascript.JSArray;
import org.liquidplayer.javascript.JSArrayBuffer;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSDataView;
import org.liquidplayer.javascript.JSFloat32Array;
import org.liquidplayer.javascript.JSInt32Array;
import org.liquidplayer.javascript.JSInt8Array;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSUint8Array;
import org.liquidplayer.javascript.JSValue;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSArrayBufferTest {

    private final int BYTE_LENGTH = 100;

    public void testGetJSObject(JSContext context) {
        JSArrayBuffer arrayBuffer = new JSArrayBuffer(context,BYTE_LENGTH);
        JSObject obj = arrayBuffer.getJSObject();

        assertEquals(arrayBuffer.byteLength(),obj.property("byteLength").toNumber().intValue());
    }

    public void testByteLength(JSContext context) {
        JSArrayBuffer arrayBuffer = new JSArrayBuffer(context,BYTE_LENGTH);
        assertThat(arrayBuffer.byteLength(),is(BYTE_LENGTH));
    }

    public void testIsView(JSContext context) {
        assertFalse(JSArrayBuffer.isView(new JSArray<>(context, JSValue.class)));
        assertFalse(JSArrayBuffer.isView(new JSObject(context)));
        assertFalse(JSArrayBuffer.isView(new JSValue(context,null)));
        assertFalse(JSArrayBuffer.isView(new JSValue(context)));
        assertFalse(JSArrayBuffer.isView(new JSArrayBuffer(context,10).getJSObject()));

        assertTrue(JSArrayBuffer.isView(new JSUint8Array(context,0)));
        assertTrue(JSArrayBuffer.isView(new JSFloat32Array(context,0)));
        assertTrue(JSArrayBuffer.isView(new JSInt8Array(context,10).subarray(0, 3)));

        JSArrayBuffer buffer = new JSArrayBuffer(context,2);
        JSDataView dv = new JSDataView(buffer);
        assertTrue(JSArrayBuffer.isView(dv));
    }

    public void testSlice(JSContext context) {
        JSArrayBuffer buf1 = new JSArrayBuffer(context, 8);
        new JSInt32Array(buf1).set(0, 42);
        JSArrayBuffer buf2 = buf1.slice(0);

        assertEquals(buf1.byteLength(), buf2.byteLength());
        assertThat(new JSInt32Array(buf1).get(0), is(42));
        assertThat(new JSInt32Array(buf2).get(0), is(42));

        new JSInt32Array(buf1).set(0, 69);
        assertThat(new JSInt32Array(buf1).get(0), is(69));
        assertThat(new JSInt32Array(buf2).get(0), is(42));

        JSArrayBuffer buf3 = buf2.slice(0, 4);
        assertThat(buf3.byteLength(), is(4));
        assertNotEquals(buf3.byteLength(), buf2.byteLength());
        assertThat(new JSInt32Array(buf3).get(0), is(42));
    }

    @Test
    public void testGetJSObject() {
        JSContext context = new JSContext();
        testGetJSObject(context);
    }

    @Test
    public void testByteLength() {
        JSContext context = new JSContext();
        testByteLength(context);
    }

    @Test
    public void testIsView() {
        JSContext context = new JSContext();
        testIsView(context);
    }

    @Test
    public void testSlice() {
        JSContext context = new JSContext();
        testSlice(context);
    }
}