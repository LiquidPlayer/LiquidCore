/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.jstest;

import org.liquidplayer.javascript.JSArray;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSDate;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSArrayTest {

    public void testJSArrayConstructors(JSContext context) {
        /*
         * new JSArray(context, JSValue[], cls)
         */
        JSValue[] initializer = new JSValue[] { new JSValue(context,1), new JSValue(context,"two")};
        JSArray<JSValue> array = new JSArray<>(context, initializer, JSValue.class);
        assertThat(array.size(),is(2));
        assertThat(array.get(0).toNumber().intValue(),is(1));
        assertThat(array.get(1).toString(),is("two"));

        /*
         * new JSArray(context, cls)
         */
        JSArray<Integer> array2 = new JSArray<>(context,Integer.class);
        array2.add(10);
        array2.add(20);
        assertThat(array2.size(),is(2));
        assertThat(array2.get(1),is(20));

        /*
         * new JSArray(context, Object[], cls)
         */
        Object [] objinit = new Object [] { 1, 2.0, "three"};
        JSArray<JSValue> array3 = new JSArray<>(context, objinit, JSValue.class);
        assertThat(array3.size(),is(3));
        assertThat(array3.get(0).toString(),is("1"));
        assertThat(array3.get(1).isStrictEqual(2),is(true));
        assertThat(array3.get(2).isStrictEqual("three"),is(true));

        /*
         * new JSArray(context, List, cls)
         */
        List<String> list = new ArrayList<>();
        list.add("first");
        list.add("second");
        JSArray<String> array4 = new JSArray<>(context,list,String.class);
        assertThat(array4.size(),is(2));
        assertThat(array4.get(0),is("first"));
        assertThat(array4.get(1),is("second"));
    }

    public void testJSArrayListMethods(JSContext context) {
        List<Object> list = new JSArray<>(context, Object.class);
        /*
         * JSArray.add(value)
         */
        list.add("zero");
        list.add(1);
        list.add(2.0);
        list.add(new Integer[]{3});
        list.add(new JSObject(context));
        assertThat(list.get(0).toString(),is("zero"));
        assertEquals(list.get(1),1);
        assertEquals(list.get(2),2);
        assertTrue(((JSValue) list.get(3)).isArray());
        assertTrue(((JSValue) list.get(4)).isObject());

        /*
         * JSArray.toArray()
         */
        Object[] array = list.toArray();
        assertEquals(array[0],"zero");
        assertEquals(array[1],1);
        assertEquals(array[2],2);
        assertTrue(((JSValue) array[3]).isArray());
        assertTrue(((JSValue) array[4]).isObject());

        /*
         * JSArray.get(index)
         */
        ((JSArray) list).propertyAtIndex(list.size(), "anotherone");
        assertEquals(list.get(5),"anotherone");
        assertTrue(((JSValue) list.get(3)).isArray());

        /*
         * JSArray.size()
         */
        assertThat(list.size(),is(6));

        /*
         * JSArray.isEmpty()
         */
        List<Integer> list2 = new JSArray<>(context, Integer.class);
        assertFalse(list.isEmpty());
        assertTrue(list2.isEmpty());

        /*
         * JSArray.contains(object)
         */
        assertTrue(list.contains("zero"));
        assertTrue(list.contains(1));
        assertTrue(list.contains(2.0));
        assertFalse(list.contains(5));

        /*
         * JSArray.iterator()
         */
        int i = 0;
        for (Iterator<Object> it = list.iterator(); it.hasNext(); i++) {
            Object next = it.next();
            assertTrue(list.contains(next));
        }
        assertThat(i,is(list.size()));

        /*
         * JSArray.toArray(Object[])
         */
        list2.add(0);
        list2.add(1);
        list2.add(2);
        Integer[] arr1 = new Integer[3];
        Integer[] arr2 = list2.toArray(arr1);
        assertArrayEquals(arr2,new Integer[] {0,1,2});

        list2.add(3);
        arr2 = list2.toArray(arr1);
        assertArrayEquals(arr2,new Integer[] {0,1,2,3});

        list2.remove(3);
        list2.remove(2);
        arr2 = list2.toArray(arr1);
        assertThat(arr2[0],is(0));
        assertThat(arr2[1],is(1));
        assertThat(arr2.length, is(2));

        /*
         * JSArray.remove(object)
         */
        assertTrue(list2.remove(Integer.valueOf(1)));
        assertFalse(list2.remove(Integer.valueOf(2)));
        assertFalse(list2.contains(1));

        /*
         * JSArray.containsAll(collection)
         */
        Collection<Object> collection = new ArrayList<>();
        collection.add("zero");
        collection.add(1);
        collection.add(2);
        Collection<Object> collection2 = new ArrayList<>(collection);
        collection2.add(25.0);
        assertTrue(list.containsAll(collection));
        assertFalse(list.containsAll(collection2));

        /*
         * JSArray.addAll(collection)
         */
        int size = list.size();
        list.addAll(collection);
        assertThat(list.size(),is(size + collection.size()));

        /*
         * JSArray.removeAll(collection)
         */
        size = list.size();
        list.removeAll(collection);
        assertThat(list.size(),is(size - collection.size() * 2));

        /*
         * JSArray.retainAll(collection)
         */
        list.addAll(collection);
        list.retainAll(collection);
        assertThat(list.size(),is(collection.size()));
        assertTrue(list.containsAll(collection));

        /*
         * JSArray.clear()
         */
        list.clear();
        assertThat(list.size(),is(0));

        /*
         * JSArray.set(index,object)
         */
        list.addAll(collection);
        Object last1;
        try {
            list.set(10, "bar");
            last1 = 0;
        } catch (IndexOutOfBoundsException e) {
            last1 = list.set(1, "foo");
        }
        assertEquals(last1,1);
        assertEquals(list.get(1),"foo");

        /*
         * JSArray.add(index,object)
         */
        list.add(1, "hello");
        list.add(4, "world");
        boolean exception = false;
        try {
            list.add(10, 10.0);
        } catch (IndexOutOfBoundsException e) {
            exception = true;
        }
        assertTrue(exception);
        assertEquals(list.get(1),"hello");
        assertEquals(list.get(2),"foo");
        assertThat(list.size(),is(5));
        assertEquals(list.get(4),"world");

        /*
         * JSArray.remove(index)
         */
        list.remove(4);
        list.remove(1);
        assertEquals(list.get(1),"foo");
        assertThat(list.size(),is(3));

        /*
         * JSArray.indexOf(object)
         */
        list.addAll(collection);
        assertThat(list.indexOf("zero"),is(0));
        assertThat(list.indexOf("foo"),is(1));
        assertThat(list.indexOf(2),is(2));
        assertThat(list.indexOf(1),is(4));
        assertThat(list.indexOf("world"),is(-1));

        /*
         * JSArray.lastIndexOf(object)
         */
        assertThat(list.lastIndexOf("zero"),is(3));
        assertThat(list.lastIndexOf("foo"),is(1));
        assertThat(list.lastIndexOf(2),is(5));
        assertThat(list.lastIndexOf(1),is(4));
        assertThat(list.lastIndexOf("world"),is(-1));

        /*
         * JSArray.listIterator()
         */
        // List iterator is heavily used by underlying JSArray methods already tested.  Only
        // underlying methods untested are 'set' and 'add'
        for (ListIterator<Object> it = list.listIterator(); it.hasNext(); ) {
            Object dupe = it.next();
            it.set("changed");
            it.add(dupe);
        }
        assertThat(list.size(),is(12));
        assertThat(list.indexOf("changed"),is(0));
        assertThat(list.lastIndexOf("changed"),is(10));

        /*
         * JSArray.listIterator(index)
         */
        for (ListIterator<Object> it = list.listIterator(0); it.hasNext(); ) {
            if (it.next().equals("changed")) it.remove();
        }
        assertEquals(list.listIterator(list.size()).previous(),list.listIterator(list.size() + 10).previous());
        assertThat(list.size(),is(6));

        /*
         * JSArray.subList(fromIndex, toIndex)
         */
        list.subList(1, 4).clear();
        assertThat(list.size(),is(3));
        assertEquals(list.get(0),"zero");
        assertEquals(list.get(1),1);
        assertEquals(list.get(2),2);
        list.subList(1,2).set(0,3);
        assertEquals(list.get(1),3);
        list.subList(1,2).set(0,1);

        /*
         * JSArray.equals()
         */
        ArrayList<Object> arrayList = new ArrayList<>(collection);
        assertEquals(list,arrayList);
        assertNotEquals(list,list2);

        /*
         * JSArray.hashCode()
         */
        JSArray<Object> hashList = new JSArray<>(context, collection, Object.class);
        ArrayList<Object> arrayList2 = new ArrayList<>();
        arrayList2.add("zero");
        arrayList2.add(1.0); // <-- Note: making these Doubles is necessary for hashCode match
        arrayList2.add(2.0); // <--
        assertThat(list.hashCode(),is(hashList.hashCode()));
        assertThat(list.hashCode(),not(list2.hashCode()));
        assertThat(list.hashCode(),is(arrayList2.hashCode()));
        assertEquals(list,arrayList2);
    }

    @SuppressWarnings("unused")
    public void testJSArrayJSMethods(final JSContext context) {
        // Array.from()
        JSArray<JSValue> from = JSArray.from(context,"foo");

        String [] foo = { "f", "o", "o" };

        assertEquals(new JSArray<>(context,from,String.class),Arrays.asList(foo));

        JSArray<JSValue> from2 = JSArray.from(context, "foo", new JSArray.MapCallback<JSValue>() {
            @Override
            public JSValue callback(JSValue currentValue, int index, JSArray<JSValue> array) {
                return new JSValue(context,currentValue.toString().toUpperCase());
            }
        });
        String [] FOO = { "F", "O", "O" };
        assertEquals(new JSArray<>(context,from2,String.class),Arrays.asList(FOO));

        JSArray<JSValue> from3 = JSArray.from(context, "foo", new JSFunction(context,"_from") {
            public String _from(String currentValue) {
                return currentValue.toUpperCase();
            }
        });
        assertEquals(new JSArray<>(context,from3,String.class),Arrays.asList(FOO));

        final JSObject bar = new JSObject(context);
        bar.property("string",JSArray.from(context,"BAR"));
        bar.property("_from", new JSFunction(context,"_from") {
            public String _from(String currentValue, int index) {
                return getThis().property("string").toJSArray().get(index).toString();
            }
        });
        bar.property("_from").toFunction();
        JSArray<JSValue> from4 = JSArray.from(context,foo,bar.property("_from").toFunction(),bar);

        String [] BAR = { "B", "A", "R" };
        assertEquals(new JSArray<>(context,from4,String.class),Arrays.asList(BAR));

        // Array.isArray()
        assertTrue(JSArray.isArray(from));
        assertFalse(JSArray.isArray(new JSValue(context,5)));

        // Array.of()
        JSArray<JSValue> of = JSArray.of(context,1,2,3);
        Integer [] bar2 = { 1, 2, 3 };
        assertEquals(new JSArray<>(context,of,Integer.class),Arrays.asList(bar2));

        // Array.prototype.concat()
        JSArray<JSValue> concat = JSArray.of(context,"first");
        concat = concat.concat(from,of,50);
        assertThat(concat.size(),is(8));
        assertEquals(concat.get(2),new JSValue(context,"o"));
        assertEquals(concat.get(5),new JSValue(context,2));
        assertEquals(concat.get(7),new JSValue(context,50));

        // Array.prototype.copyWithin()
        JSArray copyWithin = JSArray.of(context,1,2,3,4,5);
        JSArray copyWithin2 = copyWithin.copyWithin(-2);
        Integer [] copyWithin3 = new Integer [] { 1,2,3,1,2 };
        assertEquals(copyWithin,Arrays.asList(copyWithin3));
        assertEquals(copyWithin2,Arrays.asList(copyWithin3));

        JSArray copyWithin4 = JSArray.of(context,1,2,3,4,5);
        JSArray copyWithin5 = copyWithin4.copyWithin(0,3,5);
        Integer [] copyWithin6 = new Integer [] { 4,5,3,4,5 };
        assertEquals(copyWithin4,Arrays.asList(copyWithin6));
        assertEquals(copyWithin5,Arrays.asList(copyWithin6));

        JSArray copyWithin7 = JSArray.of(context,1,2,3,4,5);
        JSArray copyWithin8 = copyWithin7.copyWithin(0,3);
        Integer [] copyWithin9 = new Integer [] { 4,5,3,4,5 };
        assertEquals(copyWithin7,Arrays.asList(copyWithin9));
        assertEquals(copyWithin8,Arrays.asList(copyWithin9));

        // Array.prototype.entries()
        JSArray<Integer> entriesArray = new JSArray<>(context,copyWithin3,Integer.class);
        Iterator<Map.Entry<Integer,Integer>> entries = entriesArray.entries();
        assertThat(entries.next().getValue(),is(1));
        assertThat(entries.next().getValue(),is(2));

        JSObject fakeThis = new JSObject(context);
        fakeThis.property("foo", "bar");

        // Array.prototype.every()
        JSArray<Integer> every1 = new JSArray<>(context, JSArray.of(context,12,5,8,130,44), Integer.class);
        JSArray<Integer> every2 = new JSArray<>(context, JSArray.of(context,12,54,18,130,44), Integer.class);
        assertFalse(
                every1.every(new JSArray.EachBooleanCallback<Integer>() {
                    @Override
                    public boolean callback(Integer value, int i, JSArray<Integer> jsArray) {
                        return value >= 10;
                    }
                }));
        assertTrue(every2.every(new JSFunction(context,"callback",new String [] {"integer"},
                "return integer >= 10;", null, 0)));
        assertTrue(every2.every(new JSFunction(context,"callback",new String [] {"integer"},
                "return this.foo === 'bar';", null, 0), fakeThis));

        // Array.prototype.fill()
        JSArray<Integer> fillArray = new JSArray<>(context,copyWithin3,Integer.class);
        JSArray<Integer> fillArray2 = fillArray.fill(4,1);
        Integer [] fillCompare = new Integer [] { 1,4,4,4,4 };
        assertEquals(fillArray,Arrays.asList(fillCompare));
        assertEquals(fillArray2,Arrays.asList(fillCompare));
        fillCompare = new Integer [] { 5,5,5,5,5 };
        assertEquals(fillArray, fillArray.fill(5));
        assertEquals(fillArray,Arrays.asList(fillCompare));
        JSArray<Integer> fillArray3 = fillArray.fill(6,1, 3);
        fillCompare = new Integer [] { 5,6,6,5,5 };
        assertEquals(fillArray, fillArray3);
        assertEquals(fillArray,Arrays.asList(fillCompare));

        // Array.prototype.filter()
        JSArray<Integer> filtered =
                every1.filter(new JSArray.EachBooleanCallback<Integer>() {
                    @Override
                    public boolean callback(Integer value, int i, JSArray<Integer> jsArray) {
                        return value >= 10;
                    }
                });
        assertEquals(filtered,Arrays.asList(12,130,44));
        JSArray<Integer> filtered2 =
                every1.filter(new JSFunction(context,"filter") {
                    @SuppressWarnings("unused")
                    public boolean filter(Integer value) {
                        return value >= 10;
                    }
                });
        assertEquals(filtered2,Arrays.asList(12,130,44));
        JSArray<Integer> filtered3 =
                every1.filter(new JSFunction(context,"filter") {
                    @SuppressWarnings("unused")
                    public boolean filter(Integer value) {
                        assertEquals("bar", getThis().property("foo").toString());
                        return value >= 10;
                    }
                }, fakeThis);
        assertEquals(filtered3,Arrays.asList(12,130,44));

        // Array.prototype.find()
        Map<String,Object> map1 = new HashMap<>();
        Map<String,Object> map2 = new HashMap<>();
        Map<String,Object> map3 = new HashMap<>();
        map1.put("name", "apples");  map2.put("name", "bananas");  map3.put("name", "cherries");
        map1.put("quantity", 2);     map2.put("quantity", 0);      map3.put("quantity", 5);

        JSArray<Map> inventory = new JSArray<>(context,Arrays.asList(map1,map2,map3),Map.class);
        Map cherries = inventory.find(new JSArray.EachBooleanCallback<Map>() {
            @Override
            public boolean callback(Map map, int i, JSArray<Map> jsArray) {
                Object name = map.get("name");
                assertNotNull(name);
                return name.equals("cherries");
            }
        });
        assertEquals(cherries.get("quantity"),5);
        Map cherries2 = inventory.find(new JSFunction(context,"find") {
            @SuppressWarnings("unused")
            public boolean find(Map map) {
                Object name = map.get("name");
                assertNotNull(name);
                return name.equals("cherries");
            }
        });
        assertEquals(cherries2.get("quantity"),5);
        Map cherries3 = inventory.find(new JSFunction(context,"find") {
            @SuppressWarnings("unused")
            public boolean find(Map map) {
                Object name = map.get("name");
                assertNotNull(name);
                assertEquals("bar", getThis().property("foo").toString());
                return name.equals("cherries");
            }
        },fakeThis);
        assertEquals(cherries3.get("quantity"),5);

        // Array.prototype.findIndex()
        JSFunction isPrime = new JSFunction(context,"isPrime") {
            @SuppressWarnings("unused")
            public boolean isPrime(Integer element) {
                int start = 2;
                while (start <= Math.sqrt(element)) {
                    if (element % start++ < 1) {
                        return false;
                    }
                }
                return element > 1;
            }
        };
        JSArray.EachBooleanCallback<Integer> isPrime2 =
                new JSArray.EachBooleanCallback<Integer>() {
            @Override
            public boolean callback(Integer currentValue, int index, JSArray<Integer> array) {
                int start = 2;
                while (start <= Math.sqrt(currentValue)) {
                    if (currentValue % start++ < 1) {
                        return false;
                    }
                }
                return currentValue > 1;
            }
        };
        JSFunction isPrime3 = new JSFunction(context,"isPrime") {
            @SuppressWarnings("unused")
            public boolean isPrime(Integer element) {
                assertEquals("bar", getThis().property("foo").toString());
                int start = 2;
                while (start <= Math.sqrt(element)) {
                    if (element % start++ < 1) {
                        return false;
                    }
                }
                return element > 1;
            }
        };

        JSArray<Integer> notPrime = new JSArray<>(context,Arrays.asList(4,6,8,12),Integer.class);
        JSArray<Integer> sevenPrime = new JSArray<>(context,Arrays.asList(4,6,7,12),Integer.class);
        assertThat(notPrime.findIndex(isPrime),is(-1));
        assertThat(sevenPrime.findIndex(isPrime),is(2));
        assertThat(notPrime.findIndex(isPrime2),is(-1));
        assertThat(sevenPrime.findIndex(isPrime2),is(2));
        assertThat(notPrime.findIndex(isPrime3, fakeThis),is(-1));
        assertThat(sevenPrime.findIndex(isPrime3, fakeThis),is(2));

        // Array.prototype.forEach()
        final HashMap<Integer,Integer> forEachMap = new HashMap<>();
        notPrime.forEach(new JSArray.ForEachCallback<Integer>() {
            @Override
            public void callback(Integer integer, int i, JSArray<Integer> jsArray) {
                forEachMap.put(i,integer);
            }
        });
        assertThat(forEachMap.size(),is(4));
        assertThat(forEachMap.get(0),is(4));
        assertThat(forEachMap.get(3),is(12));

        final HashMap<Integer,Integer> forEachMap2 = new HashMap<>();
        notPrime.forEach(new JSFunction(context,"forEach") {
            @SuppressWarnings("unused")
            public void forEach(Integer integer, int i) {
                forEachMap2.put(i,integer);
            }
        });
        assertThat(forEachMap2.size(),is(4));
        assertThat(forEachMap2.get(0),is(4));
        assertThat(forEachMap2.get(3),is(12));

        final HashMap<Integer,Integer> forEachMap3 = new HashMap<>();
        notPrime.forEach(new JSFunction(context,"forEach") {
            @SuppressWarnings("unused")
            public void forEach(Integer integer, int i) {
                assertEquals("bar", getThis().property("foo").toString());
                forEachMap3.put(i,integer);
            }
        }, fakeThis);
        assertThat(forEachMap3.size(),is(4));
        assertThat(forEachMap3.get(0),is(4));
        assertThat(forEachMap3.get(3),is(12));

        // Array.prototype.includes()
        assertFalse(notPrime.includes(7));
        assertTrue(sevenPrime.includes(7));
        assertFalse(sevenPrime.includes(7,3));

        // Array.prototype.indexOf()
        assertThat(notPrime.indexOf(8),is(notPrime.indexOf(8,0)));

        // Array.prototype.join()
        assertEquals(sevenPrime.join("|"),"4|6|7|12");
        assertEquals(sevenPrime.join(),"4,6,7,12");

        // Array.prototype.keys()
        JSArray<String> keysArr = new JSArray<>(context,String.class);
        keysArr.propertyAtIndex(0,"Zero");
        keysArr.propertyAtIndex(1,"One");
        keysArr.propertyAtIndex(1000,"Thousand");
        Iterator<Integer> keys = keysArr.keys();
        assertThat(keys.next(),is(0));
        assertThat(keys.next(),is(1));
        assertThat(keys.next(),is(2));

        Iterator<Integer> keys2 = sevenPrime.keys();
        assertThat(keys2.next(),is(0));
        assertThat(keys2.next(),is(1));
        assertThat(keys2.next(),is(2));
        assertThat(keys2.next(),is(3));
        assertNull(keys2.next());

        // Array.prototype.lastIndexOf()
        assertThat(notPrime.lastIndexOf(8),is(notPrime.lastIndexOf(8,notPrime.size()-1)));

        // Array.prototype.pop()
        String popped = keysArr.pop();
        assertEquals(popped,"Thousand");

        // Array.prototype.push()
        assertEquals(1002, keysArr.push("One-Thousand","One-Thousand One"));
        assertThat(keysArr.size(),is(1002));
        assertEquals(keysArr.pop(),"One-Thousand One");

        // Array.prototype.map()
        JSArray<JSValue> inventoryMap = inventory
                .map(new JSArray.MapCallback<Map>() {
                    @Override
                    public JSValue callback(Map map, int i, JSArray<Map> jsArray) {
                        return new JSValue(context,map.get("quantity"));
                    }
                });
        assertEquals(new JSArray<>(context,inventoryMap,Integer.class),Arrays.asList(2,0,5));

        // Array.prototype.reduce()
        int inventoryCount = inventoryMap
                .reduce(new JSArray.ReduceCallback() {
                    @Override
                    public JSValue callback(JSValue jsValue, JSValue jsValue1, int i,
                                            JSArray<JSValue> jsArray) {
                        return new JSValue(context,jsValue.toNumber() + jsValue1.toNumber());
                    }
                }).toNumber().intValue();
        assertThat(inventoryCount,is(7));

        int inventoryCount2 = inventoryMap
                .reduce(new JSFunction(context,"reduce") {
                    @SuppressWarnings("unused")
                    public Integer reduce(Integer value, Integer value1) {
                        return value + value1;
                    }
                }).toNumber().intValue();
        assertThat(inventoryCount2,is(7));

        int inventoryCount3 = inventoryMap
                .reduce(new JSFunction(context,"reduce") {
                    @SuppressWarnings("unused")
                    public Integer reduce(Integer value, Integer value1) {
                        return value + value1;
                    }
                },1).toNumber().intValue();
        assertThat(inventoryCount3,is(8));

        int inventoryCount4 = inventoryMap
                .reduce(new JSArray.ReduceCallback() {
                    @Override
                    public JSValue callback(JSValue jsValue, JSValue jsValue1, int i,
                                            JSArray<JSValue> jsArray) {
                        return new JSValue(context,jsValue.toNumber() + jsValue1.toNumber());
                    }
                }, 1).toNumber().intValue();
        assertThat(inventoryCount4,is(8));

        // Array.prototype.reduceRight()
        int inventoryCountRight = inventory
                .map(new JSFunction(context,"map") {
                    @SuppressWarnings("unused")
                    public JSValue map(Map map) {
                        return (JSValue) map.get("quantity");
                    }
                })
                .reduceRight(new JSArray.ReduceCallback() {
                    @Override
                    public JSValue callback(JSValue jsValue, JSValue jsValue1, int i,
                                            JSArray<JSValue> jsArray) {
                        return new JSValue(context,jsValue.toNumber() - jsValue1.toNumber());
                    }
                },inventoryCount)
                .toNumber()
                .intValue();
        assertThat(inventoryCountRight,is(0));

        int inventoryCountRight2 = inventory
                .map(new JSFunction(context,"map") {
                    @SuppressWarnings("unused")
                    public JSValue map(Map map) {
                        return (JSValue) map.get("quantity");
                    }
                })
                .reduceRight(new JSFunction(context, "reduceRight") {
                    @SuppressWarnings("unused")
                    public Integer reduceRight(Integer value, Integer value1) {
                        return value - value1;
                    }
                },inventoryCount)
                .toNumber()
                .intValue();
        assertThat(inventoryCountRight2,is(0));

        int inventoryCountRight4 = inventory
                .map(new JSFunction(context,"map") {
                    @SuppressWarnings("unused")
                    public JSValue map(Map map) {
                        return (JSValue) map.get("quantity");
                    }
                })
                .reduceRight(new JSFunction(context, "reduceRight") {
                    @SuppressWarnings("unused")
                    public Integer reduceRight(Integer value, Integer value1) {
                        return value - value1;
                    }
                })
                .toNumber()
                .intValue();
        assertThat(inventoryCountRight4,is(3));

        int inventoryCount5 = inventory
                .map(new JSFunction(context,"map") {
                    @SuppressWarnings("unused")
                    public JSValue map(Map map) {
                        return (JSValue) map.get("quantity");
                    }
                })
                .reduce(new JSFunction(context, "reduce") {
                    @SuppressWarnings("unused")
                    public Integer reduce(Integer value, Integer value1) {
                        return value - value1;
                    }
                })
                .toNumber()
                .intValue();
        assertThat(inventoryCount5,is(-3));

        int inventoryCountRight3 = inventory
                .map(new JSFunction(context,"map") {
                    @SuppressWarnings("unused")
                    public JSValue map(Map map) {
                        return (JSValue) map.get("quantity");
                    }
                })
                .reduceRight(new JSArray.ReduceCallback() {
                    @Override
                    public JSValue callback(JSValue jsValue, JSValue jsValue1, int i,
                                            JSArray<JSValue> jsArray) {
                        return new JSValue(context,jsValue.toNumber() - jsValue1.toNumber());
                    }
                })
                .toNumber()
                .intValue();
        assertThat(inventoryCountRight3,is(3));

        // Array.prototype.reverse()
        JSArray<JSValue> forward = JSArray.of(context,"one","two","three");
        JSArray<JSValue> reverse = forward.reverse();
        assertThat(forward,is(reverse));
        JSArray<String> reverseString = new JSArray<>(context,reverse,String.class);
        assertEquals(reverseString,Arrays.asList("three","two","one"));

        // Array.prototype.shift()
        assertEquals(reverseString.shift(),"three");
        assertThat(reverseString.size(),is(2));

        // Array.prototype.unshift()
        assertThat(reverseString.unshift("four","three"),is(4));

        // Array.prototype.slice()
        JSArray<String> slice = reverseString.slice(1,3);
        assertEquals(slice,Arrays.asList("three","two"));

        JSArray<String> copy = reverseString.slice();
        assertEquals(copy,reverseString);

        JSArray<String> sliceEnd = reverseString.slice(2);
        assertEquals(sliceEnd,Arrays.asList("two","one"));

        // Array.prototype.some()
        assertTrue(sevenPrime.some(new JSFunction(context,"some") {
            @SuppressWarnings("unused")
            public boolean some(JSValue value) {
                return value.toString().equals("7");
            }
        }));
        assertFalse(notPrime.some(new JSArray.EachBooleanCallback<Integer>() {
            @Override
            public boolean callback(Integer integer, int i, JSArray<Integer> jsArray) {
                return integer==7;
            }
        }));
        assertTrue(sevenPrime.some(new JSFunction(context,"some") {
            @SuppressWarnings("unused")
            public boolean some(JSValue value) {
                assertEquals("bar", getThis().property("foo").toString());
                return value.toString().equals("7");
            }
        }, fakeThis));

        // Array.prototype.sort()
        assertEquals(reverseString.sort(),Arrays.asList("four","one","three","two"));
        assertEquals(notPrime.sort(new JSArray.SortCallback<Integer>() {
            @Override
            public double callback(Integer t1, Integer t2) {
                return t2 - t1;
            }
        }),Arrays.asList(12,8,6,4));
        assertEquals(sevenPrime.sort(new JSFunction(context,"sort") {
            @SuppressWarnings("unused")
            public Integer sort(Integer t1, Integer t2) {
                return t2 - t1;
            }
        }),Arrays.asList(12,7,6,4));

        // Array.prototype.splice()
        JSArray<Integer> spliced = notPrime.splice(1,2,11,10,9);
        assertEquals(spliced,Arrays.asList(8,6));
        assertEquals(notPrime,Arrays.asList(12,11,10,9,4));

        // Array.prototype.toLocaleString()
        JSDate date = new JSDate(context);
        JSArray<JSValue> locale = JSArray.of(context,337,date,"foo");
        String dateLocale = date.property("toLocaleString").toFunction().call(date).toString();
        String toLocale = locale.toLocaleString();
        assertEquals(toLocale,"337," + dateLocale + ",foo");
    }

    @org.junit.Test
    public void testJSArrayConstructors() {
        JSContext context = new JSContext();
        testJSArrayConstructors(context);
    }

    @org.junit.Test
    public void testJSArrayListMethods() {
        final JSContext context = new JSContext();
        testJSArrayListMethods(context);
    }

    @org.junit.Test
    public void testJSArrayJSMethods() {
        final JSContext context = new JSContext();
        testJSArrayJSMethods(context);
    }
}