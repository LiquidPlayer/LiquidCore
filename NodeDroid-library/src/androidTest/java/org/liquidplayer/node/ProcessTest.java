package org.liquidplayer.node;

import org.junit.Test;
import org.liquidplayer.v8.JSContext;
import org.liquidplayer.v8.JSFunction;
import org.liquidplayer.v8.JSValue;

import java.util.concurrent.Semaphore;

import static org.junit.Assert.*;

public class ProcessTest {

    final Object mutex = new Object();
    int count = 0;

    @Test
    public void multiProcessTest() throws Exception {

        final Semaphore semaphore = new Semaphore(-2);

        Process.EventListener listener = new Process.EventListener() {
            @Override
            public void onProcessStart(final Process process, final JSContext context) {
                assertNotNull(process.context);
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
            }

            @Override
            public void onProcessExit(final Process process, int exitCode) {
                assertNull(process.context);
                assertFalse(process.isActive());
                semaphore.release();
            }

            @Override
            public void onProcessFailed(final Process process, Exception error) {

            }
        };
        new Process(listener);
        new Process(listener);
        new Process(listener);

        // Hang out here until the processes all finish
        semaphore.acquire();

        assertTrue(true);
    }

    @Test
    public void multiThreadTest() throws Exception {

        final Semaphore semaphore = new Semaphore(0);

        new Process(new Process.EventListener() {
            @Override
            public void onProcessStart(final Process process, final JSContext context) {

                // First don't let the process die -- give us a second
                context.property("f_done", new JSFunction(context));
                context.evaluateScript("setTimeout(f_done,1000);");
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        context.property("foo", "bar");
                        JSValue value = context.evaluateScript("5 + 10");
                        assertEquals(value.toNumber().intValue(), 15);
                        assertEquals(context.property("foo").toString(), "bar");
                    }
                }).start();
            }

            @Override
            public void onProcessExit(Process process, int exitCode) {
                semaphore.release();
            }

            @Override
            public void onProcessFailed(Process process, Exception error) {

            }
        });

        // Hang out here until the process finishes
        semaphore.acquire();

        assertTrue(true);
    }

    @org.junit.After
    public void shutDown() {
        Runtime.getRuntime().gc();
    }

}
