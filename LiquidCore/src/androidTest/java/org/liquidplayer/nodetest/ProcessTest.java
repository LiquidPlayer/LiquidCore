/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.nodetest;

import android.content.Context;

import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.liquidplayer.javascript.JSArray;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.Process;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class ProcessTest {

    private final Object mutex = new Object();
    private int count = 0;
    private Context androidContext = InstrumentationRegistry.getInstrumentation().getContext();

    @Test
    public void multiProcessTest() throws Exception {

        final CountDownLatch cdl = new CountDownLatch(3);

        Process.EventListener listener = new Process.EventListener() {
            @Override
            public void onProcessStart(final Process process, final JSContext context) {
                assertTrue(process.isActive());
                JSFunction function = new JSFunction(context,"testme") {
                    @SuppressWarnings("unused")
                    public int testme(int in) {
                        return in + 1;
                    }
                };
                int mycount;
                synchronized (mutex) {
                    mycount = count++;
                }
                int incd = function.call(null,mycount).toNumber().intValue();
                assertEquals(incd,mycount+1);
                android.util.Log.d("multiProcessTest", "Got here");
            }

            @Override
            public void onProcessExit(final Process process, int exitCode) {
                assertFalse(process.isActive());
                android.util.Log.d("multiProcessTest", "Outta here");
                cdl.countDown();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {}

            @Override
            public void onProcessFailed(final Process process, Exception error) {
            }
        };
        new Process(androidContext,"_1", Process.kMediaAccessPermissionsRW,listener);
        new Process(androidContext,"_2", Process.kMediaAccessPermissionsRW,listener);
        new Process(androidContext,"_3", Process.kMediaAccessPermissionsRW,listener);

        // Hang out here until the processes all finish
        assertTrue(cdl.await(10L, TimeUnit.SECONDS));
    }

    @Test
    public void multiThreadTest() throws Exception {

        final CountDownLatch cdl = new CountDownLatch(1);

        new Process(androidContext,"_",
                Process.kMediaAccessPermissionsRW,new Process.EventListener() {
            @Override
            public void onProcessStart(final Process process, final JSContext context) {

                // Done let the process exit until our thread finishes
                final JSContextGroup.LoopPreserver preserver = process.keepAlive();

                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        context.property("foo", "bar");
                        JSValue value = context.evaluateScript("5 + 10");
                        assertEquals(value.toNumber().intValue(), 15);
                        assertEquals(context.property("foo").toString(), "bar");
                        context.property("dir_contents", new JSFunction(context,"dir_contents") {
                            @SuppressWarnings("unused")
                            public void dir_contents(JSValue err, JSArray<String> files) {
                                android.util.Log.d("dir_contents", files.toString());
                            }
                        });
                        context.evaluateScript(
                                "(function() {" +
                                        "  var fs = require('fs');" +
                                        "  fs.readdir('/home',dir_contents);" +
                                        "})();");
                        // ok, we're done here
                        preserver.release();
                    }
                }).start();
            }

            @Override
            public void onProcessExit(Process process, int exitCode) {
                cdl.countDown();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {}

            @Override
            public void onProcessFailed(Process process, Exception error) {

            }
        });

        // Hang out here until the process finishes
        assertTrue(cdl.await(10L, TimeUnit.SECONDS));
    }

    @Test
    public void testForceExit() throws Exception {
        final CountDownLatch cdl = new CountDownLatch(1);

        new Process(androidContext,"forceExitTest",
                Process.kMediaAccessPermissionsRW,new Process.EventListener() {
            @Override
            public void onProcessStart(final Process process, final JSContext context) {
                context.evaluateScript("setInterval(function(){console.log('tick');},1000);");
                context.evaluateScript("setTimeout(function(){process.exit(2);},500);");
            }

            @Override
            public void onProcessExit(Process process, int exitCode) {
                assertEquals(2, exitCode);
                cdl.countDown();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {}

            @Override
            public void onProcessFailed(Process process, Exception error) {

            }
        });

        // Hang out here until the process finishes
        assertTrue(cdl.await(10L, TimeUnit.SECONDS));

        Process.uninstall(androidContext, "forceExitTest",
                Process.UninstallScope.Global);

    }

    @Test
    public void eventListenerTest() throws InterruptedException {
        final CountDownLatch cdl = new CountDownLatch(2);
        final CountDownLatch cdl2 = new CountDownLatch(1);

        Process process = new Process(androidContext, "_",
                Process.kMediaAccessPermissionsWrite, new Process.EventListener() {
            @Override
            public void onProcessStart(Process process, JSContext context) {
                cdl.countDown();
            }

            @Override public void onProcessAboutToExit(Process process, int exitCode) {}
            @Override public void onProcessExit(Process process, int exitCode) {
                cdl2.countDown();
            }
            @Override public void onProcessFailed(Process process, Exception error) { error.printStackTrace(); }
        });

        final Process.EventListener newListener = new Process.EventListener() {
            @Override
            public void onProcessStart(Process process, JSContext context) {
                cdl.countDown();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {
                fail();
            }

            @Override
            public void onProcessExit(Process process, int exitCode) {
                fail();
            }

            @Override
            public void onProcessFailed(Process process, Exception error) {
                fail();
            }
        };

        process.addEventListener(newListener);

        assertTrue(cdl.await(10, TimeUnit.SECONDS));

        process.removeEventListener(newListener);

        assertTrue(cdl2.await(10, TimeUnit.SECONDS));
    }

    @Test
    public void testContext() throws Exception {
        final CountDownLatch started = new CountDownLatch(1);
        final CountDownLatch ended = new CountDownLatch(1);
        final JSContextGroup.LoopPreserver [] preserver = new JSContextGroup.LoopPreserver[1];
        final JSContext [] contexts = new JSContext[1];
        final Process process = new Process(androidContext, "__testContext__",
                Process.kMediaAccessPermissionsRW, new Process.EventListener() {
            @Override
            public void onProcessStart(Process process, JSContext context) {
                preserver[0] = process.keepAlive();
                contexts[0] = context;
                started.countDown();
            }
            @Override public void onProcessAboutToExit(Process process, int exitCode) {}
            @Override public void onProcessExit(Process process, int exitCode) { ended.countDown();}
            @Override public void onProcessFailed(Process process, Exception error) {}
        });

        started.await(10, TimeUnit.SECONDS);
        assertEquals(process.getJSContext(), contexts[0]);
        preserver[0].release();
        ended.await(10, TimeUnit.SECONDS);
    }

    @org.junit.After
    public void shutDown() {
        Runtime.getRuntime().gc();
    }

}
