/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.jstest;

import org.junit.Test;
import org.liquidplayer.javascript.JSArrayBuffer;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSDataView;

import static org.junit.Assert.*;

public class JSDataViewTest {

    public void testConstructorsAndProperties(JSContext context) {
        JSArrayBuffer buffer = new JSArrayBuffer(context,8);
        JSDataView view = new JSDataView(buffer);
        assertEquals(buffer.byteLength(),view.buffer().byteLength());
        assertEquals(view.byteLength(),buffer.byteLength());
        assertEquals(view.byteOffset(),0);

        JSDataView view2 = new JSDataView(buffer,4);
        assertEquals(buffer.byteLength(),view2.buffer().byteLength());
        assertEquals(view2.byteLength(),4);
        assertEquals(view2.byteOffset(),4);

        JSDataView view3 = new JSDataView(buffer,4,2);
        assertEquals(buffer.byteLength(),view3.buffer().byteLength());
        assertEquals(view3.byteLength(),2);
        assertEquals(view3.byteOffset(),4);

        JSDataView view4 = new JSDataView(view3);
        assertTrue(view4.isStrictEqual(view3));
    }

    public void testFloat32(JSContext context) {
        JSDataView view = new JSDataView(new JSArrayBuffer(context,12));
        view.setFloat32(0,5.56f);
        view.setFloat32(4,6.87f,true);
        view.setFloat32(8,6.87f,false);

        assertEquals(Float.valueOf(5.56f), view.getFloat32(0));
        assertEquals(Float.valueOf(6.87f), view.getFloat32(4, true));
        assertEquals(Float.valueOf(6.87f), view.getFloat32(8, false));
        assertNotEquals(6.87f, view.getFloat32(4, false));
        assertNotEquals(6.87f, view.getFloat32(8, true));
    }

    public void testFloat64(JSContext context) {
        JSDataView view = new JSDataView(new JSArrayBuffer(context,24));
        view.setFloat64(0,5.5678);
        view.setFloat64(8,6.8765,true);
        view.setFloat64(16,6.8765,false);

        assertEquals(Double.valueOf(5.5678), view.getFloat64(0));
        assertEquals(Double.valueOf(6.8765), view.getFloat64(8,true));
        assertEquals(Double.valueOf(6.8765), view.getFloat64(16,false));
        assertNotEquals(6.8765, view.getFloat64(6,false));
        assertNotEquals(6.8765, view.getFloat64(16,true));
    }

    public void testInt32(JSContext context) {
        JSDataView view = new JSDataView(new JSArrayBuffer(context,12));
        view.setInt32(0,5);
        view.setInt32(4,6,true);
        view.setInt32(8,6,false);

        assertEquals(Integer.valueOf(5), view.getInt32(0));
        assertEquals(Integer.valueOf(6), view.getInt32(4,true));
        assertEquals(Integer.valueOf(6), view.getInt32(8,false));
        assertNotEquals(Integer.valueOf(6), view.getInt32(4,false));
        assertNotEquals(Integer.valueOf(6), view.getInt32(8,true));
    }

    public void testInt16(JSContext context) {
        JSDataView view = new JSDataView(new JSArrayBuffer(context,6));
        view.setInt16(0,(short)5);
        view.setInt16(2,(short)-6,true);
        view.setInt16(4,(short)-6,false);

        assertEquals(Short.valueOf((short)5), view.getInt16(0));
        assertEquals(Short.valueOf((short)-6), view.getInt16(2,true));
        assertEquals(Short.valueOf((short)-6), view.getInt16(4,false));
        assertNotEquals(Short.valueOf((short)-6), view.getInt16(2,false));
        assertNotEquals(Short.valueOf((short)-6), view.getInt16(4,true));
    }

    public void testInt8(JSContext context) {
        JSDataView view = new JSDataView(new JSArrayBuffer(context,2));
        view.setInt8(0,(byte)5);
        view.setInt8(1,(byte)-6);

        assertEquals(Byte.valueOf((byte)5), view.getInt8(0));
        assertEquals(Byte.valueOf((byte)-6), view.getInt8(1));
    }

    public void testUint32(JSContext context) {
        JSDataView view = new JSDataView(new JSArrayBuffer(context,12));
        view.setUint32(0,0xffffffffL);
        view.setUint32(4,6L,true);
        view.setUint32(8,6L,false);

        assertEquals(0xffffffffL,view.getUint32(0).longValue());
        assertEquals(6L, view.getUint32(4,true).longValue());
        assertEquals(6L, view.getUint32(8,false).longValue());
        assertNotEquals(6L, view.getUint32(4,false).longValue());
        assertNotEquals(6L, view.getUint32(8,true).longValue());
    }

    public void testUint16(JSContext context) {
        JSDataView view = new JSDataView(new JSArrayBuffer(context,6));
        view.setUint16(0,(short)0xfffe);
        view.setUint16(2,(short)6,true);
        view.setUint16(4,(short)6,false);

        assertEquals(Short.valueOf((short)0xfffe), view.getUint16(0));
        assertEquals(Short.valueOf((short)6), view.getUint16(2,true));
        assertEquals(Short.valueOf((short)6), view.getUint16(4,false));
        assertNotEquals(Short.valueOf((short)6), view.getUint16(2,false));
        assertNotEquals(Short.valueOf((short)6), view.getUint16(4,true));
    }

    public void testUint8(JSContext context) {
        JSDataView view = new JSDataView(new JSArrayBuffer(context,2));
        view.setUint8(0,(byte)0xfd);
        view.setUint8(1,(byte)6);

        assertEquals(Byte.valueOf((byte)0xfd), view.getUint8(0));
        assertEquals(Byte.valueOf((byte)6), view.getUint8(1));
    }

    @Test
    public void testConstructorsAndProperties() {
        JSContext context = new JSContext();
        testConstructorsAndProperties(context);
    }

    @Test
    public void testFloat32() {
        JSContext context = new JSContext();
        testFloat32(context);
    }

    @Test
    public void testFloat64() {
        JSContext context = new JSContext();
        testFloat64(context);
    }

    @Test
    public void testInt32() {
        JSContext context = new JSContext();
        testInt32(context);
    }

    @Test
    public void testInt16() {
        JSContext context = new JSContext();
        testInt16(context);
    }

    @Test
    public void testInt8() {
        JSContext context = new JSContext();
        testInt8(context);
    }

    @Test
    public void testUint32() {
        JSContext context = new JSContext();
        testUint32(context);
    }

    @Test
    public void testUint16() {
        JSContext context = new JSContext();
        testUint16(context);
    }

    @Test
    public void testUint8() {
        JSContext context = new JSContext();
        testUint8(context);
    }
}