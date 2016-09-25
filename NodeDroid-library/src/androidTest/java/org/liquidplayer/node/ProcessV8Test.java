package org.liquidplayer.node;

import android.support.test.InstrumentationRegistry;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.liquidplayer.v8.JSArrayBufferTest;
import org.liquidplayer.v8.JSArrayTest;
import org.liquidplayer.v8.JSContext;
import org.liquidplayer.v8.JSDataViewTest;
import org.liquidplayer.v8.JSDateTest;
import org.liquidplayer.v8.JSErrorTest;
import org.liquidplayer.v8.JSFloat32ArrayTest;
import org.liquidplayer.v8.JSFloat64ArrayTest;
import org.liquidplayer.v8.JSFunctionTest;
import org.liquidplayer.v8.JSInt16ArrayTest;
import org.liquidplayer.v8.JSInt32ArrayTest;
import org.liquidplayer.v8.JSInt8ArrayTest;
import org.liquidplayer.v8.JSIteratorTest;
import org.liquidplayer.v8.JSONTest;
import org.liquidplayer.v8.JSObjectPropertiesMapTest;
import org.liquidplayer.v8.JSObjectTest;
import org.liquidplayer.v8.JSRegExpTest;
import org.liquidplayer.v8.JSTypedArrayTest;
import org.liquidplayer.v8.JSUint16ArrayTest;
import org.liquidplayer.v8.JSUint32ArrayTest;
import org.liquidplayer.v8.JSUint8ArrayTest;
import org.liquidplayer.v8.JSUint8ClampedArrayTest;
import org.liquidplayer.v8.JSValueTest;

import java.util.concurrent.Semaphore;

import static org.junit.Assert.*;

public class ProcessV8Test {

    JSContext context = null;
    Process process = null;
    Semaphore processCompleted;

