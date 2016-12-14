//
// JSObjectPropertiesMap.java
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

import java.util.AbstractList;
import java.util.AbstractSet;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;

/**
 * A JSObject shadow class which implements the Java Map interface.  Convenient
 * for setting/getting/iterating properties.
 */
class JSObjectPropertiesMap<V> extends JSObjectWrapper implements Map<String, V> {
    /**
     * Creates a new Map object which operates on object 'object' and assumes type 'cls'.
     * Example:
     * <code>
     *     java.util.Map&lt;String,Double&gt; map = new JSObjectPropertiesMap&lt;String,Double&gt;(object,Double.class);
     * </code>
     * @param object The JSObject whose properties will be mapped
     * @param cls    The class of the component Values; must match template
     * @since 3.0
     */
    JSObjectPropertiesMap(JSObject object, Class<V> cls) {
        super(object);
        mType = cls;
    }

    /**
     * Creates a new Map object and underlying JSObject and sets initial properties in 'map'.
     * Assumes value class of type 'cls'.
     * Example:
     * <code>
     *     java.util.Map&lt;String,Double&gt; map = new HashMap&lt;&gt;();
     *     map.put("one",1.0);
     *     map.put("two",2.0);
     *     java.util.Map&lt;String,Double&gt; jsmap = new JSObjectPropertiesMap&lt;String,Double&gt;(context,map,Double.class)
     * </code>
     * @param context  The JSContext in which to create the object
     * @param map      The initial properties to set
     * @param cls      The class of the component Values; must match template
     * @since 3.0
     */
    JSObjectPropertiesMap(JSContext context, Map map, Class<V> cls) {
        super(new JSObject(context,map));
        mType = cls;
    }

    /**
     * Creates a new Map object and underlying JSObject with no initial properties.
     * Assumes value class of type 'cls'.
     * Example:
     * <code>
     *     java.util.Map&lt;String,Double&gt; jsmap = new JSObjectPropertiesMap&lt;String,Double&gt;(context,Double.class)
     * </code>
     * @param context  The JSContext in which to create the object
     * @param cls      The class of the component Values; must match template
     * @since 3.0
     */
    JSObjectPropertiesMap(JSContext context, Class<V> cls) {
        super(new JSObject(context));
        mType = cls;
    }

    private final Class<V> mType;

    /**
     * @see Map#size()
     */
    @Override
    public int size() {
        return propertyNames().length;
    }

    /**
     * @see Map#isEmpty()
     * @since 3.0
     */
    @Override
    public boolean isEmpty() {
        return size() == 0;
    }

    /**
     * @see Map#containsKey(Object)
     * @since 3.0
     */
    @Override
    public boolean containsKey(final Object key) {
        return hasProperty(key.toString());
    }

    /**
     * @see Map#containsValue(Object)
     * @since 3.0
     */
    @Override
    public boolean containsValue(final Object value) {
        String[] properties = propertyNames();
        for (String key : properties) {
            if (property(key).equals(value))
                return true;
        }
        return false;
    }

    /**
     * @see Map#get(Object)
     * @since 3.0
     */
    @Override
    @SuppressWarnings("unchecked")
    public V get(final Object key) {
        JSValue val = property(key.toString());
        if (val.isUndefined()) return null;
        return (V) val.toJavaObject(mType);
    }

    /**
     * @see Map#put(Object, Object)
     * @since 3.0
     */
    @Override
    public V put(final String key, final V value) {
        final V oldValue = get(key);
        property(key,value);
        return oldValue;
    }

    /**
     * @see Map#remove(Object)
     * @since 3.0
     */
    @Override
    public V remove(final Object key) {
        final V oldValue = get(key);
        deleteProperty(key.toString());
        return oldValue;
    }

    /**
     * @see Map#putAll(Map)
     * @since 3.0
     */
    @Override
    public void putAll(final @NonNull Map<? extends String, ? extends V> map) {
        for (String key : map.keySet()) {
            put(key,map.get(key));
        }
    }

    /**
     * @see Map#clear()
     * @since 3.0
     */
    @Override
    public void clear()
    {
        for (String prop : propertyNames()) {
            deleteProperty(prop);
        }
    }

    /**
     * @see Map#keySet()
     * @since 3.0
     */
    @Override
    @SuppressWarnings("unchecked")
    public @NonNull
    Set keySet() {
        return new HashSet(Arrays.asList(propertyNames()));
    }

    /**
     * @see Map#values()
     * @since 3.0
     */
    @Override
    public @NonNull
    Collection<V> values()
    {
        return new AbstractList<V>()
        {
            @Override
            public V get(final int index)
            {
                String [] propertyNames = propertyNames();
                if (index > propertyNames.length)
                {
                    throw new IndexOutOfBoundsException();
                }
                return JSObjectPropertiesMap.this.get(propertyNames[index]);
            }

            @Override
            public int size()
            {
                return propertyNames().length;
            }

            @Override
            public boolean contains(Object val) {
                return containsValue(val);
            }
        };
    }

    private class SetIterator implements Iterator<Entry<String,V>> {
        private String current = null;
        private String removal = null;

        SetIterator() {
            String [] properties = propertyNames();
            if (properties.length > 0)
                current = properties[0];
        }

        @Override
        public boolean hasNext() {
            if (current==null) return false;

            // Make sure 'current' still exists
            String [] properties = propertyNames();
            for (String prop : properties) {
                if (current.equals(prop)) return true;
            }
            return false;
        }

        @Override
        @SuppressWarnings("unchecked")
        public Entry<String,V> next() {
            if (current == null)
                throw new NoSuchElementException();

            String [] properties = propertyNames();
            Entry<String,V> entry = null;
            int i = 0;
            for (; i<properties.length; i++) {
                if (current.equals(properties[i])) {
                    final Object key = properties[i];
                    entry = new Entry<String, V>() {
                        @Override
                        public String getKey() {
                            return (String) key;
                        }

                        @Override
                        public V getValue() {
                            return get(key);
                        }

                        @Override
                        public V setValue(V object) {
                            return put((String)key,object);
                        }
                    };
                    break;
                }
            }
            removal = current;
            if (i+1 < properties.length)
                current = properties[i+1];
            else
                current = null;

            if (entry != null)
                return entry;
            else
                throw new NoSuchElementException();
        }

        @Override
        public void remove() {
            if (removal==null)
                throw new NoSuchElementException();

            deleteProperty(removal);
            removal = null;
        }
    }

    /**
     * @see Map#entrySet()
     * @since 3.0
     */
    @Override
    public @NonNull
    Set<Entry<String, V>> entrySet() {
        return new AbstractSet<Entry<String, V>>() {

            @Override
            public @NonNull
            Iterator<Entry<String, V>> iterator() {
                return new SetIterator();
            }

            @Override
            public int size() {
                return propertyNames().length;
            }
        };
    }

}
