package org.liquidplayer.v8;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSObjectTest {

    public interface IFunctionObject {
        @SuppressWarnings("unused")
        void voidFunc();
        @SuppressWarnings("unused")
        JSValue jsvalueFunc();
        @SuppressWarnings("unused")
        JSObject jsobjectFunc();
        @SuppressWarnings("unused")
        Integer intFunc();
        @SuppressWarnings("unused")
        int intFunc2();
        @SuppressWarnings("unused")
        Long longFunc();
        @SuppressWarnings("unused")
        long longFunc2();
        @SuppressWarnings("unused")
        Float floatFunc();
        @SuppressWarnings("unused")
        float floatFunc2();
        @SuppressWarnings("unused")
        Double doubleFunc();
        @SuppressWarnings("unused")
        double doubleFunc2();
        @SuppressWarnings("unused")
        String stringFunc();
        @SuppressWarnings("unused")
        Boolean booleanFunc();
        @SuppressWarnings("unused")
        Integer[] arrayFunc();
    }
    public static class FunctionObject extends JSObject implements IFunctionObject {
        private final JSContext context;
        public FunctionObject(JSContext ctx) {
            super(ctx, IFunctionObject.class);
            context = ctx;
        }

        @Override
        public void voidFunc() {

        }
        @Override
        public JSValue jsvalueFunc() {
            return new JSValue(context);
        }
        @Override
        public JSObject jsobjectFunc() {
            return new JSObject(context);
        }
        @Override
        public Integer intFunc() {
            return 5;
        }
        @Override
        public Long longFunc() {
            return 6L;
        }
        @Override
        public Float floatFunc() {
            return 7.6f;
        }
        @Override
        public Double doubleFunc() {
            return 8.8;
        }
        @Override
        public String stringFunc() {
            return "string";
        }
        @Override
        public Boolean booleanFunc() {
            return true;
        }
        @Override
        public int intFunc2() {
            return 9;
        }
        @Override
        public long longFunc2() {
            return 10L;
        }
        @Override
        public float floatFunc2() {
            return 17.6f;
        }
        @Override
        public double doubleFunc2() {
            return 18.8;
        }
        @Override
        public Integer[] arrayFunc() {
            return new Integer[] {5,6,7,8};
        }
    }

    public final static String functionObjectScript =
            "var empty = {}; \n" +
                    "var functionObject = {\n" +
                    "   voidFunc:    function() {}, \n" +
                    "   jsvalueFunc: function() { var undef; return undef; }, \n" +
                    "   jsobjectFunc:function() { return {}; }, \n" +
                    "   intFunc:     function() { return 5; }, \n" +
                    "   intFunc2:    function() { return 9; }, \n" +
                    "   longFunc:    function() { return 6; }, \n" +
                    "   longFunc2:   function() { return 10; }, \n" +
                    "   floatFunc:   function() { return 7.6; }, \n" +
                    "   floatFunc2:  function() { return 17.6; }, \n" +
                    "   doubleFunc:  function() { return 8.8; }, \n" +
                    "   doubleFunc2: function() { return 18.8; }, \n" +
                    "   stringFunc:  function() { return 'string'; }, \n" +
                    "   arrayFunc:   function() { return [5,6,7,8]; }, \n" +
                    "   booleanFunc: function() { return true; } \n" +
                    "};";

    @org.junit.Test
    public void testJSObjectConstructors() throws Exception {
        JSContext context = new JSContext();
        context.evaluateScript(functionObjectScript);

        /**
         * new JSObject(context)
         */
        JSObject empty = new JSObject(context);
        assertEquals(empty.toJSON(),context.property("empty").toJSON());

        /**
         * new JSObject(context, interface)
         */
        JSObject functionObject = new FunctionObject(context);
        JSObject functionObjectJS = context.property("functionObject").toObject();
        context.property("java", functionObject);
        String[] array1 = functionObject.propertyNames();
        String[] array2 = functionObjectJS.propertyNames();
        java.util.Arrays.sort(array1);
        java.util.Arrays.sort(array2);
        assertThat(array1.length,is(array2.length));
        for (int i=0; i<array1.length; i++)
            assertEquals(array1[i],array2[i]);

        /**
         * new JSObject(context, map)
         */
        Map<String,Integer> map = new HashMap<>();
        map.put("first",1);
        map.put("second",2);
        JSObject mapObject = new JSObject(context,map);
        assertTrue(mapObject.property("first").isStrictEqual(1));
    }

    @org.junit.Test
    public void testJSObjectProperties() throws Exception {
        JSContext context = new JSContext();

        Map<String,Object> map = new HashMap<>();
        map.put("one",1);
        map.put("two",2.0);
        map.put("string","this is a string");
        map.put("object",new JSObject(context));
        map.put("array",new JSArray<>(context, new Integer[] {1,2,3}, Integer.class));
        map.put("func",new JSFunction(context,"func") {
            @SuppressWarnings("unused")
            public int func(int x) {
                return x+1;
            }
        });
        context.evaluateScript("var object = {one:1, two:2.0, string:'this is a string', " +
                "object: {}, array:[1,2,3], func: function(x) { return x+1;} }");

        JSObject object = new JSObject(context,map);
        JSObject jsobj  = context.property("object").toObject();

        /**
         * hasProperty(property)
         */
        assertTrue(object.hasProperty("one"));
        assertTrue(jsobj.hasProperty("one"));
        assertFalse(object.hasProperty("foo"));
        assertFalse(jsobj.hasProperty("foo"));

        /**
         * property(property)
         */
        assertEquals(object.property("one").toNumber(),jsobj.property("one").toNumber());
        assertEquals(object.property("two"),jsobj.property("two"));
        assertEquals(object.property("string"),jsobj.property("string"));
        assertTrue(object.property("object").isObject());
        assertTrue(jsobj.property("object").isObject());
        assertTrue(object.property("object").isObject());
        assertTrue(jsobj.property("object").isObject());
        assertTrue(object.property("array").isArray());
        assertTrue(jsobj.property("array").isArray());
        assertTrue(object.property("func").toObject().isFunction());
        assertTrue(jsobj.property("func").toObject().isFunction());

        /**
         * property(property,value,-attributes-)
         */
        object.property("added",3);
        assertTrue(object.hasProperty("added"));
        assertEquals(object.property("added"),new JSValue(context,3));
        object.property("readonly",4,JSObject.JSPropertyAttributeReadOnly);
        object.property("readonly",5);
        assertTrue(object.hasProperty("readonly"));
        assertEquals(object.property("readonly"),new JSValue(context,4));
        object.property("dontdelete",6,JSObject.JSPropertyAttributeDontDelete);
        object.deleteProperty("dontdelete");
        assertTrue(object.hasProperty("dontdelete"));
        assertEquals(object.property("dontdelete"),new JSValue(context,6));
        object.property("noenum",7,JSObject.JSPropertyAttributeDontEnum);
        assertTrue(new JSFunction(context,"f",new String[] {"obj"},
                        "for (p in obj) if (p=='noenum') return false; return true;",null,0).call(null,object).toBoolean());
        assertTrue(object.hasProperty("noenum"));
        assertEquals(object.property("noenum"), new JSValue(context,7));

        /**
         * deleteProperty(property)
         */
        object.deleteProperty("added");
        assertFalse(object.hasProperty("added"));
        assertTrue(object.property("added").isUndefined());

        /**
         * propertyAtIndex(index,value)
         */
        object.propertyAtIndex(50,"fifty");
        assertEquals(object.property("50").toString(),"fifty");

        /**
         * propertyAtIndex(index)
         */
        assertEquals(object.propertyAtIndex(50).toString(),"fifty");
        assertTrue(object.propertyAtIndex(51).isUndefined());

        /**
         * propertyNames()
         */
        object.propertyAtIndex(52,"fifty-two");
        List<String> names = Arrays.asList(object.propertyNames());
        // readonly, dontdelete, noenum, 50, and 52 were added, but noenum is not enumerable (diff 4)
        assertThat(names.size() - jsobj.propertyNames().length,is(4));
        assertTrue(names.contains("one"));
        assertTrue(names.contains("52"));
        assertFalse(names.contains("noenum"));
    }

    @org.junit.Test
    public void testJSObjectTesters() throws Exception {
        JSContext context = new JSContext();
        final String script = "var func = function() {}; var nofunc = {}; var constr = function(x) {this.x = x;};";
        context.evaluateScript(script);
        JSFunction func = new JSFunction(context);
        JSObject nofunc = new JSObject(context);
        JSFunction constr = new JSFunction(context,"constructor") {
            @SuppressWarnings("unused")
            public void constructor(int x) {
                getThis().property("x",x);
            }
        };

        /**
         * isFunction()
         */
        assertTrue(func.isFunction());
        assertFalse(nofunc.isFunction());
        assertTrue(constr.isFunction());
        assertTrue(context.property("func").toObject().isFunction());
        assertFalse(context.property("nofunc").toObject().isFunction());
        assertTrue(context.property("constr").toObject().isFunction());

        /**
         * isConstructor()
         */
        assertTrue(func.isConstructor());
        assertFalse(nofunc.isConstructor());
        assertTrue(constr.isConstructor());
        assertTrue(context.property("func").toObject().isConstructor());
        assertFalse(context.property("nofunc").toObject().isConstructor());
        assertTrue(context.property("constr").toObject().isConstructor());
    }

    @org.junit.After
    public void shutDown() {
        Runtime.getRuntime().gc();
    }
}