//
// JSFunction.java
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

import android.support.annotation.NonNull;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;

/**
 * A JavaScript function object.
 * @since 3.0
 *
 */
@SuppressWarnings("JniMissingFunction")
public class JSFunction extends JSObject {

    private abstract class JNIReturnClass implements Runnable {
        JNIReturnObject jni;
    }

    /**
     * Creates a JavaScript function that takes parameters 'parameterNames' and executes the
     * JS code in 'body'.
     *
     * @param ctx                The JSContext in which to create the function
     * @param name               The name of the function
     * @param parameterNames     A String array containing the names of the parameters
     * @param body               The JavaScript code to execute in the function
     * @param sourceURL          The URI of the source file, only used for reporting in stack trace (optional)
     * @param startingLineNumber The beginning line number, only used for reporting in stack trace (optional)
     * @since 2.2
     */
    public JSFunction(JSContext ctx, final @NonNull String name, final @NonNull String[] parameterNames,
                      final @NonNull String body, final String sourceURL, final int startingLineNumber)
    {
        context = ctx;
        context.sync(new Runnable() {
            @Override
            public void run() {
                String func = "(function(";
                for (int i=0; i<parameterNames.length; i++) {
                    func += parameterNames[i];
                    if (i<parameterNames.length-1) {
                        func += ",";
                    }
                }
                func += "){" + body + "})";
                final String function = func;

                JNIReturnObject jni = makeFunction(
                        context.ctxRef(),
                        name,
                        function,
                        (sourceURL==null) ? "<anonymous>" : sourceURL,
                        startingLineNumber);
                valueRef = testException(jni);
                addJSExports();
            }
        });
        context.persistObject(this);
    }

    /**
     * Creates a JavaScript function that takes parameters 'parameterNames' and executes the
     * JS code in 'body'.
     *
     * @param ctx                The JSContext in which to create the function
     * @param name               The name of the function
     * @param parameterNames     A String array containing the names of the parameters
     * @param body               The JavaScript code to execute in the function
     */
    public JSFunction(JSContext ctx, final @NonNull String name, final @NonNull String body,
                      final String ... parameterNames)
    {
        this(ctx,name,parameterNames,body,null,1);
    }

    private long testException(JNIReturnObject jni) {
        if (jni.exception!=0) {
            context.throwJSException(new JSException(new JSValue(jni.exception, context)));
            return(make(context.ctxRef()));
        } else {
            return jni.reference;
        }
    }

