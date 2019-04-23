package org.liquidplayer.jstest;

import org.junit.Test;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSPromise;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

public class JSPromiseTest {

    public void testPromiseResolve(JSContext context) throws InterruptedException {
        final CountDownLatch cdl = new CountDownLatch(1);

        final String test = "" +
                "async function f() {" +
                "  try {"+
                "    let result = await promiseMe();"+
                "    alertResult(result);"+
                "  } catch (err) {"+
                "    alertResult(-1);"+
                "  }"+
                "}"+
                "f();";

        class ResultContainer {
            private Integer result;
        }
        final ResultContainer container = new ResultContainer();
        container.result = 0;

        JSFunction promiseMe = new JSFunction(context, "promiseMe") {
            @SuppressWarnings("unused")
            public JSPromise promiseMe() {
                final JSPromise promise = new JSPromise(context);
                new Thread() {
                    @Override
                    public void run() {
                        try {
                            Thread.sleep(500);
                        } catch (InterruptedException e) {
                            fail();
                        }
                        promise.resolve(42);
                    }
                }.start();
                return promise;
            }
        };

        JSFunction alertResult = new JSFunction(context, "alertResult") {
            @SuppressWarnings("unused")
            public void alertResult(Integer result) {
                container.result = result;
                cdl.countDown();
            }
        };

        context.property("promiseMe", promiseMe);
        context.property( "alertResult", alertResult );
        context.evaluateScript(test);
        assertTrue(cdl.await(10, TimeUnit.SECONDS));
        assertEquals(42, (long)container.result);
    }

    public void testPromiseReject(JSContext context) throws InterruptedException {
        final CountDownLatch cdl = new CountDownLatch(1);

        final String test = "" +
                "async function f() {" +
                "  try {"+
                "    let result = await rejectMe();"+
                "    alertResult(result);"+
                "  } catch (err) {"+
                "    alertResult(-1);"+
                "  }"+
                "}"+
                "f();";

        class ResultContainer {
            private Integer result;
        }
        final ResultContainer container = new ResultContainer();
        container.result = 0;

        JSFunction rejectMe = new JSFunction(context, "rejectMe") {
            @SuppressWarnings("unused")
            public JSPromise rejectMe() {
                final JSPromise promise = new JSPromise(context);
                new Thread() {
                    @Override
                    public void run() {
                        try {
                            Thread.sleep(500);
                        } catch (InterruptedException e) {
                            fail();
                        }
                        promise.reject();
                    }
                }.start();
                return promise;
            }
        };

        JSFunction alertResult = new JSFunction(context, "alertResult") {
            @SuppressWarnings("unused")
            public void alertResult(Integer result) {
                container.result = result;
                cdl.countDown();
            }
        };

        context.property("rejectMe", rejectMe);
        context.property( "alertResult", alertResult );
        context.evaluateScript(test);
        assertTrue(cdl.await(10, TimeUnit.SECONDS));
        assertEquals(-1, (long)container.result);
    }

    @Test
    public void testPromiseResolve() throws InterruptedException {
        JSContext context = new JSContext();
        testPromiseResolve(context);
    }

    @Test
    public void testPromiseReject() throws InterruptedException {
        JSContext context = new JSContext();
        testPromiseReject(context);
    }

}
