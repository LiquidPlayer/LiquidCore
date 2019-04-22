/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;
import android.util.LongSparseArray;

import java.lang.ref.WeakReference;
import java.lang.reflect.Method;
import java.util.concurrent.Semaphore;

/**
 * Wraps a JavaScript context
 */
public class JSContext extends JSObject {

    static {
        System.loadLibrary("node");
        System.loadLibrary("liquidcore");
    }

    static void init() {
    }

    public void sync(final Runnable runnable) {
        if (isOnThread()) {
            runnable.run();
        } else {
            final Semaphore sempahore = new Semaphore(0);
            Runnable syncRunner = new Runnable() {
                @Override
                public void run() {
                    runnable.run();
                    sempahore.release();
                }
            };
            getGroup().schedule(syncRunner);
            sempahore.acquireUninterruptibly();
        }
    }

    private boolean isOnThread() {
        return ctxRef == null || getGroup().isOnThread();
    }

    /**
     * Returns the JSC context reference if this context was created using JSC.
     */
    @SuppressWarnings("unused")
    public long getJSCContext() {
        return 0L;
    }

    private JSContextGroup contextGroup;

    /**
     * Object interface for handling JSExceptions.
     * @since 0.1.0
     */
    public interface IJSExceptionHandler {
        /**
         * Implement this method to catch JSExceptions
         * @param exception caught exception
         * @since 0.1.0
         */
        void handle(JSException exception);
    }

    private JNIJSContext ctxRef;
    private IJSExceptionHandler exceptionHandler;

    protected JSContext(long ctxHandle, JSContextGroup group) {
        context = this;
        contextGroup = group;
        ctxRef = JNIJSContext.fromRef(ctxHandle);
        valueRef = ctxRef.getGlobalObject();
        addJSExports();
    }

    /**
     * Creates a new JavaScript context
     * @since 0.1.0
     */
    public JSContext() {
        this(new JSContextGroup());
    }

    private static String timer_code = "" +
            " let makeTimer = function(callback, millis) { \n"+
            "   if (!callback || typeof callback !== 'function') { \n"+
            "     throw new TypeError('[ERR_INVALID_CALLBACK]: Callback must be a function') \n"+
            "   } \n"+
            "\n"+
            "   let args = Array.from(arguments) \n"+
            "   args.shift() \n"+
            "   args.shift() \n"+
            "\n"+
            "   var timer = function() { \n"+
            "     if (!this.destroyed) { \n"+
            "       if (this.interval) { \n"+
            "         __NativeTimer__(this) \n"+
            "       } else { \n"+
            "         this.destroyed = true \n"+
            "       } \n"+
            "       this.callback.apply(this, this.args) \n"+
            "     } \n"+
            "   } \n"+
            "\n"+
            "   timer.callback = callback \n"+
            "   timer.args = args \n"+
            "   timer.millis = millis \n"+
            "   timer.destroyed = false \n"+
            "\n"+
            "   return timer \n"+
            " } \n"+
            "\n"+
            " function setTimeout(callback, millis) { \n"+
            "   var timer = makeTimer(...arguments) \n"+
            "   timer.interval = false \n"+
            "   __NativeTimer__(timer) \n"+
            "   return timer \n"+
            " } \n"+
            "\n"+
            " function setInterval(callback, millis) { \n"+
            "   var timer = makeTimer(...arguments) \n"+
            "   timer.interval = true \n"+
            "   __NativeTimer__(timer) \n"+
            "   return timer \n"+
            " }\n"+
            "\n"+
            " function clearTimer(timer) { \n"+
            "   if (timer && typeof timer === 'function') { \n"+
            "      timer.destroyed = true \n"+
            "   } \n"+
            " } \n"+
            "";

    /**
     * Creates a new JavaScript context in the context group 'inGroup'.
     * @param inGroup  The context group to create the context in
     * @since 0.1.0
     */
    public JSContext(final JSContextGroup inGroup) {
        context = this;
        contextGroup = inGroup;

        ctxRef = JNIJSContext.createContext(inGroup.groupRef());
        valueRef = ctxRef.getGlobalObject();
        addJSExports();

        context.property("__NativeTimer__", new JSFunction(context, "__NativeTimer__") {
            @SuppressWarnings("unused")
            public void __NativeTimer__(final JSFunction timer) {
                final int millis = timer.property("millis").toNumber().intValue();
                (new Thread() {
                    @Override public void run() {
                        try {
                            Thread.sleep(millis);
                        } catch (InterruptedException e) {
                            /* Do nothing */
                        } finally {
                            timer.call(timer);
                        }
                    }
                }).start();
            }
        });

        context.evaluateScript(timer_code, "InitTimer", 1);
    }
    /**
     * Creates a JavaScript context, and defines the global object with interface 'iface'.  This
     * object must implement 'iface'.  The methods in 'iface' will be exposed to the JavaScript environment.
     * @param iface  The interface to expose to JavaScript
     * @since 0.1.0
     */
    public JSContext(final Class<?> iface) {
        this(new JSContextGroup(), iface);
    }
    /**
     * Creates a JavaScript context in context group 'inGroup', and defines the global object
     * with interface 'iface'.  This object must implement 'iface'.  The methods in 'iface' will
     * be exposed to the JavaScript environment.
     * @param inGroup  The context group to create the context in
     * @param iface  The interface to expose to JavaScript
     * @since 0.1.0
     */
    public JSContext(final JSContextGroup inGroup, final Class<?> iface) {
        this(inGroup);
        Method[] methods = iface.getDeclaredMethods();
        for (Method m : methods) {
            JSObject f = new JSFunction(context, m, JSObject.class, context);
            property(m.getName(),f);
        }
    }