    /**
     * Creates a new function object which calls method 'method' on this Java object.
     * Assumes the 'method' exists on this object and will throw a JSException if not found.  If
     * 'new' is called on this function, it will create a new 'instanceClass' instance.
     * In JS:
     * <pre>{@code
     * var f = function(a) { ... };
     * }
     * </pre>
     *
     * Example:
     * <pre>{@code
     *
     * public class FunctionObject extends JSObject {
     *     void function(int x) {
     *         getThis().property("varx",x);
     *     }
     * }
     *
     * public class MyFunc extends JSFunction {
     *     MyFunc(JSContext ctx) {
     *         super(ctx,
     *              FunctionObject.class.getMethod("function",int.class), // will call method 'function'
     *              JSObject.class                                // calling 'new' will create a JSObject
     *              new FunctionObject(ctx)                       // function will be called on FunctionObject
     *         );
     *     }
     * }
     * }
     * </pre>
     *
     * @param ctx    The JSContext to create the object in
     * @param method The method to invoke
     * @param instanceClass The class to be created on 'new' call
     * @param invokeObject  The object on which to invoke the method
     * @since 3.0
     */
    public JSFunction(JSContext ctx,
                      final Method method,
                      final Class<? extends JSObject> instanceClass,
                      JSObject invokeObject) {
        context = ctx;
        this.method = method;
        this.invokeObject = (invokeObject==null) ? this: invokeObject;
        context.sync(new Runnable() {
            @Override
            public void run() {
                valueRef = makeFunctionWithCallback(context.ctxRef(), method.getName());
                subclass = instanceClass;
                addJSExports();
            }
        });

        context.persistObject(this);
        context.zombies.add(this);
    }
    /**
     * Creates a new function object which calls method 'method' on this Java object.
     * Assumes the 'method' exists on this object and will throw a JSException if not found.  If
     * 'new' is called on this function, it will create a new 'instanceClass' instance.
     * In JS:
     * <pre>{@code
     * var f = function(a) { ... };
     * }
     * </pre>
     *
     * Example:
     * <pre>{@code
     * public class MyFunc extends JSFunction {
     *     MyFunc(JSContext ctx) {
     *         super(ctx,
     *              MyFunc.class.getMethod("function",int.class), // will call method 'function'
     *              JSObject.class                                // calling 'new' will create a JSObject
     *         );
     *     }
     *     void function(int x) {
     *         getThis().property("varx",x);
     *     }
     * }
     * }
     * </pre>
     *
     *
     * @param ctx    The JSContext to create the object in
     * @param method The method to invoke
     * @param instanceClass The class to be created on 'new' call
     * @since 3.0
     */
    public JSFunction(JSContext ctx,
                      final Method method,
                      final Class<? extends JSObject> instanceClass) {
        this(ctx,method,instanceClass,null);
    }
    /**
     * Creates a new function object which calls method 'method' on this Java object.
     * Assumes the 'method' exists on this object and will throw a JSException if not found.  If
     * 'new' is called on this function, it will create a new JSObject instance.
     * In JS:
     * <pre>{@code
     * var f = function(a) { ... };
     * }
     * </pre>
     *
     * Example:
     * <pre>{@code
     * public class MyFunc extends JSFunction {
     *     MyFunc(JSContext ctx) {
     *         super(ctx,
     *              MyFunc.class.getMethod("function",int.class), // will call method 'function'
     *              JSObject.class                                // calling 'new' will create a JSObject
     *         );
     *     }
     *     void function(int x) {
     *         getThis().property("varx",x);
     *     }
     * }
     * }
     * </pre>
     *
     *
     * @param ctx    The JSContext to create the object in
     * @param method The method to invoke
     * @since 3.0
     */
    public JSFunction(JSContext ctx,
                      final Method method) {
        this(ctx,method,JSObject.class);
    }
    /**
     * Creates a new function which basically does nothing.
     * In JS:
     * <pre>{@code
     * var f = function() {};
     * }
     * </pre>
     *
     * Example:
     * <pre>{@code
     * JSFunction f = new JSFunction(context);
     * }
     * </pre>
     *
     *
     * @param ctx    The JSContext to create the object in
     * @since 3.0
     */
    public JSFunction(JSContext ctx) {
        this(ctx,(String)null);
    }

    /**
     * Creates a new function object which calls method 'methodName' on this Java object.
     * Assumes the 'methodName' method exists on this object and will throw a JSException if not found.  If
     * 'new' is called on this function, it will create a new 'instanceClass' instance.
     * In JS:
     * <pre>{@code
     * var f = function(a) { ... };
     * }
     * </pre>
     *
     * Example:
     * <pre>{@code
     *
     * public class FunctionObject extends JSObject {
     *     void function(int x) {
     *         getThis().property("varx",x);
     *     }
     * }
     *
     * public class MyFunc extends JSFunction {
     *     MyFunc(JSContext ctx) {
     *         super(ctx,
     *              "function",               // will call method 'function'
     *              JSObject.class            // calling 'new' will create a JSObject
     *              new FunctionObject(ctx)   // function will be called on FunctionObject
     *         );
     *     }
     * }
     * }
     * </pre>
     *
     * @param ctx    The JSContext to create the object in
     * @param methodName The method to invoke (searches for first instance)
     * @param instanceClass The class to be created on 'new' call
     * @param invokeObject  The object on which to invoke the method
     * @since 3.0
     */
    public JSFunction(JSContext ctx,
                      final String methodName,
                      final Class<? extends JSObject> instanceClass,
                      JSObject invokeObject) {
        context = ctx;
        this.invokeObject = (invokeObject==null) ? this : invokeObject;
        String name = (methodName==null) ? "__nullFunc" : methodName;
        Method [] methods = this.invokeObject.getClass().getMethods();
        for (Method method : methods) {
            if (method.getName().equals(name)) {
                this.method = method;
                break;
            }
        }
        if (method == null) {
            context.sync(new Runnable() {
                @Override
                public void run() {
                    valueRef = makeUndefined(context.ctxRef());
                }
            });
            context.throwJSException(new JSException(context,"No such method. Did you make it public?"));
        }
        context.sync(new Runnable() {
            @Override
            public void run() {
                valueRef = makeFunctionWithCallback(context.ctxRef(), method.getName());
                subclass = instanceClass;
                addJSExports();
            }
        });

        context.persistObject(this);
        context.zombies.add(this);
    }
    /**
     * Creates a new function object which calls method 'methodName' on this Java object.
     * Assumes the 'methodName' method exists on this object and will throw a JSException if not found.  If
     * 'new' is called on this function, it will create a new JSObject instance.
     * In JS:
     * <pre>{@code
     * var f = function(a) { ... };
     * }
     * </pre>
     *
     * Example:
     * <pre>{@code
     *
     * JSFunction f = new JSFunction(context,"function",JSObject.class) {
     *     void function(int x) {
     *         getThis().property("varx",x);
     *     }
     * }
     * }
     * </pre>
     *
     * @param ctx    The JSContext to create the object in
     * @param methodName The method to invoke (searches for first instance)
     * @param instanceClass The class to be created on 'new' call
     * @since 3.0
     */
    public JSFunction(JSContext ctx,
                      final String methodName,
                      final Class<? extends JSObject> instanceClass) {
        this(ctx,methodName,instanceClass,null);
    }
    /**
     * Creates a new function object which calls method 'methodName' on this Java object.
     * Assumes the 'methodName' method exists on this object and will throw a JSException if not found.  If
     * 'new' is called on this function, it will create a new JSObject instance.
     * In JS:
     * <pre>{@code
     * var f = function(a) { ... };
     * }
     * </pre>
     *
     * Example:
     * <pre>{@code
     *
     * JSFunction f = new JSFunction(context,"function") {
     *     void function(int x) {
     *         getThis().property("varx",x);
     *     }
     * }
     * }
     * </pre>
     *
     * @param ctx    The JSContext to create the object in
     * @param methodName The method to invoke (searches for first instance)
     * @since 3.0
     */
    public JSFunction(JSContext ctx,
                      final String methodName) {
        this(ctx,methodName,JSObject.class);
    }

