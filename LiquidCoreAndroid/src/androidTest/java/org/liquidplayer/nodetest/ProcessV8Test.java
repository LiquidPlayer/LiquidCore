/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.nodetest;

import android.support.test.InstrumentationRegistry;

import org.junit.Test;
import org.liquidplayer.jstest.JSArrayBufferTest;
import org.liquidplayer.jstest.JSArrayTest;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;
import org.liquidplayer.jstest.JSContextTest;
import org.liquidplayer.jstest.JSDataViewTest;
import org.liquidplayer.jstest.JSDateTest;
import org.liquidplayer.jstest.JSErrorTest;
import org.liquidplayer.jstest.JSFloat32ArrayTest;
import org.liquidplayer.jstest.JSFloat64ArrayTest;
import org.liquidplayer.jstest.JSFunctionTest;
import org.liquidplayer.jstest.JSInt16ArrayTest;
import org.liquidplayer.jstest.JSInt32ArrayTest;
import org.liquidplayer.jstest.JSInt8ArrayTest;
import org.liquidplayer.jstest.JSIteratorTest;
import org.liquidplayer.jstest.JSONTest;
import org.liquidplayer.jstest.JSObjectPropertiesMapTest;
import org.liquidplayer.jstest.JSObjectTest;
import org.liquidplayer.jstest.JSRegExpTest;
import org.liquidplayer.jstest.JSTypedArrayTest;
import org.liquidplayer.jstest.JSUint16ArrayTest;
import org.liquidplayer.jstest.JSUint32ArrayTest;
import org.liquidplayer.jstest.JSUint8ArrayTest;
import org.liquidplayer.jstest.JSUint8ClampedArrayTest;
import org.liquidplayer.jstest.JSValueTest;
import org.liquidplayer.node.Process;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class ProcessV8Test {

    abstract class Runner {
        abstract void run(JSContext context) throws Exception;
    }

    class V8Test {
        private JSContext context = null;
        private Process process = null;
        private CountDownLatch processCompleted;
        private volatile Boolean released = false;
        private JSContextGroup.LoopPreserver preserver = null;

        V8Test(Runner runnable) throws Exception {
            final CountDownLatch processStarted = new CountDownLatch(1);
            processCompleted = new CountDownLatch(1);

            new Process(InstrumentationRegistry.getContext(), "_",
                    Process.kMediaAccessPermissionsRW, new Process.EventListener() {
                @Override
                public void onProcessStart(final Process proc, final JSContext ctx) {
                    context = ctx;
                    process = proc;
                    // Don't let the process die before we run the test
                    preserver = process.keepAlive();
                    released = true;
                    processStarted.countDown();
                }

                @Override
                public void onProcessExit(Process process, int exitCode) {
                    processCompleted.countDown();
                    assertTrue(released);
                }

                @Override
                public void onProcessAboutToExit(Process process, int exitCode) {
                    assertTrue(released);
                }

                @Override
                public void onProcessFailed(Process process, Exception error) {
                    processStarted.countDown();
                    assertTrue(released);
                    fail();
                }
            });

            // Hang out here until the process and promise are ready
            assertTrue(processStarted.await(10L, TimeUnit.SECONDS));
            if (released) {
                runnable.run(context);
                preserver.release();
            }
            if (released) {
                assertTrue(processCompleted.await(10L, TimeUnit.SECONDS));
            }
        }
    }

    /* JSArrayBufferTest */

    @Test
    public void testGetJSObject() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSArrayBufferTest().testGetJSObject(context);
            }
        });
    }

    @Test
    public void testByteLength() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSArrayBufferTest().testByteLength(context);
            }
        });
    }

    @Test
    public void testIsView() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSArrayBufferTest().testIsView(context);
            }
        });
    }

    @Test
    public void testSlice() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSArrayBufferTest().testSlice(context);
            }
        });
    }

    /* JSArrayTest */

    @org.junit.Test
    public void testJSArrayConstructors() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSArrayTest().testJSArrayConstructors(context);
            }
        });
    }

    @org.junit.Test
    public void testJSArrayListMethods() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSArrayTest().testJSArrayListMethods(context);
            }
        });
    }

    @org.junit.Test // FIXME
    public void testJSArrayJSMethods() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSArrayTest().testJSArrayJSMethods(context);
            }
        });
    }

    /* JSDataViewTest */

    @Test
    public void testConstructorsAndProperties() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testConstructorsAndProperties(context);
            }
        });
    }

    @Test
    public void testFloat32() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testFloat32(context);
            }
        });
    }

    @Test
    public void testFloat64() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testFloat64(context);
            }
        });
    }

    @Test
    public void testInt32() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testInt32(context);
            }
        });
    }

    @Test
    public void testInt16() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testInt16(context);
            }
        });
    }

    @Test
    public void testInt8() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testInt8(context);
            }
        });
    }

    @Test
    public void testUint32() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testUint32(context);
            }
        });
    }

    @Test
    public void testUint16() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testUint16(context);
            }
        });
    }

    @Test
    public void testUint8() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSDataViewTest().testUint8(context);
            }
        });
    }

    /* JSDateTest */

    @Test
    public void testConstructors() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSDateTest t = new JSDateTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testConstructors();
            }
        });
    }

    @Test
    public void testMethods() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSDateTest t = new JSDateTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testMethods();
            }
        });
    }

    @Test
    public void testGetters() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSDateTest t = new JSDateTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testGetters();
            }
        });
    }

    @Test
    public void testSetters() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSDateTest t = new JSDateTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testSetters();
            }
        });
    }

    @Test
    public void testUTC() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSDateTest t = new JSDateTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testUTC();
            }
        });
    }

    @Test
    public void testConversionGetter() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSDateTest t = new JSDateTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testConversionGetter();
            }
        });
    }

    /* JSErrorTest */

    @Test
    public void TestJSErrorAndJSException() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSErrorTest().TestJSErrorAndJSException(context);
            }
        });
    }

    /* Typed array tests */

    @Test
    public void testJSTypedArray() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSTypedArrayTest().testJSTypedArray(context);
            }
        });
    }

    @Test
    public void testJSFloat32Array() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSFloat32ArrayTest t = new JSFloat32ArrayTest();
                t.setUp(context);
                t.testJSFloat32Array();
            }
        });
    }

    @Test
    public void testJSFloat64Array() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSFloat64ArrayTest t = new JSFloat64ArrayTest();
                t.setUp(context);
                t.testJSFloat64Array();
            }
        });
    }

    @Test
    public void testJSInt32Array() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSInt32ArrayTest t = new JSInt32ArrayTest();
                t.setUp(context);
                t.testJSInt32Array();
            }
        });
    }

    @Test
    public void testJSInt16Array() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSInt16ArrayTest t = new JSInt16ArrayTest();
                t.setUp(context);
                t.testJSInt16Array();
            }
        });
    }

    @Test
    public void testJSInt8Array() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSInt8ArrayTest t = new JSInt8ArrayTest();
                t.setUp(context);
                t.testJSInt8Array();
            }
        });
    }

    @Test
    public void testJSUint32Array() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSUint32ArrayTest t = new JSUint32ArrayTest();
                t.setUp(context);
                t.testJSUint32Array();
            }
        });
    }

    @Test
    public void testJSUint16Array() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSUint16ArrayTest t = new JSUint16ArrayTest();
                t.setUp(context);
                t.testJSUint16Array();
            }
        });
    }

    @Test
    public void testJSUint8Array() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSUint8ArrayTest t = new JSUint8ArrayTest();
                t.setUp(context);
                t.testJSUint8Array();
            }
        });
    }

    @Test
    public void testJSUint8ClampedArray() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                JSUint8ClampedArrayTest t = new JSUint8ClampedArrayTest();
                t.setUp(context);
                t.testJSUint8ClampedArray();
            }
        });
    }

    /* JSFunctionTest */

    @org.junit.Test
    public void testJSFunctionConstructors() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) throws Exception {
                new JSFunctionTest().testJSFunctionConstructors(context);
            }
        });
    }

    @org.junit.Test
    public void testJSFunctionCallback() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSFunctionTest().testJSFunctionCallback(context);
            }
        });
    }

    @org.junit.Test
    public void testExceptionCases() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) throws Exception {
                new JSFunctionTest().testExceptionCases(context);
            }
        });
    }

    @org.junit.Test
    public void issue15Test() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSFunctionTest().issue15Test(context);
            }
        });
    }


    /* JSIteratorTest */

    @Test
    public void testJSIterator() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSIteratorTest().testJSIterator(context);
            }
        });
    }

    /* JSObjectPropertiesMapTest */

    @Test
    public void testJSMapConstructors() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSObjectPropertiesMapTest().testJSMapConstructors(context);
            }
        });
    }

    @Test
    public void testJSMapMethods() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSObjectPropertiesMapTest().testJSMapMethods(context);
            }
        });
    }

    /* JSObjectTest */

    @org.junit.Test
    public void testJSObjectConstructors() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSObjectTest().testJSObjectConstructors(context);
            }
        });
    }

    @org.junit.Test
    public void testJSObjectProperties() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSObjectTest().testJSObjectProperties(context);
            }
        });
    }

    @org.junit.Test
    public void testJSObjectTesters() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSObjectTest().testJSObjectTesters(context);
            }
        });
    }

    @org.junit.Test
    public void issue31Test() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSObjectTest().issue31Test(context);
            }
        });
    }

    /* JSONTest */

    @Test
    public void testStringify() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSONTest t = new JSONTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testStringify();
            }
        });
    }

    @Test
    public void testStringify1() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSONTest t = new JSONTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testStringify1();
            }
        });
    }

    @Test
    public void testParse() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(final JSContext context) {
                JSONTest t = new JSONTest() {
                    @Override
                    protected JSContext getContext() {
                        return context;
                    }
                };
                t.testParse();
            }
        });
    }

    /* JSRegExpTest */

    @Test
    public void testJSRegExp() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSRegExpTest().testJSRegExp(context);
            }
        });
    }

    /* JSValueTest */

    @org.junit.Test
    public void testJSValueConstructors() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSValueTest().testJSValueConstructors(context);
            }
        });
    }

    @org.junit.Test
    public void testJSValueTesters() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSValueTest().testJSValueTesters(context);
            }
        });
    }

    @org.junit.Test
    public void testJSValueComparators() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSValueTest().testJSValueComparators(context);
            }
        });
    }

    @org.junit.Test
    public void testJSValueGetters() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) {
                new JSValueTest().testJSValueGetters(context);
            }
        });
    }

    @org.junit.Test
    public void testTerminate() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) throws InterruptedException {
                new JSContextTest().terminateTest(context);
            }
        });
    }

    @org.junit.Test
    public void scheduleTest() throws Exception {
        new V8Test(new Runner() {
            @Override
            public void run(JSContext context) throws InterruptedException {
                new JSContextTest().scheduleTest(context);
            }
        });
    }

}