/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.jstest;

import org.junit.Test;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSObjectPropertiesMap;
import org.liquidplayer.javascript.JSValue;

import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSObjectPropertiesMapTest {

    @Test
    public void testJSMapConstructors() {
        JSContext context = new JSContext();
        testJSMapConstructors(context);
    }

    public void testJSMapConstructors(JSContext context) {
        /*
         * new JSObjectPropertiesMap(object,cls)
         */
        JSObject object = new JSObject(context);
        object.property("a",1);
        object.property("b",2.0f);
        Map<String,Integer> map = new JSObjectPropertiesMap<>(object,Integer.class);
        assertEquals(map.get("a"),Integer.valueOf(1));
        assertEquals(map.get("b"),Integer.valueOf(2));

        /*
         * new JSObjectPropertiesMap(context,map,cls)
         */
        Map<String,Integer> map2 = new JSObjectPropertiesMap<>(context,map,Integer.class);
        assertEquals(map2.get("a"),Integer.valueOf(1));
        assertEquals(map2.get("b"),Integer.valueOf(2));
        assertFalse(((JSObjectPropertiesMap)map2).getJSObject()
                .isStrictEqual(((JSObjectPropertiesMap)map).getJSObject()));

        /*
         * new JSObjectPropertiesMap(context,cls)
         */
        Map<String,Double> map3 = new JSObjectPropertiesMap<>(context,Double.class);
        map3.put("key",3.0);
        assertEquals(Double.valueOf(3), map3.get("key"));
    }

    @Test
    public void testJSMapMethods() {
        JSContext context = new JSContext();
        testJSMapMethods(context);
    }

    public void testJSMapMethods(JSContext context) {
        Map<String,Object> map = new JSObjectPropertiesMap<>(context,Object.class);

        /*
         * isEmpty()
         */
        assertTrue(map.isEmpty());
        ((JSObjectPropertiesMap)map).getJSObject().property("foo","bar");
        assertFalse(map.isEmpty());

        /*
         * size()
         */
        assertThat(map.size(),is(1));
        ((JSObjectPropertiesMap)map).getJSObject().property("mutt","jeff",JSObject.JSPropertyAttributeDontEnum);
        assertThat(map.size(),is(1));
        map.put("cup","saucer");
        ((JSObjectPropertiesMap)map).getJSObject().property("yin","yang");
        assertThat(map.size(),is(3));

        /*
         * containsKey()
         */
        assertTrue(map.containsKey("foo"));
        assertTrue(map.containsKey("mutt"));
        assertTrue(map.containsKey("cup"));
        assertTrue(map.containsKey("yin"));
        assertFalse(map.containsKey("notme"));

        /*
         * containsValue()
         * (Non-enumerable values will not be here)
         */
        assertTrue(map.containsValue("bar"));
        assertFalse(map.containsValue("jeff"));
        assertTrue(map.containsValue("saucer"));
        assertTrue(map.containsValue("yang"));
        assertFalse(map.containsValue("notme"));

        /*
         * get()
         */
        assertEquals(map.get("foo"),"bar");
        assertEquals(map.get("mutt"),"jeff");
        assertEquals(map.get("cup"),"saucer");
        assertEquals(map.get("yin"),"yang");
        assertNull(map.get("notme"));

        /*
         * put()
         */
        map.put("int",1);
        map.put("double",2.2);
        map.put("float",3.3f);
        map.put("string","a string");
        map.put("object", new JSObject(context));
        map.put("array", new Float [] { 1.1f, 2.2f, 3.3f});
        assertEquals(map.get("int"),1);
        assertEquals(map.get("double"), 2.2);
        assertEquals(map.get("float"), 3.3);
        assertEquals(map.get("string"),"a string");
        JSValue object = (JSValue) map.get("object");
        assertNotNull(object);
        assertTrue(object.isObject());
        JSValue array = (JSValue) map.get("array");
        assertNotNull(array);
        assertTrue(array.isArray());

        Map<String,String> map2 = new JSObjectPropertiesMap<>(context,String.class);
        map2.put("0","zero");
        map2.put("1","one");
        map2.put("2","two");
        assertEquals(map2.get("0"),"zero");
        assertEquals(map2.get("1"),"one");
        assertEquals(((JSObjectPropertiesMap)map2).getJSObject().propertyAtIndex(2).toString(),"two");

        /*
         * remove()
         */
        ((JSObjectPropertiesMap)map).getJSObject().property("cantremoveme",3,JSObject.JSPropertyAttributeDontDelete);
        map.remove("double");
        map.remove("cantremoveme");
        assertNull(map.get("double"));
        assertEquals(map.get("cantremoveme"),3);

        /*
         * putAll()
         */
        Map<String,Object> map3 = new HashMap<>();
        map3.put("frommap3_1",1);
        map3.put("frommap3_2",2);
        map3.put("frommap3_3",3);
        map.putAll(map3);
        assertEquals(map.get("frommap3_1"),1);
        assertEquals(map.get("frommap3_2"),2);
        assertEquals(map.get("frommap3_3"),3);
        assertEquals(map.get("mutt"),"jeff");

        /*
         * keySet()
         */
        Set<String> keys = map.keySet();
        Set<String> keys2 = map2.keySet();
        assertTrue(keys.contains("float"));
        assertTrue(keys2.contains("2"));
        assertFalse(keys.contains("notme"));
        assertFalse(keys2.contains("5"));

        /*
         * values()
         */
        Collection<Object> values = map.values();
        Collection<String> values2 = map2.values();
        assertThat(values2.size(),is(3));
        assertThat(values2.iterator().next(),is("zero"));
        assertTrue(values.contains(3.3));
        assertTrue(values.contains(1));
        assertTrue(values.contains("a string"));
        assertFalse(values.contains(2.2));
        assertFalse(values.contains("foo"));
        assertTrue(values2.contains("zero"));
        assertTrue(values2.contains("two"));
        assertFalse(values2.contains("1"));

        /*
         * entrySet()
         */
        Set<Map.Entry<String,Object>> entrySet = map.entrySet();
        boolean gotit = false;
        for (Map.Entry<String,Object> entry : entrySet) {
            gotit = gotit || (entry.getKey().equals("float") && entry.getValue().equals(3.3));
            entry.setValue("cleared");
        }
        assertTrue(gotit);
        for (Map.Entry<String,Object> entry : entrySet) {
            assertEquals(entry.getValue().toString(),"cleared");
        }
        Iterator<Map.Entry<String,Object>> it = entrySet.iterator();
        int size = entrySet.size();
        it.next();
        it.remove();
        assertEquals(entrySet.size(),size-1);

        /*
         * clear()
         */
        map.clear();
        map2.clear();
        assertThat(map.size(),is(1));
        assertThat(map2.size(),is(0));
    }

    @org.junit.After
    public void shutDown() {
        Runtime.getRuntime().gc();
    }
}