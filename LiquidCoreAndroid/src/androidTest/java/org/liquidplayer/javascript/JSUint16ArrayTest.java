//
// JSUint16ArrayTest.java
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

public class JSUint16ArrayTest {
    private JSContext context;

    @Before
    public void setUp() throws Exception {
        context = new JSContext();
    }

    public void setUp(JSContext context) {
        this.context = context;
    }

    @Test
    public void testJSUint16Array() throws Exception {
        JSUint16Array array = new JSUint16Array(context,8);
        assertThat(array.byteLength(),is(8*2));
        assertThat(array.byteOffset(),is(0));
        assertTrue(JSTypedArray.isTypedArray(array));
        assertEquals(array.property("BYTES_PER_ELEMENT").toNumber().intValue(),2);

        JSUint16Array i8 = new JSUint16Array(context,8);
        for (short i=0; i<8; i++) i8.set(i,i);
        JSUint16Array array2 = new JSUint16Array(i8);
        for (int i=0; i<8; i++)
            assertEquals(array2.get(i),Short.valueOf((short)i));

        List<Integer> ai = Arrays.asList(0,1,2,3,4,5,6,7);
        JSUint16Array array3 = new JSUint16Array(context,ai);
        context.property("array2",array2);
        context.property("array3",array3);
        assertEquals(array2.isEqual(array3),context.evaluateScript("array2==array3").toBoolean());
        assertEquals(array2,array3);
        assertThat(array3.size(),is(8));
        assertThat(array3.get(0),is((short)0));
        assertThat(array3.get(1),is((short)1));

        JSUint16Array ab = new JSUint16Array(context,8);
        for (int i=0; i<8; i++) ab.set(i,(short)i);

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSUint16Array array4 = new JSUint16Array(ab.buffer(),8,2);
        assertThat(array4.size(),is(2));
        assertThat(array4.get(0),is((short)4));
        assertThat(array4.get(1),is((short)5));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSUint16Array array5 = new JSUint16Array(ab.buffer(),12);
        assertThat(array5.size(),is(2));
        assertThat(array5.get(0),is((short)6));
        assertThat(array5.get(1),is((short)7));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSUint16Array array6 = new JSUint16Array(ab.buffer());
        assertThat(array6.size(),is(8));
        assertThat(array6.get(0),is((short)0));
        assertThat(array6.get(1),is((short)1));
        assertThat(ab.buffer().byteLength(),is(array6.byteLength()));

        JSUint16Array array7 = new JSUint16Array(context,Arrays.asList(5,4,3,2,1));
        JSUint16Array array8 = array7.subarray(0,2);
        assertThat(array8.size(),is(2));
        assertThat(array8.get(0),is((short)5));
        assertThat(array8.get(1),is((short)4));

        JSUint16Array array9 = array7.subarray(2);
        assertThat(array9.size(),is(3));
        assertThat(array9.get(0),is((short)3));
        assertThat(array9.get(1),is((short)2));
        assertThat(array9.get(2),is((short)1));

        context.evaluateScript("var Uint16a = new Uint16Array(10);");
        assertEquals(context.property("Uint16a").toObject().getClass(),JSUint16Array.class);

        List<Short> list = new JSUint16Array(context, Arrays.asList(1,2,3,4,5));

        boolean exception = false;
        try {
            list.add((short)6);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Object[] toarray = list.toArray();
        assertEquals(toarray[0],(short)1);
        assertEquals(toarray[1],(short)2);
        assertEquals(toarray[2],(short)3);

        assertThat(list.get(0),is((short)1));
        assertThat(list.get(1),is((short)2));
        assertThat(list.get(2),is((short)3));

        assertThat(list.size(),is(5));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        List<Short> list2 = new JSUint16Array(context,0);
        assertFalse(list.isEmpty());
        assertTrue(list2.isEmpty());

        assertTrue(list.contains((short)1));
        assertTrue(list.contains((short)2));
        assertTrue(list.contains((short)2));
        assertFalse(list.contains((short)6));

        int i = 0;
        for (Iterator<Short> it = list.iterator(); it.hasNext(); i++) {
            Short next = it.next();
            assertTrue(list.contains(next));
        }
        assertThat(i,is(list.size()));

        Short[] arr1 = new Short[5];
        Short[] arr2 = list.toArray(arr1);
        assertArrayEquals(arr2,new Short[] {(short)1,(short)2,(short)3,(short)4,(short)5});

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
            list.remove((short)2);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Collection<Short> collection = new ArrayList<>();
        collection.add((short)2);
        collection.add((short)3);
        collection.add((short)4);
        Collection<Short> collection2 = new ArrayList<>(collection);
        collection2.add((short)25);
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

        Short last1;
        try {
            list.set(10, (short)10);
            last1 = (short)0;
        } catch (IndexOutOfBoundsException e) {
            last1 = list.set(1, (short)20);
        }
        assertTrue(last1.equals((short)2));
        assertTrue(list.get(1).equals((short)20));

        exception = false;
        try {
            list.add(1, (short)30);
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

        list = new JSUint16Array(context,Arrays.asList(0,1,2,3,0,1,2,3));
        assertThat(list.indexOf((short)0),is(0));
        assertThat(list.indexOf((short)1),is(1));
        assertThat(list.indexOf((short)2),is(2));

        assertThat(list.lastIndexOf((short)0),is(4));
        assertThat(list.lastIndexOf((short)1),is(5));
        assertThat(list.lastIndexOf((short)2),is(6));

        ListIterator<Short> it = list.listIterator();
        it.next();
        it.set((short)100);
        assertTrue(list.get(0).equals((short)100));

        exception = false;
        try {
            it.add((short)20);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        ListIterator<Short> it2 = list.listIterator(1);
        assertTrue(it2.next().equals((short)1));

        assertEquals(list.subList(1, 4),Arrays.asList((short)1,(short)2,(short)3));

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