    @Before
    public void setUp() throws Exception {
        final Semaphore processStarted = new Semaphore(0);
        processCompleted = new Semaphore(0);

        new Process(InstrumentationRegistry.getContext(),"_",new Process.EventListener() {
            @Override
            public void onProcessStart(final Process proc, final JSContext ctx) {
                context = ctx;
                process = proc;
                // Don't let the process die before we run the test
                process.keepAlive();
                processStarted.release();
            }

            @Override
            public void onProcessExit(Process process, int exitCode) {
                processCompleted.release();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {}

            @Override
            public void onProcessFailed(Process process, Exception error) {

            }
        });

        // Hang out here until the process and promise are ready
        processStarted.acquire();
    }

    /* JSArrayBufferTest */

    @Test
    public void testGetJSObject() throws Exception {
        new JSArrayBufferTest().testGetJSObject(context);
    }

    @Test
    public void testByteLength() throws Exception {
        new JSArrayBufferTest().testByteLength(context);
    }

    @Test
    public void testIsView() throws Exception {
        new JSArrayBufferTest().testIsView(context);
    }

    @Test
    public void testSlice() throws Exception {
        new JSArrayBufferTest().testSlice(context);
    }

    /* JSArrayTest */

    @org.junit.Test
    public void testJSArrayConstructors() throws Exception {
        new JSArrayTest().testJSArrayConstructors(context);
    }

    @org.junit.Test
    public void testJSArrayListMethods() throws Exception {
        new JSArrayTest().testJSArrayListMethods(context);
    }

    @org.junit.Test // FIXME
    public void testJSArrayJSMethods() throws Exception {
        new JSArrayTest().testJSArrayJSMethods(context);
    }

    /* JSDataViewTest */

    @Test
    public void testConstructorsAndProperties() throws Exception {
        new JSDataViewTest().testConstructorsAndProperties(context);
    }

    @Test
    public void testFloat32() throws Exception {
        new JSDataViewTest().testFloat32(context);
    }

    @Test
    public void testFloat64() throws Exception {
        new JSDataViewTest().testFloat64(context);
    }

    @Test
    public void testInt32() throws Exception {
        new JSDataViewTest().testInt32(context);
    }

    @Test
    public void testInt16() throws Exception {
        new JSDataViewTest().testInt16(context);
    }

    @Test
    public void testInt8() throws Exception {
        new JSDataViewTest().testInt8(context);
    }

    @Test
    public void testUint32() throws Exception {
        new JSDataViewTest().testUint32(context);
    }

    @Test
    public void testUint16() throws Exception {
        new JSDataViewTest().testUint16(context);
    }

    @Test
    public void testUint8() throws Exception {
        new JSDataViewTest().testUint8(context);
    }

    /* JSDateTest */

    @Test
    public void testConstructors() throws Exception {
        JSDateTest t = new JSDateTest();
        assertNotNull(context);
        t.setUp(context);
        t.testConstructors();
    }

    @Test
    public void testMethods() throws Exception {
        JSDateTest t = new JSDateTest();
        t.setUp(context);
        t.testMethods();
    }

    @Test
    public void testGetters() throws Exception {
        JSDateTest t = new JSDateTest();
        t.setUp(context);
        t.testGetters();
    }

    @Test
    public void testSetters() throws Exception {
        JSDateTest t = new JSDateTest();
        t.setUp(context);
        t.testSetters();
    }

    @Test
    public void testUTC() throws Exception {
        JSDateTest t = new JSDateTest();
        t.setUp(context);
        t.testUTC();
    }

    @Test
    public void testConversionGetter() throws Exception {
        JSDateTest t = new JSDateTest();
        t.setUp(context);
        t.testConversionGetter();
    }

    /* JSErrorTest */

    @Test
    public void TestJSErrorAndJSException() throws Exception {
        new JSErrorTest().TestJSErrorAndJSException(context);
    }

    /* Typed array tests */

    @Test
    public void testJSTypedArray() throws Exception {
        new JSTypedArrayTest().testJSTypedArray(context);
    }

    @Test
    public void testJSFloat32Array() throws Exception {
        JSFloat32ArrayTest t = new JSFloat32ArrayTest();
        t.setUp(context);
        t.testJSFloat32Array();
    }

    @Test
    public void testJSFloat64Array() throws Exception {
        JSFloat64ArrayTest t = new JSFloat64ArrayTest();
        t.setUp(context);
        t.testJSFloat64Array();
    }

    @Test
    public void testJSInt32Array() throws Exception {
        JSInt32ArrayTest t = new JSInt32ArrayTest();
        t.setUp(context);
        t.testJSInt32Array();
    }

    @Test
    public void testJSInt16Array() throws Exception {
        JSInt16ArrayTest t = new JSInt16ArrayTest();
        t.setUp(context);
        t.testJSInt16Array();
    }

    @Test
    public void testJSInt8Array() throws Exception {
        JSInt8ArrayTest t = new JSInt8ArrayTest();
        t.setUp(context);
        t.testJSInt8Array();
    }

    @Test
    public void testJSUint32Array() throws Exception {
        JSUint32ArrayTest t = new JSUint32ArrayTest();
        t.setUp(context);
        t.testJSUint32Array();
    }

    @Test
    public void testJSUint16Array() throws Exception {
        JSUint16ArrayTest t = new JSUint16ArrayTest();
        t.setUp(context);
        t.testJSUint16Array();
    }

    @Test
    public void testJSUint8Array() throws Exception {
        JSUint8ArrayTest t = new JSUint8ArrayTest();
        t.setUp(context);
        t.testJSUint8Array();
    }

    @Test
    public void testJSUint8ClampedArray() throws Exception {
        JSUint8ClampedArrayTest t = new JSUint8ClampedArrayTest();
        t.setUp(context);
        t.testJSUint8ClampedArray();
    }

    /* JSFunctionTest */

    @org.junit.Test
    public void testJSFunctionConstructors() throws Exception {
        new JSFunctionTest().testJSFunctionConstructors(context);
    }

    // FIXME
    @org.junit.Test
    public void testJSFunctionCallback() throws Exception {
        new JSFunctionTest().testJSFunctionCallback(context);
    }

    @org.junit.Test
    public void testExceptionCases() throws Exception {
        new JSFunctionTest().testExceptionCases(context);
    }

    /* JSIteratorTest */

    @Test
    public void testJSIterator() throws Exception {
        new JSIteratorTest().testJSIterator(context);
    }

    /* JSObjectPropertiesMapTest */

    @Test
    public void testJSMapConstructors() throws Exception {
        new JSObjectPropertiesMapTest().testJSMapConstructors(context);
    }

    @Test
    public void testJSMapMethods() throws Exception {
        new JSObjectPropertiesMapTest().testJSMapMethods(context);
    }

    /* JSObjectTest */

    @org.junit.Test
    public void testJSObjectConstructors() throws Exception {
        new JSObjectTest().testJSObjectConstructors(context);
    }

    @org.junit.Test
    public void testJSObjectProperties() throws Exception {
        new JSObjectTest().testJSObjectProperties(context);
    }

    @org.junit.Test
    public void testJSObjectTesters() throws Exception {
        new JSObjectTest().testJSObjectTesters(context);
    }

    /* JSONTest */

    @Test
    public void testStringify() throws Exception {
        JSONTest t = new JSONTest();
        t.setUp(context);
        t.testStringify();
    }

    @Test
    public void testStringify1() throws Exception {
        JSONTest t = new JSONTest();
        t.setUp(context);
        t.testStringify1();
    }

    @Test
    public void testParse() throws Exception {
        JSONTest t = new JSONTest();
        t.setUp(context);
        t.testParse();
    }

    /* JSRegExpTest */

    @Test
    public void testJSRegExp() throws Exception {
        new JSRegExpTest().testJSRegExp(context);
    }

    /* JSValueTest */

    @org.junit.Test
    public void testJSValueConstructors() throws Exception {
        new JSValueTest().testJSValueConstructors(context);
    }

    @org.junit.Test
    public void testJSValueTesters() throws Exception {
        new JSValueTest().testJSValueTesters(context);
    }

    @org.junit.Test
    public void testJSValueComparators() throws Exception {
        new JSValueTest().testJSValueComparators(context);
    }

    @org.junit.Test
    public void testJSValueGetters() throws Exception {
        new JSValueTest().testJSValueGetters(context);
    }

    @After
    public void shutDown() throws Exception {
        // Mark the process as done and wait until the process shuts down
        process.letDie();
        processCompleted.acquire();
        context = null;
    }
}