    /**
     * Sets the JS exception handler for this context.  Any thrown JSException in this
     * context will call the 'handle' method on this object.  The calling function will
     * return with an undefined value.
     * @param handler An object that implements 'IJSExceptionHandler'
     * @since 0.1.0
     */
    public void setExceptionHandler(IJSExceptionHandler handler) {
        exceptionHandler = handler;
    }

    /**
     * Clears a previously set exception handler.
     * @since 0.1.0
     */
    public void clearExceptionHandler() {
        exceptionHandler = null;
    }

    /**
     * If an exception handler is set, calls the exception handler, otherwise throws
     * the JSException.
     * @param exception The JSException to be thrown
     * @since 0.1.0
     */
    void throwJSException(JSException exception) {
        if (exceptionHandler == null) {
            throw exception;
        } else {
            // Before handling this exception, disable the exception handler.  If a JSException
            // is thrown in the handler, then it would recurse and blow the stack.  This way an
            // actual exception will get thrown.  If successfully handled, then turn it back on.
            IJSExceptionHandler temp = exceptionHandler;
            exceptionHandler = null;
            temp.handle(exception);
            exceptionHandler = temp;
        }
    }

    /**
     * Gets the context group to which this context belongs.
     * @return  The context group to which this context belongs
     */
    public JSContextGroup getGroup() {
        if (contextGroup == null) {
            JNIJSContextGroup g = ctxRef.getGroup();
            if (g == null) return null;
            contextGroup = new JSContextGroup(g);
        }
        return contextGroup;
    }

    /* package */ JNIJSContext ctxRef() {
        return ctxRef;
    }

    /**
     * Executes the JavaScript code in 'script' in this context
     * @param script  The code to execute
     * @param sourceURL  The URI of the source file, only used for reporting in stack trace (optional)
     * @param startingLineNumber  The beginning line number, only used for reporting in stack trace (optional)
     * @return  The return value returned by 'script'
     * @since 0.1.0
     */
    public JSValue evaluateScript(final @NonNull String script,
                                  final String sourceURL, final int startingLineNumber) {

        String src = (sourceURL==null) ? "<code>" : sourceURL;
        try {
            return new JSValue(ctxRef.evaluateScript(script, src, startingLineNumber), context);
        } catch (JNIJSException excp){
            throwJSException(new JSException(new JSValue(excp.exception, context)));
            return new JSValue(this);
        }
    }

    /**
     * Executes the JavaScript code in 'script' in this context
     * @param script  The code to execute
     * @return  The return value returned by 'script'
     * @since 0.1.0
     */
    public JSValue evaluateScript(String script) {
        return evaluateScript(script,null,0);
    }

    private final LongSparseArray<WeakReference<JSObject>> objects = new LongSparseArray<>();

    /**
     * Keeps a reference to an object in this context.  This is used so that only one
     * Java object instance wrapping a JavaScript object is maintained at any time.  This way,
     * local variables in the Java object will stay wrapped around all returns of the same
     * instance.  This is handled by JSObject, and should not need to be called by clients.
     * @param obj  The object with which to associate with this context
     */
    /* package */ void persistObject(JSObject obj) {
        objects.put(obj.canonical(), new WeakReference<>(obj));
        obj.persisted = true;
    }
    /**
     * Removes a reference to an object in this context.  Should only be used from the 'finalize'
     * object method.  This is handled by JSObject, and should not need to be called by clients.
     * @param obj the JSObject to dereference
     */
    /* package */ void finalizeObject(JSObject obj) {
        objects.remove(obj.canonical());
    }
    /**
     * Reuses a stored reference to a JavaScript object if it exists, otherwise, it creates the
     * reference.
     * @param objRef the JavaScript object reference
     * @return The JSObject representing the reference
     */
    /* package */ JSObject getObjectFromRef(final JNIJSObject objRef) {
        if (objRef.equals(valueRef())) {
            return this;
        }
        WeakReference<JSObject> wr = objects.get(objRef.canonicalReference());
        JSObject obj = null;
        if (wr != null) {
            obj = wr.get();
        }
        if (obj==null) {
            obj = new JSObject(objRef,this);
            if (obj.isArray()) {
                obj = new JSArray(objRef, this);
            } else if (obj.isTypedArray()) {
                obj = JSTypedArray.from(obj);
            } else if (obj.isFunction()) {
                obj = new JSFunction(objRef, this);
            }
        }
        return obj;
    }
}
