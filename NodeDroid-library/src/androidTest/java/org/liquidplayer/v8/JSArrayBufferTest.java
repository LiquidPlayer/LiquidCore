package org.liquidplayer.v8;

import org.junit.Test;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSArrayBufferTest {

    private final int BYTE_LENGTH = 100;

    @Test
    public void testGetJSObject() throws Exception {
        JSContext context = new JSContext();
        JSArrayBuffer arrayBuffer = new JSArrayBuffer(context,BYTE_LENGTH);
        JSObject obj = arrayBuffer.getJSObject();

        assertEquals(arrayBuffer.byteLength(),obj.property("byteLength").toNumber().intValue());
    }

    @Test
    public void testByteLength() throws Exception {
        JSContext context = new JSContext();
        JSArrayBuffer arrayBuffer = new JSArrayBuffer(context,BYTE_LENGTH);
        assertThat(arrayBuffer.byteLength(),is(BYTE_LENGTH));
    }

    @Test
    public void testIsView() throws Exception {
        JSContext context = new JSContext();

        assertFalse(JSArrayBuffer.isView(new JSArray<>(context,JSValue.class)));
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

    @Test
    public void testSlice() throws Exception {
        JSContext context = new JSContext();
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
}