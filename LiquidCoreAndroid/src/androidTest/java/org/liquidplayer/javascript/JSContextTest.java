//
// JSContextTest.java
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

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import org.junit.Test;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Semaphore;

import static org.junit.Assert.*;

public class JSContextTest {

    public interface JSContextInterface {
        @SuppressWarnings("unused")
        int func1();
    }
    public class JSContextClass extends JSContext implements JSContextInterface {
        public JSContextClass() {
            super(JSContextInterface.class);
        }
        @Override
        public int func1() {
            return 55;
        }
    }
    public class JSContextInGroup extends JSContext implements JSContextInterface {
        public JSContextInGroup(JSContextGroup inGroup) {
            super(inGroup, JSContextInterface.class);
        }
        @Override
        public int func1() {
            return property("testObject").toFunction().call().toNumber().intValue();
        }
    }

    @org.junit.Test
    public void testJSContextConstructor() throws Exception {
        JSContext context = new JSContext();

        context.property("test",10);
        assertTrue(context.property("test").toNumber().equals(10.0));

        JSContext context1 = new JSContextClass();
        JSValue ret = context1.evaluateScript("func1()");

        assertTrue(ret.toNumber().equals(55.0));

        JSContextGroup contextGroup = new JSContextGroup();
        JSContext context2 = new JSContext(contextGroup);
        JSContext context3 = new JSContext(contextGroup);
        context2.evaluateScript("var forty_two = 42; var cx2_func = function() { return forty_two; };");
        JSValue cx2_func = context2.property("cx2_func");

        context3.property("cx3_func", cx2_func);
        JSValue forty_two = context3.evaluateScript("cx3_func()");
        assertTrue(forty_two.toNumber().equals(42.0));

        JSContextInGroup context4 = new JSContextInGroup(contextGroup);
        context4.property("testObject", cx2_func);
        ret = context4.evaluateScript("func1()");
        assertTrue(ret.toNumber().equals(42.0));

        assertEquals(context2.getGroup(),context3.getGroup());
        assertEquals(context3.getGroup(),context4.getGroup());
        assertNotEquals(context4.getGroup(),null);
        assertNotEquals(context1.getGroup(),context2.getGroup());
    }

    private boolean excp;

    @org.junit.Test
    public void testJSContextExceptionHandler() throws Exception {
        JSContext context = new JSContext();
        try {
            context.property("does_not_exist").toFunction();
            assertTrue(false);
        } catch (JSException e) {
            assertTrue(true);
        }

        excp = false;
        context.setExceptionHandler(new JSContext.IJSExceptionHandler() {
            @Override
            public void handle(JSException e) {
                excp = !excp;
            }
        });
        try {
            context.property("does_not_exist").toFunction();
            assertTrue(excp);
        } catch (JSException e) {
            assertTrue(false);
        }

        context.clearExceptionHandler();
        try {
            context.property("does_not_exist").toFunction();
            assertTrue(false);
        } catch (JSException e) {
            // excp should still be true
            assertTrue(excp);
        }

        excp = false;
        final JSContext context2 = new JSContext();
        context2.setExceptionHandler(new JSContext.IJSExceptionHandler() {
            @Override
            public void handle(JSException e) {
                excp = !excp;
                // Raise another exception.  Should throw JSException
                context2.property("does_not_exist").toFunction();
            }
        });
        try {
            context2.property("does_not_exist").toFunction();
            assertTrue(false);
        } catch (JSException e) {
            assertTrue(excp);
        }
    }

    @org.junit.Test
    public void testJSContextEvaluateScript() throws Exception {
        final String script1 = "" +
                "var val1 = 1;\n" +
                "var val2 = 'foo';\n" +
                "does_not_exist(do_something);";
        String url = "http://liquidplayer.com/script1.js";

        JSContext context = new JSContext();

        try {
            context.evaluateScript(script1, url, 1);
            assertTrue(false);
        } catch (JSException e) {
            String stack = e.getError().toObject().property("stack").toString();
            String expected = "at " + url + ":4:";
            assertTrue(stack.contains(expected));
        }

        context.property("localv",69);
        JSValue val = context.evaluateScript("this.localv");
        assertTrue(val.toNumber().equals(69.0));
    }

    Exception thrownInMainThread = null;

    @org.junit.Test
    public void testMainThreadSupport() throws Exception {
        Handler handler = new Handler(Looper.getMainLooper());
        final Semaphore mutex = new Semaphore(0);
        handler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    JSContextTest test = new JSContextTest();
                    test.testJSContextConstructor();
                    test.testJSContextEvaluateScript();
                    test.testJSContextExceptionHandler();
                    test.testDeadReferences();
                } catch (Exception e) {
                    thrownInMainThread = e;
                } finally {
                    mutex.release();
                }
            }
        });
        mutex.acquireUninterruptibly();
        if (thrownInMainThread != null) throw thrownInMainThread;
    }

    @Test
    public void testDeadReferences() throws Exception {
        JSContext context = new JSContext();
        for (int i=0; i<200; i++) {
            new JSValue(context);
        }
        assertTrue(true);
    }

    private class ContextTest {

        Semaphore semaphore = new Semaphore(0);

        private JSContext thisContext;
        private final Map<Integer,JSContext> contextMap =
                Collections.synchronizedMap(new HashMap<Integer,JSContext>());
        private final Object mutex = new Object();

        ContextTest() {
        }

        void createContext() {

            JSContext context = new JSContext();
            context.setExceptionHandler(new JSExceptionHandler() {
                @Override
                public void handle(JSException exception) {
                    Log.e("Issue18", exception.getMessage());
                }
            });

            thisContext = context;
        }

        void callFunction() {

            new Thread() {
                @Override
                public void run() {
                    android.util.Log.d("Issue18", "THREAD TEST 1 ");
                    android.util.Log.d("Issue18", "THREAD TEST " + thisContext);
                    semaphore.release();
                }
            }.start();

        }

        public void createContext(int id) {

            JSContext context = new JSContext();
            context.setExceptionHandler(new JSExceptionHandler());

            try {
                synchronized (mutex) {
                    contextMap.put(id, context);
                }

            }
            catch (Exception ioe) {
                ioe.printStackTrace();
            }
        }


        public void callFunction(final int id) {

            ExecutorService service = Executors.newSingleThreadExecutor();
            Runnable runnable = new Runnable() {
                @Override
                public void run() {
                    synchronized (mutex) {
                        JSContext context = contextMap.get(id);
                        Log.d("Thread", "context : " + context.toString());
                    }
                    semaphore.release();
                }
            };
            service.execute(runnable);
        }

        class JSExceptionHandler implements JSContext.IJSExceptionHandler {
            @Override
            public void handle(JSException exception) {
                android.util.Log.e("Issue18", "Exception: " + exception.toString());
            }
        }

    }

    @Test
    public void Issue18Test() throws Exception {
        ContextTest contextTest = new ContextTest();
        contextTest.createContext();
        contextTest.callFunction();
        contextTest.semaphore.acquire();

        ContextTest contextTest2 = new ContextTest();
        contextTest2.createContext(12);
        for(int i =0; i < 10; i++) {
            contextTest2.callFunction(12);
        }
        contextTest2.semaphore.acquire();
    }

    @org.junit.After
    public void shutDown() {
    }
}