/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.jsctest;

import android.support.test.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;
import org.liquidplayer.node.Process;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class JSCTest {

    private class InNodeProcess implements Process.EventListener {

        final CountDownLatch processCompleted = new CountDownLatch(1);
        private final Runnable runnable;

        InNodeProcess(final Runnable runnable, String vm) {
            this.runnable = runnable;
            new Process(InstrumentationRegistry.getContext(),vm,
                    Process.kMediaAccessPermissionsRW,this);
        }

        @Override
        public void onProcessStart(final Process proc, final JSContext ctx) {
            group = ctx.getGroup();
            process = proc;
            runnable.run();
        }

        @Override
        public void onProcessAboutToExit(Process process, int exitCode) {
        }

        @Override
        public void onProcessExit(Process process, int exitCode) {
            processCompleted.countDown();
        }

        @Override
        public void onProcessFailed(Process process, Exception error) {

        }
    }

    private Exception exception = null;
    private JSContextGroup group = null;
    private Process process = null;
    private JSContextGroup.LoopPreserver preserver = null;
    @Before
    public void setUp() {
        exception = null;
        group = null;
        process = null;
    }

    @Test
    public void testJavaScriptCoreBridge() {
        JSC jsc = new JSC(null);
        assertEquals(0,jsc.testAPI());
    }

    @Test
    public void testJavaScriptCoreMiniDOM() {
        JSC jsc = new JSC(null);
        assertEquals(0,jsc.testMinidom());
    }

    @Test
    public void testJavaScriptCoreBridgeInNode() throws Exception {
        InNodeProcess p = new InNodeProcess(new Runnable() {
            @Override
            public void run() {
                try {
                    JSC jsc = new JSC(group);
                    assertEquals(0, jsc.testAPI());
                } catch (Exception e) {
                    exception = e;
                }
            }
        }, "_testapi");
        assertTrue(p.processCompleted.await(10L, TimeUnit.SECONDS));

        if (exception != null) throw exception;
    }

    @Test
    public void testJavaScriptCoreBridgeOutsideNode() throws Exception {
        final CountDownLatch ready = new CountDownLatch(1);
        InNodeProcess inp = new InNodeProcess(new Runnable() {
            @Override
            public void run() {
                preserver = process.keepAlive();
                ready.countDown();
            }
        }, "_testapi");
        // Wait until process is active
        assertTrue(ready.await(10L, TimeUnit.SECONDS));

        // Test outside of node thread
        JSC jsc = new JSC(group);
        assertEquals(0, jsc.testAPI());

        preserver.release();
        assertTrue(inp.processCompleted.await(10L, TimeUnit.SECONDS));
    }

    @Test
    public void testJavaScriptCoreMiniDOMInNode() throws Exception {
        InNodeProcess p = new InNodeProcess(new Runnable() {
            @Override
            public void run() {
                try {
                    JSC jsc = new JSC(group);
                    assertEquals(0, jsc.testMinidom());
                } catch (Exception e) {
                    exception = e;
                }
            }
        }, "_testapi");
        assertTrue(p.processCompleted.await(10L, TimeUnit.SECONDS));

        if (exception != null) throw exception;
    }

    @Test
    public void testJavaScriptCoreMiniDomOutsideNode() throws Exception {
        final CountDownLatch ready = new CountDownLatch(1);
        InNodeProcess inp = new InNodeProcess(new Runnable() {
            @Override
            public void run() {
                preserver = process.keepAlive();
                ready.countDown();
            }
        }, "_testapi");
        // Wait until process is active
        assertTrue(ready.await(10L, TimeUnit.SECONDS));

        // Test outside of node thread
        JSC jsc = new JSC(group);
        assertEquals(0, jsc.testMinidom());

        preserver.release();
        assertTrue(inp.processCompleted.await(10L, TimeUnit.SECONDS));
    }

}