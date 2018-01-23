//
// JSValue.java
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

import java.util.List;
import java.util.Map;

/**
 * A JavaScript value
 * @since 0.1.0
 */
public class JSValue {

    private abstract class JNIReturnClass implements Runnable {
        JNIReturnObject jni;
    }

    protected JNIJSValue valueRef;
    protected JSContext context = null;

    /* Constructors */
    /**
     * Creates an empty JSValue.  This can only be used by subclasses, and those
     * subclasses must define 'context' and 'valueRef' themselves
     * @since 0.1.0
     */
    protected JSValue() {
    }
    /**
     * Creates a new undefined JavaScript value
     * @param ctx  The context in which to create the value
     * @since 0.1.0
     */
    public JSValue(final JSContext ctx) {
        context = ctx;
        context.sync(new Runnable() {
            @Override
            public void run() {
                valueRef = JNIJSValue.makeUndefined(context.ctxRef());
            }
        });
    }
    /**
     * Creates a new JavaScript value from a Java value.  Classes supported are:
     * Boolean, Double, Integer, Long, String, Byte, Short, List, Map, Float.  Any other object will
     * generate an undefined JavaScript value.
     * @param ctx  The context in which to create the value
     * @param val  The Java value
     * @since 0.1.0
     */
    @SuppressWarnings("unchecked")
    public JSValue(JSContext ctx, final Object val) {
        context = ctx;
        context.sync(new Runnable() {
            @Override
            public void run() {
                if (val == null) {
                    valueRef = JNIJSValue.makeNull(context.ctxRef());
                } else if (val instanceof JSValue) {
                    valueRef = ((JSValue) val).valueRef();
                } else if (val instanceof Map) {
                    valueRef = new JSObjectPropertiesMap(context, (Map)val, Object.class).getJSObject().valueRef();
                } else if (val instanceof List) {
                    valueRef = new JSArray<>(context, (List) val, JSValue.class).valueRef();
                } else if (val.getClass().isArray()) {
                    valueRef = new JSArray<>(context, (Object[])val, JSValue.class).valueRef();
                } else if (val instanceof Boolean) {
                    valueRef = JNIJSValue.makeBoolean(context.ctxRef(), (Boolean)val);
                } else if (val instanceof Double) {
                    valueRef = JNIJSValue.makeNumber(context.ctxRef(), (Double)val);
                } else if (val instanceof Float) {
                    valueRef = JNIJSValue.makeNumber(context.ctxRef(), Double.valueOf(val.toString()));
                } else if (val instanceof Integer ) {
                    valueRef = JNIJSValue.makeNumber(context.ctxRef(), ((Integer)val).doubleValue());
                } else if (val instanceof Long) {
                    valueRef = JNIJSValue.makeNumber(context.ctxRef(), ((Long)val).doubleValue());
                } else if (val instanceof Byte) {
                    valueRef = JNIJSValue.makeNumber(context.ctxRef(), ((Byte)val).doubleValue());
                } else if (val instanceof Short) {
                    valueRef = JNIJSValue.makeNumber(context.ctxRef(), ((Short)val).doubleValue());
                } else if (val instanceof String) {
                    valueRef = JNIJSValue.makeString(context.ctxRef(), (String)val);
                } else {
                    valueRef = JNIJSValue.makeUndefined(context.ctxRef());
                }
            }
        });
    }

    /**
     * Wraps an existing JavaScript value
     * @param valueRef  The JavaScriptCore reference to the value
     * @param ctx  The context in which the value exists
     * @since 0.1.0
     */
    protected JSValue(final JNIJSValue valueRef, JSContext ctx) {
        context = ctx;
        if (valueRef != null) {
            this.valueRef = valueRef;
        } else {
            context.sync(new Runnable() {
                @Override
                public void run() {
                    JSValue.this.valueRef = JNIJSValue.makeUndefined(context.ctxRef());
                }
            });
        }
    }

