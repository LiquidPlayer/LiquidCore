//
// JSUint8ArrayTest.java
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

public class JSUint8ArrayTest {

    private JSContext context;

    @Before
    public void setUp() throws Exception {
        context = new JSContext();
    }

    public void setUp(JSContext context) {
        this.context = context;
    }

    @Test
    public void testJSUint8Array() throws Exception {
        JSUint8Array array = new JSUint8Array(context,8);
        assertThat(array.byteLength(),is(8));
        assertThat(array.byteOffset(),is(0));
        assertTrue(JSTypedArray.isTypedArray(array));
        assertEquals(array.property("BYTES_PER_ELEMENT").toNumber().intValue(),1);

        JSUint8Array i8 = new JSUint8Array(context,8);
        for (byte i=0; i<8; i++) i8.set(i,i);
        JSUint8Array array2 = new JSUint8Array(i8);
        for (int i=0; i<8; i++)
            assertEquals(array2.get(i),Byte.valueOf((byte)i));

        List<Integer> ai = Arrays.asList(0,1,2,3,4,5,6,7);
        JSUint8Array array3 = new JSUint8Array(context,ai);
        context.property("array2",array2);
        context.property("array3",array3);
        assertEquals(array2.isEqual(array3),context.evaluateScript("array2==array3").toBoolean());
        assertEquals(array2,array3);
        assertThat(array3.size(),is(8));
        assertThat(array3.get(0),is((byte)0));
        assertThat(array3.get(1),is((byte)1));

        JSUint8Array ab = new JSUint8Array(context,8);
        for (int i=0; i<8; i++) ab.set(i,(byte)i);

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSUint8Array array4 = new JSUint8Array(ab.buffer(),4,2);
        assertThat(array4.size(),is(2));
        assertThat(array4.get(0),is((byte)4));
        assertThat(array4.get(1),is((byte)5));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSUint8Array array5 = new JSUint8Array(ab.buffer(),6);
        assertThat(array5.size(),is(2));
        assertThat(array5.get(0),is((byte)6));
        assertThat(array5.get(1),is((byte)7));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSUint8Array array6 = new JSUint8Array(ab.buffer());
        assertThat(array6.size(),is(8));
        assertThat(array6.get(0),is((byte)0));
        assertThat(array6.get(1),is((byte)1));
        assertThat(ab.buffer().byteLength(),is(array6.byteLength()));

        JSUint8Array array7 = new JSUint8Array(context,Arrays.asList(5,4,3,2,1));
        JSUint8Array array8 = array7.subarray(0,2);
        assertThat(array8.size(),is(2));
        assertThat(array8.get(0),is((byte)5));
        assertThat(array8.get(1),is((byte)4));

        JSUint8Array array9 = array7.subarray(2);
        assertThat(array9.size(),is(3));
        assertThat(array9.get(0),is((byte)3));
        assertThat(array9.get(1),is((byte)2));
        assertThat(array9.get(2),is((byte)1));

        context.evaluateScript("var uint8a = new Uint8Array(10);");
        assertEquals(context.property("uint8a").toObject().getClass(),JSUint8Array.class);

        List<Byte> list = new JSUint8Array(context, Arrays.asList(1,2,3,4,5));

        boolean exception = false;
        try {
            list.add((byte)6);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Object[] toarray = list.toArray();
        assertEquals(toarray[0],(byte)1);
        assertEquals(toarray[1],(byte)2);
        assertEquals(toarray[2],(byte)3);

        assertThat(list.get(0),is((byte)1));
        assertThat(list.get(1),is((byte)2));
        assertThat(list.get(2),is((byte)3));

        assertThat(list.size(),is(5));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        List<Byte> list2 = new JSUint8Array(context,0);
        assertFalse(list.isEmpty());
        assertTrue(list2.isEmpty());

        assertTrue(list.contains((byte)1));
        assertTrue(list.contains((byte)2));
        assertTrue(list.contains((byte)2));
        assertFalse(list.contains((byte)6));

        int i = 0;
        for (Iterator<Byte> it = list.iterator(); it.hasNext(); i++) {
            Byte next = it.next();
            assertTrue(list.contains(next));
        }
        assertThat(i,is(list.size()));

        Byte[] arr1 = new Byte[5];
        Byte[] arr2 = list.toArray(arr1);
        assertArrayEquals(arr2,new Byte[] {(byte)1,(byte)2,(byte)3,(byte)4,(byte)5});

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
            list.remove((byte)2);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Collection<Byte> collection = new ArrayList<>();
        collection.add((byte)2);
        collection.add((byte)3);
        collection.add((byte)4);
        Collection<Byte> collection2 = new ArrayList<>(collection);
        collection2.add((byte)25);
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

        Byte last1;
        try {
            list.set(10, (byte)10);
            last1 = (byte)0;
        } catch (IndexOutOfBoundsException e) {
            last1 = list.set(1, (byte)20);
        }
        assertTrue(last1.equals((byte)2));
        assertTrue(list.get(1).equals((byte)20));

        exception = false;
        try {
            list.add(1, (byte)30);
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

        list = new JSUint8Array(context,Arrays.asList(0,1,2,3,0,1,2,3));
        assertThat(list.indexOf((byte)0),is(0));
        assertThat(list.indexOf((byte)1),is(1));
        assertThat(list.indexOf((byte)2),is(2));

        assertThat(list.lastIndexOf((byte)0),is(4));
        assertThat(list.lastIndexOf((byte)1),is(5));
        assertThat(list.lastIndexOf((byte)2),is(6));

        ListIterator<Byte> it = list.listIterator();
        it.next();
        it.set((byte)100);
        assertTrue(list.get(0).equals((byte)100));

        exception = false;
        try {
            it.add((byte)20);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        ListIterator<Byte> it2 = list.listIterator(1);
        assertTrue(it2.next().equals((byte)1));

        assertEquals(list.subList(1, 4),Arrays.asList((byte)1,(byte)2,(byte)3));

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