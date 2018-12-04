/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;

import static org.hamcrest.Matchers.is;
import static org.junit.Assert.*;

public class JSInt32ArrayTest {
    private JSContext context;

    @Before
    public void setUp() throws Exception {
        context = new JSContext();
    }

    public void setUp(JSContext context) {
        this.context = context;
    }

    @Test
    public void testJSInt32Array() throws Exception {
        JSInt32Array array = new JSInt32Array(context,8);
        assertThat(array.byteLength(),is(8*4));
        assertThat(array.byteOffset(),is(0));
        assertTrue(array.isTypedArray());
        assertEquals(array.property("BYTES_PER_ELEMENT").toNumber().intValue(),4);

        JSInt32Array i8 = new JSInt32Array(context,8);
        for (int i=0; i<8; i++) i8.set(i,i);
        JSInt32Array array2 = new JSInt32Array(i8);
        for (int i=0; i<8; i++)
            assertEquals(array2.get(i),Integer.valueOf(i));

        List<Integer> ai = Arrays.asList(0,1,2,3,4,5,6,7);
        JSInt32Array array3 = new JSInt32Array(context,ai);
        context.property("array2",array2);
        context.property("array3",array3);
        assertEquals(array2.isEqual(array3),context.evaluateScript("array2==array3").toBoolean());
        assertEquals(array2,array3);
        assertThat(array3.size(),is(8));
        assertThat(array3.get(0),is(0));
        assertThat(array3.get(1),is(1));

        JSInt32Array ab = new JSInt32Array(context,8);
        for (int i=0; i<8; i++) ab.set(i,i);

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSInt32Array array4 = new JSInt32Array(ab.buffer(),16,2);
        assertThat(array4.size(),is(2));
        assertThat(array4.get(0),is(4));
        assertThat(array4.get(1),is(5));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSInt32Array array5 = new JSInt32Array(ab.buffer(),24);
        assertThat(array5.size(),is(2));
        assertThat(array5.get(0),is(6));
        assertThat(array5.get(1),is(7));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSInt32Array array6 = new JSInt32Array(ab.buffer());
        assertThat(array6.size(),is(8));
        assertThat(array6.get(0),is(0));
        assertThat(array6.get(1),is(1));
        assertThat(ab.buffer().byteLength(),is(array6.byteLength()));

        JSInt32Array array7 = new JSInt32Array(context,Arrays.asList(5,4,3,2,1));
        JSInt32Array array8 = array7.subarray(0,2);
        assertThat(array8.size(),is(2));
        assertThat(array8.get(0),is(5));
        assertThat(array8.get(1),is(4));

        JSInt32Array array9 = array7.subarray(2);
        assertThat(array9.size(),is(3));
        assertThat(array9.get(0),is(3));
        assertThat(array9.get(1),is(2));
        assertThat(array9.get(2),is(1));

        context.evaluateScript("var Int32a = new Int32Array(10);");
        assertEquals(context.property("Int32a").toObject().getClass(),JSInt32Array.class);

        List<Integer> list = new JSInt32Array(context, Arrays.asList(1,2,3,4,5));

        boolean exception = false;
        try {
            list.add(6);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Object[] toarray = list.toArray();
        assertEquals(toarray[0],1);
        assertEquals(toarray[1],2);
        assertEquals(toarray[2],3);

        assertThat(list.get(0),is(1));
        assertThat(list.get(1),is(2));
        assertThat(list.get(2),is(3));

        assertThat(list.size(),is(5));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        List<Integer> list2 = new JSInt32Array(context,0);
        assertFalse(list.isEmpty());
        assertTrue(list2.isEmpty());

        assertTrue(list.contains(1));
        assertTrue(list.contains(2));
        assertTrue(list.contains(2));
        assertFalse(list.contains(6));

        int i = 0;
        for (Iterator<Integer> it = list.iterator(); it.hasNext(); i++) {
            Integer next = it.next();
            assertTrue(list.contains(next));
        }
        assertThat(i,is(list.size()));

        Integer[] arr1 = new Integer[5];
        Integer[] arr2 = list.toArray(arr1);
        assertArrayEquals(arr2,new Integer[] {1,2,3,4,5});

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
            list.remove(2);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Collection<Integer> collection = new ArrayList<>();
        collection.add(2);
        collection.add(3);
        collection.add(4);
        Collection<Integer> collection2 = new ArrayList<>(collection);
        collection2.add(25);
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

        Integer last1;
        try {
            list.set(10, 10);
            last1 = 0;
        } catch (IndexOutOfBoundsException e) {
            last1 = list.set(1, 20);
        }
        assertTrue(last1.equals(2));
        assertTrue(list.get(1).equals(20));

        exception = false;
        try {
            list.add(1, 30);
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

        list = new JSInt32Array(context,Arrays.asList(0,1,2,3,0,1,2,3));
        assertThat(list.indexOf(0),is(0));
        assertThat(list.indexOf(1),is(1));
        assertThat(list.indexOf(2),is(2));

        assertThat(list.lastIndexOf(0),is(4));
        assertThat(list.lastIndexOf(1),is(5));
        assertThat(list.lastIndexOf(2),is(6));

        ListIterator<Integer> it = list.listIterator();
        it.next();
        it.set(100);
        assertTrue(list.get(0).equals(100));

        exception = false;
        try {
            it.add(20);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        ListIterator<Integer> it2 = list.listIterator(1);
        assertTrue(it2.next().equals(1));

        assertEquals(list.subList(1, 4),Arrays.asList(1,2,3));

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