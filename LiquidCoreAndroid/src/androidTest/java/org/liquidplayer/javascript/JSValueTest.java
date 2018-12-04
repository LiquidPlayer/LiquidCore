/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSValueTest {

    @org.junit.Test
    public void testJSValueConstructors() throws Exception {
        JSContext context = new JSContext();
        testJSValueConstructors(context);
    }

    public void testJSValueConstructors(JSContext context) throws Exception {
        /**
         * new JSValue(context)
         */
        JSValue undefined = new JSValue(context);
        assertTrue(undefined.isUndefined());

        /**
         * new JSValue(context,null)
         */
        JSValue NULL = new JSValue(context,null);
        assertTrue(NULL.isNull());

        /**
         * new JSValue(context,boolean)
         */
        JSValue bool = new JSValue(context,false);
        assertTrue(bool.isBoolean());

        /**
         * new JSValue(context,number)
         */
        JSValue integer = new JSValue(context,50);
        JSValue doub = new JSValue(context,50.0);
        JSValue lng = new JSValue(context,50L);
        JSValue flt = new JSValue(context,50.f);
        assertTrue(integer.isNumber());
        assertTrue(doub.isNumber());
        assertTrue(lng.isNumber());
        assertTrue(flt.isNumber());

        /**
         * new JSValue(context,string)
         */
        JSValue str1 = new JSValue(context,"This is a string");
        assertTrue(str1.isString());

        /**
         * new JSValue(context,Object)
         */
        JSValue alsoUndefined = new JSValue(context,new Object());
        assertTrue(alsoUndefined.isUndefined());

        /**
         * new JSValue(context,Map)
         */
        Map<String,Integer> map = new HashMap<>();
        map.put("one",1);
        map.put("two",2);
        JSValue mapValue = new JSValue(context,map);
        assertTrue(mapValue.isObject());
        assertEquals(mapValue.toObject().property("two").toNumber().intValue(),2);

        /**
         * new JSValue(context,List)
         */
        List<String> list = new ArrayList<>();
        list.add("first");
        list.add("second");
        JSValue listValue = new JSValue(context,list);
        assertTrue(listValue.isArray());
        assertEquals(listValue.toJSArray().get(1),"second");

        /**
         * new JSValue(context,Array)
         */
        String [] array = new String[] {"first", "second", "third"};
        JSValue arrayValue = new JSValue(context,array);
        assertTrue(arrayValue.isArray());
        assertEquals(arrayValue.toJSArray().get(2),"third");

        JSValue undefined2 = new JSValue((JNIJSValue)null,context);
        assertTrue(undefined2.isUndefined());
    }

    @org.junit.Test
    public void testJSValueTesters() throws Exception {
        JSContext context = new JSContext();
        testJSValueTesters(context);
    }

    public void testJSValueTesters(JSContext context) throws Exception {
        final String script =
                "var undefined; \n" +
                        "var NULL = null; \n" +
                        "var bool = true; \n" +
                        "var number = 15.6; \n" +
                        "var string = 'string'; \n" +
                        "var object = {}; \n" +
                        "var array = []; \n" +
                        "var date = new Date(); \n" +
                        "";
        context.evaluateScript(script);
        assertFalse(context.property("undefined").isNull());
        assertFalse(context.property("undefined").isBoolean());
        assertFalse(context.property("undefined").isNumber());
        assertFalse(context.property("undefined").isString());
        assertFalse(context.property("undefined").isArray());
        assertFalse(context.property("undefined").isDate());
        assertFalse(context.property("undefined").isObject());
        assertTrue(context.property("undefined").isUndefined());

        assertFalse(context.property("NULL").isUndefined());
        assertFalse(context.property("NULL").isBoolean());
        assertFalse(context.property("NULL").isNumber());
        assertFalse(context.property("NULL").isString());
        assertFalse(context.property("NULL").isArray());
        assertFalse(context.property("NULL").isDate());
        assertFalse(context.property("NULL").isObject());
        assertTrue(context.property("NULL").isNull());

        assertFalse(context.property("bool").isUndefined());
        assertFalse(context.property("bool").isNumber());
        assertFalse(context.property("bool").isString());
        assertFalse(context.property("bool").isArray());
        assertFalse(context.property("bool").isDate());
        assertFalse(context.property("bool").isObject());
        assertFalse(context.property("bool").isNull());
        assertTrue(context.property("bool").isBoolean());

        assertFalse(context.property("number").isUndefined());
        assertFalse(context.property("number").isString());
        assertFalse(context.property("number").isArray());
        assertFalse(context.property("number").isDate());
        assertFalse(context.property("number").isObject());
        assertFalse(context.property("number").isNull());
        assertFalse(context.property("number").isBoolean());
        assertTrue(context.property("number").isNumber());

        assertFalse(context.property("string").isUndefined());
        assertFalse(context.property("string").isArray());
        assertFalse(context.property("string").isDate());
        assertFalse(context.property("string").isObject());
        assertFalse(context.property("string").isNull());
        assertFalse(context.property("string").isBoolean());
        assertFalse(context.property("string").isNumber());
        assertTrue(context.property("string").isString());

        assertFalse(context.property("object").isUndefined());
        assertFalse(context.property("object").isArray());
        assertFalse(context.property("object").isDate());
        assertFalse(context.property("object").isNull());
        assertFalse(context.property("object").isBoolean());
        assertFalse(context.property("object").isNumber());
        assertFalse(context.property("object").isString());
        assertTrue(context.property("object").isObject());

        assertFalse(context.property("array").isUndefined());
        assertFalse(context.property("array").isDate());
        assertFalse(context.property("array").isNull());
        assertFalse(context.property("array").isBoolean());
        assertFalse(context.property("array").isNumber());
        assertFalse(context.property("array").isString());
        assertTrue(context.property("array").isObject());
        assertTrue(context.property("array").isArray());

        assertFalse(context.property("date").isUndefined());
        assertFalse(context.property("date").isNull());
        assertFalse(context.property("date").isBoolean());
        assertFalse(context.property("date").isNumber());
        assertFalse(context.property("date").isString());
        assertFalse(context.property("date").isArray());
        assertTrue(context.property("date").isObject());
        assertTrue(context.property("date").isDate());

        final String script2 =
                "var foo = function() {}; var bar = new foo();";
        context.evaluateScript(script2);
        assertTrue(context.property("bar").isInstanceOfConstructor(context.property("foo").toObject()));
        assertFalse(context.property("foo").isInstanceOfConstructor(context.property("bar").toObject()));
    }

    @org.junit.Test
    public void testJSValueComparators() throws Exception {
        JSContext context = new JSContext();
        testJSValueComparators(context);
    }

    public void testJSValueComparators(JSContext context) throws Exception {
        context.property("number",42f);
        assertEquals(context.property("number").toNumber().longValue(),42L);
        assertNotEquals(context.property("number").toNumber().intValue(),43);

        context.evaluateScript("string = 'string12345';");
        assertEquals(context.property("string").toString(),"string12345");
        assertNotEquals(context.property("string"),context.property("number"));

        context.evaluateScript("var another_number = 42");
        assertEquals(context.property("number"),context.property("another_number"));

        assertFalse(new JSValue(context,0).toBoolean());
        assertFalse(new JSValue(context,0).isStrictEqual(false));
        assertEquals(new JSValue(context,1).toString(),"1");
        assertFalse(new JSValue(context,1).isStrictEqual("1"));
        assertEquals(new JSValue(context,1),new JSValue(context,1.0));
        assertTrue(new JSValue(context,1).isStrictEqual(1.0));
        assertFalse(context.evaluateScript("(function () { var foo; return foo === null; })()").toBoolean());
        assertEquals(new JSValue(context),new JSValue(context,null));
        assertFalse(new JSValue(context).isStrictEqual(null));
        assertEquals(new JSValue(context,null),(new JSValue(context)));
        assertFalse(new JSValue(context,null).isStrictEqual(new JSValue(context)));
    }

    @org.junit.Test
    public void testJSValueGetters() throws Exception {
        JSContext context = new JSContext();
        testJSValueGetters(context);
    }

    public void testJSValueGetters(JSContext context) throws Exception {
        final String script =
                "var undefined; \n" +
                        "var NULL = null; \n" +
                        "var bool = true; \n" +
                        "var number = 15.6; \n" +
                        "var string = 'string'; \n" +
                        "var object = {}; \n" +
                        "var array = []; \n" +
                        "var date = new Date(1970,10,30); \n" +
                        "var func = function(x) {return x+1;};" +
                        "";
        context.evaluateScript(script);
        JSValue undefined = context.property("undefined");
        JSValue NULL = context.property("NULL");
        JSValue bool = context.property("bool");
        JSValue number = context.property("number");
        JSValue string = context.property("string");
        JSValue object = context.property("object");
        JSValue array = context.property("array");
        JSValue date = context.property("date");
        JSValue func = context.property("func");
        assertFalse(undefined.toBoolean());
        assertFalse(NULL.toBoolean());
        assertTrue(bool.toBoolean());
        assertTrue(number.toBoolean());
        assertTrue(string.toBoolean());
        assertTrue(object.toBoolean());
        assertTrue(array.toBoolean());
        assertTrue(date.toBoolean());
        assertTrue(func.toBoolean());

        assertEquals(NULL.toNumber().intValue(),0);
        assertEquals(bool.toNumber().intValue(),1);
        assertTrue(number.toNumber().equals(15.6));
        assertTrue(context.evaluateScript("'11.5'").toNumber().equals(11.5));
        assertTrue(undefined.toNumber().isNaN());
        assertTrue(string.toNumber().isNaN());
        assertTrue(object.toNumber().isNaN());
        assertTrue(func.toNumber().isNaN());
        assertTrue(array.toNumber().equals(0.0));
        assertTrue(context.evaluateScript("[1,2,3]").toNumber().isNaN());
        assertEquals(date.toNumber(),context.evaluateScript("date.getTime()").toNumber());

        assertEquals(undefined.toString(),"undefined");
        assertEquals(NULL.toString(),"null");
        assertEquals(bool.toString(),"true");
        assertEquals(context.evaluateScript("false").toString(),"false");
        assertEquals(number.toString(),"15.6");
        assertEquals(string.toString(),"string");

        assertEquals(object.toString(),"[object Object]");

        assertEquals(func.toString(),"function (x) {return x+1;}");
        assertEquals(array.toString(),"");
        assertEquals(context.evaluateScript("[1,2,3]").toString(),"1,2,3");
        assertTrue(date.toString().startsWith("Mon Nov 30 1970"));
        final String script2 =
                "var jsUndefined = JSON.stringify(undefined); \n" +
                        "var jsNULL = JSON.stringify(NULL); \n" +
                        "var jsBool = JSON.stringify(bool); \n" +
                        "var jsNumber = JSON.stringify(number); \n" +
                        "var jsString = JSON.stringify(string); \n" +
                        "var jsObject = JSON.stringify(object); \n" +
                        "var jsArray = JSON.stringify(array); \n" +
                        "var jsDate = JSON.stringify(date); \n" +
                        "var jsFunc = JSON.stringify(func); \n" +
                        "";
        context.evaluateScript(script2);
        assertEquals(bool.toJSON(),context.property("jsBool").toString());
        assertEquals(number.toJSON(),context.property("jsNumber").toString());
        assertEquals(string.toJSON(),context.property("jsString").toString());
        assertEquals(object.toJSON(),context.property("jsObject").toString());
        assertEquals(func.toJSON(),null);
        assertEquals(array.toJSON(),context.property("jsArray").toString());
        assertEquals(date.toJSON(),context.property("jsDate").toString());
        assertEquals(undefined.toJSON(),null);
        assertEquals(NULL.toJSON(),context.property("jsNULL").toString());

        /**
         * toObject()
         */
        assertNotEquals(object.toObject(),null);
        assertNotEquals(func.toObject(),null);
        assertNotEquals(array.toObject(),null);
        assertNotEquals(date.toObject(),null);

        /**
         * toFunction()
         */
        assertNotEquals(func.toFunction(),null);
        assertEquals(func.toFunction().call(null,5).toNumber().intValue(),6);

        /**
         * toJSArray()
         */
        assertNotEquals(array.toJSArray(),null);
        assertThat(array.toJSArray().size(),is(0));
    }

    @org.junit.After
    public void shutDown() {
/*
        Runtime.getRuntime().gc();
        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            Thread.interrupted();
        }
*/
    }
}