/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.node;

import android.support.test.InstrumentationRegistry;

import org.junit.Test;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;

import java.util.concurrent.Semaphore;

import static org.junit.Assert.assertEquals;

public class PromiseTest {

    private int counter = 0;
    private Semaphore waitToFinish = new Semaphore(0);

    @Test
    public void testPromise()
    {
        final String code =
                "new Promise((resolve,reject)=>{" +
                "    console.log('Wait 1 second to resolve');" +
                "    setTimeout(resolve,1000);" +
                "})";


        Process.EventListener listener = new Process.EventListener() {
            @Override
            public void onProcessStart(final Process process, final JSContext context) {
                JSObject promise = context.evaluateScript(code).toObject();
                promise.property("then").toFunction()
                    .call(promise, new JSFunction(context, "then") {
                        public void then() {
                            android.util.Log.d("PromiseTest", "Calling then");
                            counter++;
                            assertEquals(2,counter);
                        }
                    }
                );
            }

            @Override
            public void onProcessExit(final Process process, int exitCode) {
                counter ++;
                waitToFinish.release();
            }

            @Override
            public void onProcessAboutToExit(Process process, int exitCode) {}

            @Override
            public void onProcessFailed(final Process process, Exception error) {
            }
        };

        counter++;
        Process process = new Process(
                InstrumentationRegistry.getContext(),
                "ProcessTest",
                Process.kMediaAccessPermissionsRW,
                listener
        );
        assertEquals(1, counter);
        android.util.Log.d("PromiseTest", "Got Process.");

        waitToFinish.acquireUninterruptibly();
        assertEquals(3, counter);
        android.util.Log.d("PromiseTest", "Process done");
    }

}
