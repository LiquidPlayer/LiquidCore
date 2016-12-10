//
// JSObject.java
// AndroidJSCore project
//
// https://github.com/ericwlange/AndroidJSCore/
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

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * A JavaScript object.
 * @since 1.0
 *
 */
@SuppressWarnings("JniMissingFunction")
public class JSObject extends JSValue {

    @Retention(RetentionPolicy.RUNTIME)
    public @interface jsexport {
        Class type() default Object.class;
        int attributes() default JSPropertyAttributeNone;
    }

    public class Property<T>
    {
        private T temp = null;
        private Class pT;
        private Integer attributes=null;

        private Property()
        {
        }

        public void set(T v)
        {
            temp = v;
            if (temp != null)
                pT = temp.getClass();
            if (name != null) {
                if (attributes != null) {
                    property(name, v, attributes);
                    attributes = null;
                } else {
                    property(name, v);
                }
            }
        }

        @SuppressWarnings("unchecked")
        public T get()
        {
            if (temp == null && pT==Object.class) {
                context.throwJSException(new JSException(context,"object has no defined type"));
                return null;
            }
            if (name != null)
                return (T) property(name).toJavaObject(pT);
            else
                return this.temp;
        }

        private String name=null;
        private void setName(String n, Class cls, int attributes) {
            name = n;
            this.attributes = attributes;
            if (temp != null) {
                property(name, temp, this.attributes);
                this.attributes = null;
            } else {
                pT = cls;
                property(name, new JSValue(context));
            }
        }
    }

    private abstract class JNIReturnClass implements Runnable {
        JNIReturnObject jni;
    }

    /**
     * Specifies that a property has no special attributes.
     */
    public static final int JSPropertyAttributeNone = 0;
    /**
     * Specifies that a property is read-only.
     */
    public static final int JSPropertyAttributeReadOnly = 1 << 1;
    /**
     * Specifies that a property should not be enumerated by
     * JSPropertyEnumerators and JavaScript for...in loops.
     */
    public static final int JSPropertyAttributeDontEnum = 1 << 2;
    /**
     * Specifies that the delete operation should fail on a property.
     */
    public static final int JSPropertyAttributeDontDelete = 1 << 3;

    /**
     * Creates a new, empty JavaScript object.  In JS:
     * <pre>
     * {@code
     * var obj = {}; // OR
     * var obj = new Object();
     * }
     * </pre>
     *
     * @param ctx The JSContext to create the object in
     * @since 1.0
     */
    public JSObject(JSContext ctx) {
        context = ctx;
        context.sync(new Runnable() {
            @Override
            public void run() {
                valueRef = make(context.ctxRef());
                addJSExports();
            }
        });
        context.persistObject(this);
    }

    protected void addJSExports() {
        try {
            for (Field f : getClass().getDeclaredFields()) {
                if (f.isAnnotationPresent(jsexport.class)) {
                    f.setAccessible(true);
                    if (Property.class.isAssignableFrom(f.getType())) {
                        Property prop = (Property) f.get(this);
                        if (prop == null) {
                            Constructor ctor =
                                    f.getType().getDeclaredConstructor(JSObject.class);
                            ctor.setAccessible(true);
                            prop = (Property) ctor.newInstance(this);
                            f.set(this, prop);
                        }
                        prop.setName(f.getName(),
                                f.getAnnotation(jsexport.class).type(),
                                f.getAnnotation(jsexport.class).attributes());
                    }
                }
            }
            Method[] methods = getClass().getDeclaredMethods();
            for (Method m : methods) {
                if (m.isAnnotationPresent(jsexport.class)) {
                    m.setAccessible(true);
                    JSFunction f = new JSFunction(context, m,
                            JSObject.class, JSObject.this);
                    property(m.getName(), f, m.getAnnotation(jsexport.class).attributes());
                }
            }
        } catch (Exception e) {
            context.throwJSException(new JSException(context,e.toString()));
        }
    }

    /**
     * Called only by convenience subclasses.  If you use
     * this, you must set context and valueRef yourself.
     * @since 3.0
     */
     public JSObject() {
     }