    /* Testers */
    /**
     * Tests whether the value is undefined
     * @return  true if undefined, false otherwise
     * @since 0.1.0
     */
    public Boolean isUndefined() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isUndefined();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is null
     * @return  true if null, false otherwise
     * @since 0.1.0
     */
    public Boolean isNull() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isNull();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is boolean
     * @return  true if boolean, false otherwise
     * @since 0.1.0
     */
    public Boolean isBoolean() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isBoolean();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is a number
     * @return  true if a number, false otherwise
     * @since 0.1.0
     */
    public Boolean isNumber() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isNumber();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is a string
     * @return  true if a string, false otherwise
     * @since 0.1.0
     */
    public Boolean isString() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isString();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an array
     * @return  true if an array, false otherwise
     * @since 0.1.0
     */
    public Boolean isArray() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isArray();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is a date object
     * @return  true if a date object, false otherwise
     * @since 0.1.0
     */
    public Boolean isDate() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isDate();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is a typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isTypedArray() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isTypedArray();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an Int8 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isInt8Array() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isInt8Array();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an Int16 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isInt16Array() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isInt16Array();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an Int32 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isInt32Array() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isInt32Array();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an unsigned Int8 clamped typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isUint8ClampedArray() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isUint8ClampedArray();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an unsigned Int8 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isUint8Array() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isUint8Array();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an unsigned Int16 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isUint16Array() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isUint16Array();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an unsigned Int32 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isUint32Array() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isUint32Array();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an float32 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isFloat32Array() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isFloat32Array();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether the value is an float64 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isFloat64Array() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isFloat64Array();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }

    /**
     * Tests whether the value is an object
     * @return  true if an object, false otherwise
     * @since 0.1.0
     */
    public Boolean isObject() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isObject();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Tests whether a value in an instance of a constructor object
     * @param constructor  The constructor object to test
     * @return  true if the value is an instance of the given constructor object, false otherwise
     * @since 0.1.0
     */
    public Boolean isInstanceOfConstructor(final JSObject constructor) {
        try {
            JSFunction instanceOf = new JSFunction(context, "_instanceOf", new String[]{"a", "b"},
                    "return (a instanceof b);",
                    null, 0);
            return instanceOf.call(null, this, constructor).toBoolean();
        } catch (JSException e) {
            return false;
        }
    }

    /* Comparators */
    @Override
    @SuppressWarnings("EqualsWhichDoesntCheckParameterClass")
    public boolean equals(Object other) {
        return isEqual(other);
    }

    /**
     * JavaScript definition of equality (==).  JSValue.equals() and JSValue.isEqual() represent
     * the Java and JavaScript definitions, respectively.  Normally they will return the same
     * value, however some classes may override and offer different results.  Example,
     * in JavaScript, new Float32Array([1,2,3]) == new Float32Array([1,2,3]) will be false (as
     * the equality is only true if they are the same physical object), but from a Java util.java.List
     * perspective, these two are equal.
     * @param other the value to compare for equality
     * @return true if == from JavaScript perspective, false otherwise
     * @since 0.1.0
     */
    public boolean isEqual(Object other) {
        if (other == this) return true;
        JSValue otherJSValue;
        if (other instanceof JSValue) {
            otherJSValue = (JSValue)other;
        } else {
            otherJSValue = new JSValue(context, other);
        }
        final JSValue ojsv = otherJSValue;
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = valueRef().isEqual(ojsv.valueRef);
            }
        };
        context.sync(runnable);
        return runnable.jni.exception==null && runnable.jni.bool;
    }

    /**
     * Tests whether two values are strict equal.  In JavaScript, equivalent to '===' operator.
     * @param other  The value to test against
     * @return  true if values are strict equal, false otherwise
     * @since 0.1.0
     */
    public boolean isStrictEqual(Object other) {
        if (other == this) return true;
        JSValue otherJSValue;
        if (other instanceof JSValue) {
            otherJSValue = (JSValue)other;
        } else {
            otherJSValue = new JSValue(context, other);
        }
        final JSValue ojsv = otherJSValue;
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().isStrictEqual(ojsv.valueRef);
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }

    /* Getters */
    /**
     * Gets the Boolean value of this JS value
     * @return  the Boolean value
     * @since 0.1.0
     */
    public Boolean toBoolean() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = new JNIReturnObject();
                jni.bool = valueRef().toBoolean();
            }
        };
        context.sync(runnable);
        return runnable.jni.bool;
    }
    /**
     * Gets the numeric value of this JS value
     * @return  The numeric value
     * @since 0.1.0
     */
    public Double toNumber() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = valueRef().toNumber();
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception!=null) {
            context.throwJSException(new JSException(new JSValue((JNIJSValue)runnable.jni.exception, context)));
            return 0.0;
        }
        return runnable.jni.number;
    }
    @Override
    public String toString() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = valueRef().toStringCopy();
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception!=null) {
            context.throwJSException(new JSException(new JSValue((JNIJSValue)runnable.jni.exception, context)));
            return null;
        }
        return runnable.jni.string;
    }
    /**
     * If the JS value is an object, gets the JSObject
     * @return  The JSObject for this value
     * @since 0.1.0
     */
    public JSObject toObject() {
        if (this instanceof JSObject) return (JSObject)this;

        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = valueRef().toObject();
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception!=null) {
            context.throwJSException(new JSException(new JSValue((JNIJSValue)runnable.jni.exception, context)));
            return new JSObject(context);
        }
        return context.getObjectFromRef((JNIJSObject)runnable.jni.reference);
    }

    /**
     * If the JS value is a function, gets the JSFunction
     * @return  The JSFunction for this value
     * @since 0.1.0
     */
    public JSFunction toFunction() {
        if (isObject()) {
            JSObject obj = toObject();
            if (obj instanceof JSFunction) return (JSFunction)obj;
        }
        context.throwJSException(new JSException(context, "JSObject not a function"));
        return null;
    }

    /**
     * If the JS value is an array, gets the JSArray
     * @return  The JSArray for this value
     * @since 0.1.0
     */
    public JSBaseArray toJSArray() {
        if (isObject()) {
            JSObject obj = toObject();
            if (obj instanceof JSBaseArray) return (JSBaseArray) obj;
        }
        context.throwJSException(new JSException(context, "JSObject not an array"));
        return null;
    }

    /**
     * Gets the JSON of this JS value
     * @return  the JSON representing this value, or null if value is undefined
     * @since 0.1.0
     */
    public String toJSON() {
        JNIReturnClass runnable = new JNIReturnClass() {
            @Override
            public void run() {
                jni = valueRef().createJSONString();
                if (jni.exception == null) {
                    JSValue json = new JSValue((JNIJSValue)jni.reference,context);
                    if (json.isUndefined())
                        jni.string = null;
                    else
                        jni.string = json.toString();
                }
            }
        };
        context.sync(runnable);
        if (runnable.jni.exception!=null) {
            context.throwJSException(new JSException(new JSValue((JNIJSValue)runnable.jni.exception, context)));
            return null;
        }
        return runnable.jni.string;
    }

    @SuppressWarnings("unchecked")
    protected Object toJavaObject(Class clazz) {
        if (clazz == Object.class)
            return this;
        else if (clazz == Map.class)
            return new JSObjectPropertiesMap(toObject(),Object.class);
        else if (clazz == List.class)
            return toJSArray();
        else if (clazz == String.class)
            return toString();
        else if (clazz == Double.class || clazz == double.class)
            return toNumber();
        else if (clazz == Float.class || clazz == float.class)
            return toNumber().floatValue();
        else if (clazz == Integer.class || clazz == int.class)
            return toNumber().intValue();
        else if (clazz == Long.class || clazz == long.class)
            return toNumber().longValue();
        else if (clazz == Byte.class || clazz == byte.class)
            return toNumber().byteValue();
        else if (clazz == Short.class || clazz == short.class)
            return toNumber().shortValue();
        else if (clazz == Boolean.class || clazz == boolean.class)
            return toBoolean();
        else if (clazz.isArray())
            return toJSArray().toArray(clazz.getComponentType());
        else if (JSArray.class.isAssignableFrom(clazz))
            return clazz.cast(toJSArray());
        else if (JSObject.class.isAssignableFrom(clazz))
            return clazz.cast(toObject());
        else if (JSValue.class.isAssignableFrom(clazz))
            return clazz.cast(this);
        return null;
    }

    @Override
    public int hashCode() {
        if (isBoolean()) return toBoolean().hashCode();
        else if (isNumber()) return toNumber().hashCode();
        else if (isString()) return toString().hashCode();
        else if (isUndefined() || isNull()) return 0;
        else return super.hashCode();
    }

    /**
     * Gets the JSContext of this value
     * @return the JSContext of this value
     * @since 0.1.0
     */
    public JSContext getContext() {
        return context;
    }
    /**
     * Gets the JavaScriptCore value reference
     * @return  the JavaScriptCore value reference
     * @since 0.1.0
     */
    public JNIJSValue valueRef() {
        return valueRef;
    }
}