    /**
     * Wraps an existing object as a JSFunction
     * @param objRef  The JavaScriptCore object reference
     * @param context The JSContext the object
     * @since 3.0
     */
    public JSFunction(final long objRef, JSContext context) {
        super(objRef, context);
    }

    /**
     * Calls this JavaScript function, similar to 'Function.call()' in JavaScript
     * @param thiz  The 'this' object on which the function operates, null if not on a constructor object
     * @param args  The argument list to be passed to the function
     * @return The JSValue returned by the function
     * @since 3.0
     */
    public JSValue call(final JSObject thiz, final Object ... args) {
        return apply(thiz,args);
    }

    private long [] argsToValueRefs(final Object[] args) {
        ArrayList<JSValue> largs = new ArrayList<>();
        if (args!=null) {
            for (Object o: args) {
                JSValue v;
                if (o == null) break;
                if (o.getClass() == Void.class)
                    v = new JSValue(context);
                else if (o instanceof JSValue)
                    v = (JSValue)o;
                else if (o instanceof Object[])
                    v = new JSArray<>(context, (Object[])o, Object.class);
                else
                    v = new JSValue(context,o);
                largs.add(v);
            }
        }
        long [] valueRefs = new long[largs.size()];
        for (int i=0; i<largs.size(); i++) {
            valueRefs[i] = largs.get(i).valueRef();
        }
        return valueRefs;
    }

