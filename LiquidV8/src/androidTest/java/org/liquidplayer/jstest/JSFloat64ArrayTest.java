/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.jstest;

import org.junit.Before;
import org.junit.Test;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFloat64Array;
import org.liquidplayer.javascript.JSInt8Array;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;

import static org.hamcrest.Matchers.is;
import static org.junit.Assert.*;

public class JSFloat64ArrayTest {

    private JSContext context;

    @Before
    public void setUp() {
        context = new JSContext();
    }

    public void setUp(JSContext context) {
        this.context = context;
    }

    @Test
    public void testJSFloat64Array() {
        JSFloat64Array array = new JSFloat64Array(context,8);
        assertThat(array.byteLength(),is(8*8));
        assertThat(array.byteOffset(),is(0));
        assertTrue(array.isTypedArray());

        assertFalse(array.isInt8Array());
        assertFalse(array.isInt16Array());
        assertFalse(array.isInt32Array());
        assertFalse(array.isUint8Array());
        assertFalse(array.isUint8ClampedArray());
        assertFalse(array.isUint16Array());
        assertFalse(array.isUint32Array());
        assertFalse(array.isFloat32Array());
        assertTrue(array.isFloat64Array());

        assertEquals(array.property("BYTES_PER_ELEMENT").toNumber().intValue(),8);

        JSInt8Array i8 = new JSInt8Array(context,8);
        for (byte i=0; i<8; i++) i8.set(i,i);
        JSFloat64Array array2 = new JSFloat64Array(i8);
        for (int i=0; i<8; i++)
            assertEquals(array2.get(i),Double.valueOf(i));

        List<Integer> ai = Arrays.asList(0,1,2,3,4,5,6,7);
        JSFloat64Array array3 = new JSFloat64Array(context,ai);
        context.property("array2",array2);
        context.property("array3",array3);
        assertEquals(array2.isEqual(array3),context.evaluateScript("array2==array3").toBoolean());
        assertEquals(array2,array3);
        assertThat(array3.size(),is(8));
        assertThat(array3.get(0),is(0.0));
        assertThat(array3.get(1),is(1.0));

        JSFloat64Array ab = new JSFloat64Array(context,8);
        for (int i=0; i<8; i++) ab.set(i,(double)i);

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSFloat64Array array4 = new JSFloat64Array(ab.buffer(),32,2);
        assertThat(array4.size(),is(2));
        assertThat(array4.get(0),is(4.0));
        assertThat(array4.get(1),is(5.0));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSFloat64Array array5 = new JSFloat64Array(ab.buffer(),48);
        assertThat(array5.size(),is(2));
        assertThat(array5.get(0),is(6.0));
        assertThat(array5.get(1),is(7.0));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSFloat64Array array6 = new JSFloat64Array(ab.buffer());
        assertThat(array6.size(),is(8));
        assertThat(array6.get(0),is(0.0));
        assertThat(array6.get(1),is(1.0));
        assertThat(ab.buffer().byteLength(),is(array6.byteLength()));

        JSFloat64Array array7 = new JSFloat64Array(context,Arrays.asList(5,4,3,2,1));
        JSFloat64Array array8 = array7.subarray(0,2);
        assertThat(array8.size(),is(2));
        assertThat(array8.get(0),is(5.0));
        assertThat(array8.get(1),is(4.0));

        JSFloat64Array array9 = array7.subarray(2);
        assertThat(array9.size(),is(3));
        assertThat(array9.get(0),is(3.0));
        assertThat(array9.get(1),is(2.0));
        assertThat(array9.get(2),is(1.0));

        context.evaluateScript("var Float64a = new Float64Array(10);");
        assertEquals(context.property("Float64a").toObject().getClass(),JSFloat64Array.class);

        List<Double> list = new JSFloat64Array(context, Arrays.asList(1,2,3,4,5));

        boolean exception = false;
        try {
            list.add(6.0);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Object[] toarray = list.toArray();
        assertEquals(toarray[0],1.0);
        assertEquals(toarray[1],2.0);
        assertEquals(toarray[2],3.0);

        assertThat(list.get(0),is(1.0));
        assertThat(list.get(1),is(2.0));
        assertThat(list.get(2),is(3.0));

        assertThat(list.size(),is(5));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        List<Double> list2 = new JSFloat64Array(context,0);
        assertFalse(list.isEmpty());
        assertTrue(list2.isEmpty());

        assertTrue(list.contains(1.0));
        assertTrue(list.contains(2.0));
        assertTrue(list.contains(2.0));
        assertFalse(list.contains(6.0));

        int i = 0;
        for (Iterator<Double> it = list.iterator(); it.hasNext(); i++) {
            Double next = it.next();
            assertTrue(list.contains(next));
        }
        assertThat(i,is(list.size()));

        Double[] arr1 = new Double[5];
        Double[] arr2 = list.toArray(arr1);
        assertArrayEquals(arr2,new Double[] {1.0,2.0,3.0,4.0,5.0});

        exception = false;
        try {
            list.remove(2);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        exception = false;
        try {
            list.remove(2.0);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Collection<Double> collection = new ArrayList<>();
        collection.add(2.0);
        collection.add(3.0);
        collection.add(4.0);
        Collection<Double> collection2 = new ArrayList<>(collection);
        collection2.add(25.0);
        assertTrue(list.containsAll(collection));
        assertFalse(list.containsAll(collection2));

        exception = false;
        try {
            list.addAll(collection);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        exception = false;
        try {
            list.removeAll(collection);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        exception = false;
        try {
            list.retainAll(collection);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        exception = false;
        try {
            list.clear();
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        exception = false;
        try {
            list.clear();
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Double last1;
        try {
            list.set(10, 10.0);
            last1 = 0.0;
        } catch (IndexOutOfBoundsException e) {
            last1 = list.set(1, 20.0);
        }
        assertEquals(Double.valueOf(2), last1);
        assertEquals(Double.valueOf(20), list.get(1));

        exception = false;
        try {
            list.add(1, 30.0);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        exception = false;
        try {
            list.remove(4);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        list = new JSFloat64Array(context,Arrays.asList(0,1,2,3,0,1,2,3));
        assertThat(list.indexOf(0.0),is(0));
        assertThat(list.indexOf(1.0),is(1));
        assertThat(list.indexOf(2.0),is(2));

        assertThat(list.lastIndexOf(0.0),is(4));
        assertThat(list.lastIndexOf(1.0),is(5));
        assertThat(list.lastIndexOf(2.0),is(6));

        ListIterator<Double> it = list.listIterator();
        it.next();
        it.set(100.0);
        assertEquals(Double.valueOf(100), list.get(0));

        exception = false;
        try {
            it.add(200.0);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        ListIterator<Double> it2 = list.listIterator(1);
        assertEquals(Double.valueOf(1), it2.next());

        assertEquals(list.subList(1, 4),Arrays.asList(1.0,2.0,3.0));

        exception = false;
        try {
            list.subList(-1,0);
        } catch (IndexOutOfBoundsException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }
        exception = false;
        try {
            list.subList(100,101);
        } catch (IndexOutOfBoundsException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }
        exception = false;
        try {
            list.subList(3,2);
        } catch (IndexOutOfBoundsException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }
    }
}