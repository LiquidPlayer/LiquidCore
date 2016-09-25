package org.liquidplayer.node;

import android.support.test.InstrumentationRegistry;

import org.junit.Test;
import org.liquidplayer.v8.JSArray;
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
                semaphore.release();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {}

            @Override
            public void onProcessFailed(final Process process, Exception error) {
            }
        };
        new Process(InstrumentationRegistry.getContext(),"_1",listener);
        new Process(InstrumentationRegistry.getContext(),"_2",listener);
        new Process(InstrumentationRegistry.getContext(),"_3",listener);

        // Hang out here until the processes all finish
        semaphore.acquire();

        assertTrue(true);
    }

    @Test
    public void multiThreadTest() throws Exception {

        final Semaphore semaphore = new Semaphore(0);

        new Process(InstrumentationRegistry.getContext(),"_",new Process.EventListener() {
            @Override
            public void onProcessStart(final Process process, final JSContext context) {

                // Done let the process exit until our thread finishes
                process.keepAlive();

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
                        process.letDie();
                    }
                }).start();
            }

            @Override
            public void onProcessExit(Process process, int exitCode) {
                semaphore.release();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {}

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