    /**
     * Calls this JavaScript function, similar to 'Function.apply() in JavaScript
     * @param thiz  The 'this' object on which the function operates, null if not on a constructor object
     * @param args  An array of arguments to be passed to the function
     * @return The JSValue returned by the function
     * @since 3.0
     */
    public JSValue apply(final JSObject thiz, final Object [] args) {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = callAsFunction(context.ctxRef(), valueRef, (thiz==null)?0L:thiz.valueRef(),
                        argsToValueRefs(args));
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception!=0) {
            context.throwJSException(new JSException(new JSValue(runnable.jni.exception,context)));
            return new JSValue(context);
        }
        return new JSValue(runnable.jni.reference,context);
    }
    /**
     * Calls this JavaScript function with no args and 'this' as null
     * @return The JSValue returned by the function
     * @since 3.0
     */
    public JSValue call() {
        return call(null);
    }

    /**
     * Calls this JavaScript function as a constructor, i.e. same as calling 'new func(args)'
     * @param args The argument list to be passed to the function
     * @return an instance object of the constructor
     * @since 3.0
     */
    public JSObject newInstance(final Object ... args) {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = callAsConstructor(context.ctxRef(), valueRef, argsToValueRefs(args));
            }
        };
        context.sync(runnable);
        return context.getObjectFromRef(testException(runnable.jni));
    }

    @SuppressWarnings("unused") // This is called directly from native code
    private long functionCallback(long thisObjectRef,
                                  long argumentsValueRef[], long exceptionRefRef) {
        try {
            JSValue [] args = new JSValue[argumentsValueRef.length];
            for (int i=0; i<argumentsValueRef.length; i++) {
                JSObject obj = context.getObjectFromRef(argumentsValueRef[i],false);
                if (obj!=null) args[i] = obj;
                else args[i] = new JSValue(argumentsValueRef[i],context);
            }
            JSObject thiz = context.getObjectFromRef(thisObjectRef);
            JSValue value = function(thiz,args,invokeObject);
            setException(0L, exceptionRefRef);
            return value==null ? 0L : value.valueRef();
        } catch (JSException e) {
            e.printStackTrace();
            setException(e.getError().valueRef(), exceptionRefRef);
            return 0L;
        }
    }

    protected JSValue function(JSObject thiz, JSValue [] args) {
        return function(thiz,args,this);
    }

    protected JSValue function(JSObject thiz, JSValue [] args, final JSObject invokeObject) {
        Class<?>[] pType  = method.getParameterTypes();
        Object [] passArgs = new Object[pType.length];
        for (int i=0; i<passArgs.length; i++) {
            if (i<args.length) {
                if (args[i]==null) passArgs[i] = null;
                else passArgs[i] = args[i].toJavaObject(pType[i]);
            } else {
                passArgs[i] = null;
            }
        }
        JSValue returnValue;
        JSObject stack=null;
        try {
            stack = invokeObject.getThis();
            invokeObject.setThis(thiz);
            Object ret = method.invoke(invokeObject, passArgs);
            if (ret == null) {
                returnValue = null;
            } else if (ret instanceof JSValue) {
                returnValue = (JSValue) ret;
            } else {
                returnValue = new JSValue(context, ret);
            }
        } catch (InvocationTargetException e) {
            e.printStackTrace();
            context.throwJSException(new JSException(context, e.toString()));
            returnValue = null;
        } catch (IllegalAccessException e) {
            context.throwJSException(new JSException(context, e.toString()));
            returnValue = null;
        } finally {
            invokeObject.setThis(stack);
        }
        return returnValue;
    }

    protected void constructor(final long thisObj, final JSValue [] args) {
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                JSValue proto = prototype();
                try {
                    Constructor<?> defaultConstructor = subclass.getConstructor();
                    JSObject thiz = (JSObject) defaultConstructor.newInstance();
                    thiz.context = context;
                    thiz.valueRef = thisObj;
                    function(thiz,args);
                    context.persistObject(thiz);
                    context.zombies.add(thiz);
                } catch (NoSuchMethodException e) {
                    String error = e.toString() + "If " + subclass.getName() + " is an embedded " +
                            "class, did you specify it as 'static'?";
                    context.throwJSException(new JSException(context, error));
                } catch (InvocationTargetException e) {
                    String error = e.toString() + "; Did you remember to call super?";
                    context.throwJSException(new JSException(context, error));
                } catch (IllegalAccessException e) {
                    String error = e.toString() + "; Is your constructor public?";
                    context.throwJSException(new JSException(context, error));
                } catch (InstantiationException e) {
                    context.throwJSException(new JSException(context, e.toString()));
                }
            }
        };
        context.sync(runnable);
    }

    @SuppressWarnings("unused") // This is called directly from native code
    private void constructorCallback(long thisObjectRef,
                                     long argumentsValueRef[], long exceptionRefRef) {

        try {
            JSValue [] args = new JSValue[argumentsValueRef.length];
            for (int i=0; i<argumentsValueRef.length; i++) {
                JSObject obj = context.getObjectFromRef(argumentsValueRef[i],false);
                if (obj!=null) args[i] = obj;
                else args[i] = new JSValue(argumentsValueRef[i],context);
            }
            constructor(thisObjectRef,args);
            setException(0L, exceptionRefRef);
        } catch (JSException e) {
            setException(e.getError().valueRef(), exceptionRefRef);
        }
    }

    private Class<? extends JSObject> subclass = null;

    /**
     * Called only by convenience subclasses.  If you use
     * this, you must set context and valueRef yourself.  Also,
     * don't forget to call protect()!
     */
    protected JSFunction() {
    }

    protected Method method = null;
    private JSObject invokeObject = null;

    protected native long makeFunctionWithCallback(long ctx, String name);

}
