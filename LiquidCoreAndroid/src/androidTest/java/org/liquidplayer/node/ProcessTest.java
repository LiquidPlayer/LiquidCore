//
// ProcessTest.java
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
package org.liquidplayer.node;

import android.support.test.InstrumentationRegistry;

import org.junit.Test;
import org.liquidplayer.javascript.JSArray;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSValue;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class ProcessTest {

    private final Object mutex = new Object();
    private int count = 0;

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
            }

            @Override
            public void onProcessExit(final Process process, int exitCode) {
                assertFalse(process.isActive());
                cdl.countDown();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {}

            @Override
            public void onProcessFailed(final Process process, Exception error) {
            }
        };
        new Process(InstrumentationRegistry.getContext(),"_1",
                Process.kMediaAccessPermissionsRW,listener);
        new Process(InstrumentationRegistry.getContext(),"_2",
                Process.kMediaAccessPermissionsRW,listener);
        new Process(InstrumentationRegistry.getContext(),"_3",
                Process.kMediaAccessPermissionsRW,listener);

        // Hang out here until the processes all finish
        assertTrue(cdl.await(10L, TimeUnit.SECONDS));
    }

    @Test
    public void multiThreadTest() throws Exception {

        final CountDownLatch cdl = new CountDownLatch(1);

        new Process(InstrumentationRegistry.getContext(),"_",
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

        new Process(InstrumentationRegistry.getContext(),"forceExitTest",
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

        Process.uninstall(InstrumentationRegistry.getContext(), "forceExitTest",
                Process.UninstallScope.Global);

    }

    @org.junit.After
    public void shutDown() {
        Runtime.getRuntime().gc();
    }

}