    /**
     * Wraps an existing object from JavaScript
     *
     * @param objRef The JavaScriptCore object reference
     * @param ctx    The JSContext of the reference
     * @since 1.0
     */
    protected JSObject(final long objRef, JSContext ctx) {
        super(objRef, ctx);
        context.persistObject(this);
    }

    /**
     * Creates a new object with function properties set for each method
     * in the defined interface.
     * In JS:
     * <pre>
     * {@code
     * var obj = {
     *     func1: function(a)   { alert(a); },
     *     func2: function(b,c) { alert(b+c); }
     * };
     * }
     * </pre>
     * Where func1, func2, etc. are defined in interface 'iface'.  This JSObject
     * must implement 'iface'.
     *
     * @param ctx   The JSContext to create the object in
     * @param iface The Java Interface defining the methods to expose to JavaScript
     * @since 1.0
     */
    public JSObject(JSContext ctx, final Class<?> iface) {
        context = ctx;
        context.sync(new Runnable() {
            @Override
            public void run() {
                valueRef = make(context.ctxRef());
                addJSExports();
                Method[] methods = iface.getDeclaredMethods();
                for (Method m : methods) {
                    JSFunction f = new JSFunction(context, m,
                            JSObject.class, JSObject.this);
                    property(m.getName(), f);
                }
            }
        });
        context.persistObject(this);
    }

    /**
     * Creates a new object with the entries in 'map' set as properties.
     *
     * @param ctx  The JSContext to create object in
     * @param map  The map containing the properties
     */
    @SuppressWarnings("unchecked")
    public JSObject(JSContext ctx, final Map map) {
        this(ctx);
        new JSObjectPropertiesMap<>(this,Object.class).putAll(map);
        addJSExports();
    }

    /**
     * Determines if the object contains a given property
     *
     * @param prop The property to test the existence of
     * @return true if the property exists on the object, false otherwise
     * @since 1.0
     */
    public boolean hasProperty(final String prop) {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = hasProperty(context.ctxRef(), valueRef, prop);
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }

