/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import java.util.List;
import java.util.Map;

/**
 * A JavaScript value
 * @since 0.1.0
 */
@SuppressWarnings("WeakerAccess,SameParameterValue")
public class JSValue {

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
        valueRef = ctx.ctxRef().makeUndefined();
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
        if (val == null) {
            valueRef = context.ctxRef().makeNull();
        } else if (val instanceof JSValue) {
            valueRef = ((JSValue) val).valueRef();
        } else if (val instanceof Map) {
            valueRef = new JSObjectPropertiesMap(context, (Map)val, Object.class).getJSObject().valueRef();
        } else if (val instanceof List) {
            valueRef = new JSArray<>(context, (List) val, JSValue.class).valueRef();
        } else if (val.getClass().isArray()) {
            valueRef = new JSArray<>(context, (Object[])val, JSValue.class).valueRef();
        } else if (val instanceof Boolean) {
            valueRef = context.ctxRef().makeBoolean((Boolean)val);
        } else if (val instanceof Double) {
            valueRef = context.ctxRef().makeNumber((Double)val);
        } else if (val instanceof Float) {
            valueRef = context.ctxRef().makeNumber(Double.valueOf(val.toString()));
        } else if (val instanceof Integer ) {
            valueRef = context.ctxRef().makeNumber(((Integer)val).doubleValue());
        } else if (val instanceof Long) {
            valueRef = context.ctxRef().makeNumber(((Long)val).doubleValue());
        } else if (val instanceof Byte) {
            valueRef = context.ctxRef().makeNumber(((Byte)val).doubleValue());
        } else if (val instanceof Short) {
            valueRef = context.ctxRef().makeNumber(((Short)val).doubleValue());
        } else if (val instanceof String) {
            valueRef = context.ctxRef().makeString((String)val);
        } else {
            valueRef = context.ctxRef().makeUndefined();
        }
    }

    /**
     * Wraps an existing JavaScript value
     * @param valueRef  The JavaScriptCore reference to the value
     * @param ctx  The context in which the value exists
     * @since 0.1.0
     */
    protected JSValue(final JNIJSValue valueRef, final JSContext ctx) {
        context = ctx;
        if (valueRef != null) {
            this.valueRef = valueRef;
        } else {
            this.valueRef = ctx.ctxRef().makeUndefined();
        }
    }

    /* Testers */
    /**
     * Tests whether the value is undefined
     * @return  true if undefined, false otherwise
     * @since 0.1.0
     */
    public Boolean isUndefined() {
        return valueRef().isUndefined();
    }
    /**
     * Tests whether the value is null
     * @return  true if null, false otherwise
     * @since 0.1.0
     */
    public Boolean isNull() {
        return valueRef().isNull();
    }
    /**
     * Tests whether the value is boolean
     * @return  true if boolean, false otherwise
     * @since 0.1.0
     */
    public Boolean isBoolean() {
        return valueRef().isBoolean();
    }
    /**
     * Tests whether the value is a number
     * @return  true if a number, false otherwise
     * @since 0.1.0
     */
    public Boolean isNumber() {
        return valueRef().isNumber();
    }
    /**
     * Tests whether the value is a string
     * @return  true if a string, false otherwise
     * @since 0.1.0
     */
    public Boolean isString() {
        return valueRef().isString();
    }
    /**
     * Tests whether the value is an array
     * @return  true if an array, false otherwise
     * @since 0.1.0
     */
    public Boolean isArray() {
        return valueRef().isArray();
    }
    /**
     * Tests whether the value is a date object
     * @return  true if a date object, false otherwise
     * @since 0.1.0
     */
    public Boolean isDate() {
        return valueRef().isDate();
    }
    /**
     * Tests whether the value is a typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isTypedArray() {
        return valueRef().isTypedArray();
    }
    /**
     * Tests whether the value is an Int8 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isInt8Array() {
        return valueRef().isInt8Array();
    }
    /**
     * Tests whether the value is an Int16 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isInt16Array() {
        return valueRef().isInt16Array();
    }
    /**
     * Tests whether the value is an Int32 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isInt32Array() {
        return valueRef().isInt32Array();
    }
    /**
     * Tests whether the value is an unsigned Int8 clamped typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isUint8ClampedArray() {
        return valueRef().isUint8ClampedArray();
    }
    /**
     * Tests whether the value is an unsigned Int8 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isUint8Array() {
        return valueRef().isUint8Array();
    }
    /**
     * Tests whether the value is an unsigned Int16 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isUint16Array() {
        return valueRef().isUint16Array();
    }
    /**
     * Tests whether the value is an unsigned Int32 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isUint32Array() {
        return valueRef().isUint32Array();
    }
    /**
     * Tests whether the value is an float32 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isFloat32Array() {
        return valueRef().isFloat32Array();
    }
    /**
     * Tests whether the value is an float64 typed array
     * @return  true if a typed array object, false otherwise
     * @since 0.4.4
     */
    public Boolean isFloat64Array() {
        return valueRef().isFloat64Array();
    }

    /**
     * Tests whether the value is an object
     * @return  true if an object, false otherwise
     * @since 0.1.0
     */
    public Boolean isObject() {
        return valueRef().isObject();
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
        try {
            return valueRef().isEqual(ojsv.valueRef);
        } catch (JNIJSException excp) {
            return false;
        }
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
        return valueRef().isStrictEqual(ojsv.valueRef);
    }

    /* Getters */
    /**
     * Gets the Boolean value of this JS value
     * @return  the Boolean value
     * @since 0.1.0
     */
    public Boolean toBoolean() {
        return valueRef().toBoolean();
    }
    /**
     * Gets the numeric value of this JS value
     * @return  The numeric value
     * @since 0.1.0
     */
    public Double toNumber() {
        try {
            return valueRef().toNumber();
        } catch (JNIJSException excp) {
            context.throwJSException(new JSException(new JSValue(excp.exception, context)));
            return 0.0;
        }
    }
    @Override
    public String toString() {
        try {
            return valueRef().toStringCopy();
        } catch (JNIJSException excp) {
            context.throwJSException(new JSException(new JSValue(excp.exception, context)));
            return null;
        }
    }
    /**
     * If the JS value is an object, gets the JSObject
     * @return  The JSObject for this value
     * @since 0.1.0
     */
    public JSObject toObject() {
        if (this instanceof JSObject) return (JSObject)this;

        try {
            return context.getObjectFromRef(valueRef().toObject());
        } catch (JNIJSException excp) {
            context.throwJSException(new JSException(new JSValue(excp.exception, context)));
            return new JSObject(context);
        }
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
        try {
            JSValue json = new JSValue(valueRef().createJSONString(),context);
            if (json.isUndefined())
                return null;
            else
                return json.toString();
        } catch (JNIJSException excp) {
            context.throwJSException(new JSException(new JSValue(excp.exception, context)));
            return null;
        }
    }

    @SuppressWarnings("unchecked")
    Object toJavaObject(Class clazz) {
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
    public long valueHash() { return valueRef().reference; }

}