    /**
     * Gets the property named 'prop'
     *
     * @param prop The name of the property to fetch
     * @return The JSValue of the property, or null if it does not exist
     * @since 1.0
     */
    public JSValue property(final String prop) {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = getProperty(context.ctxRef(), valueRef, prop);
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception != 0) {
            context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
            return new JSValue(context);
        }
        return new JSValue(runnable.jni.reference, context);
    }

    /**
     * Sets the value of property 'prop'
     *
     * @param prop       The name of the property to set
     * @param value      The Java object to set.  The Java object will be converted to a JavaScript object
     *                   automatically.
     * @param attributes And OR'd list of JSProperty constants
     * @since 1.0
     */
    public void property(final String prop, final Object value, final int attributes) {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                long ref = (value instanceof JSValue) ?
                        ((JSValue) value).valueRef() : new JSValue(context, value).valueRef();
                jni = setProperty(
                        context.ctxRef(),
                        valueRef,
                        prop,
                        ref,
                        attributes);
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception != 0) {
            context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
        }
    }

    /**
     * Sets the value of property 'prop'.  No JSProperty attributes are set.
     *
     * @param prop  The name of the property to set
     * @param value The Java object to set.  The Java object will be converted to a JavaScript object
     *              automatically.
     * @since 1.0
     */
    public void property(String prop, Object value) {
        property(prop, value, JSPropertyAttributeNone);
    }

    /**
     * Deletes a property from the object
     *
     * @param prop The name of the property to delete
     * @return true if the property was deleted, false otherwise
     * @since 1.0
     */
    public boolean deleteProperty(final String prop) {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = deleteProperty(context.ctxRef(), valueRef, prop);
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception != 0) {
            context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
            return false;
        }
        return runnable.jni.bool;
    }

    /**
     * Returns the property at index 'index'.  Used for arrays.
     *
     * @param index The index of the property
     * @return The JSValue of the property at index 'index'
     * @since 1.0
     */
    public JSValue propertyAtIndex(final int index) {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = getPropertyAtIndex(context.ctxRef(), valueRef, index);
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception != 0) {
            context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
            return new JSValue(context);
        }
        return new JSValue(runnable.jni.reference, context);
    }

    /**
     * Sets the property at index 'index'.  Used for arrays.
     *
     * @param index The index of the property to set
     * @param value The Java object to set, will be automatically converted to a JavaScript value
     * @since 1.0
     */
    public void propertyAtIndex(final int index, final Object value) {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = setPropertyAtIndex(context.ctxRef(), valueRef, index,
                        (value instanceof JSValue) ? ((JSValue) value).valueRef() : new JSValue(context, value).valueRef());
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception != 0) {
            context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
        }
    }

    private abstract class StringArrayReturnClass implements Runnable {
        public String[] sArray;
    }

    /**
     * Gets the list of set property names on the object
     *
     * @return A string array containing the property names
     * @since 1.0
     */
    public String[] propertyNames() {
        StringArrayReturnClass runnable = new StringArrayReturnClass() {
            @Override
            public void run() {
                sArray = copyPropertyNames(context.ctxRef(), valueRef);
            }
        };
        context.sync(runnable);
        return runnable.sArray;
    }

    /**
     * Determines if the object is a function
     *
     * @return true if the object is a function, false otherwise
     * @since 1.0
     */
    public boolean isFunction() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = isFunction(context.ctxRef(), valueRef);
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }

    /**
     * Determines if the object is a constructor
     *
     * @return true if the object is a constructor, false otherwise
     * @since 1.0
     */
    public boolean isConstructor() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = isConstructor(context.ctxRef(), valueRef);
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }

    @Override
    public int hashCode() {
        return valueRef().intValue();
    }

    /**
     * Gets the prototype object, if it exists
     * @return A JSValue referencing the prototype object, or null if none
     * @since 3.0
     */
    public JSValue prototype() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.reference = getPrototype(context.ctxRef(), valueRef);
            }
        };
        context.sync(runnable);
        return new JSValue(runnable.jni.reference,context);
    }

    /**
     * Sets the prototype object
     * @param proto The object defining the function prototypes
     * @since 3.0
     */
    public void prototype(final JSValue proto) {
        context.sync(new Runnable() {
            @Override
            public void run() {
                setPrototype(context.ctxRef(), valueRef, proto.valueRef());
            }
        });
    }

    protected final List<JSObject> zombies = new ArrayList<>();

    @Override
    protected void finalize() throws Throwable {
        context.finalizeObject(this);
        super.finalize();
    }

    protected void setThis(JSObject thiz) {
        this.thiz = thiz;
    }

    public JSObject getThis() {
        return thiz;
    }

    @SuppressWarnings("unused")
    public JSValue __nullFunc() {
        return new JSValue(context);
    }

    private JSObject thiz = null;

    /* Native Methods */

    protected native long make(long ctx);

    protected native JNIReturnObject makeArray(long ctx, long[] args);

    protected native long makeDate(long ctx, long[] args);

    protected native long makeError(long ctx, String message);

    protected native JNIReturnObject makeRegExp(long ctx, String pattern, String flags);

    protected native long getPrototype(long ctx, long object);

    protected native void setPrototype(long ctx, long object, long value);

    protected native boolean hasProperty(long ctx, long object, String propertyName);

    protected native JNIReturnObject getProperty(long ctx, long object, String propertyName);

    protected native JNIReturnObject setProperty(long ctx, long object, String propertyName, long value, int attributes);

    protected native JNIReturnObject deleteProperty(long ctx, long object, String propertyName);

    protected native JNIReturnObject getPropertyAtIndex(long ctx, long object, int propertyIndex);

    protected native JNIReturnObject setPropertyAtIndex(long ctx, long object, int propertyIndex, long value);

    protected native boolean isFunction(long ctx, long object);

    protected native JNIReturnObject callAsFunction(long ctx, long object, long thisObject, long[] args);

    protected native boolean isConstructor(long ctx, long object);

    protected native JNIReturnObject callAsConstructor(long ctx, long object, long[] args);

    protected native String[] copyPropertyNames(long ctx, long object);

    protected native JNIReturnObject makeFunction(long ctx, String name,
                                                  String func, String sourceURL, int startingLineNumber);
}