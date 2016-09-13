package org.liquidplayer.v8;

import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSFloat32ArrayTest {

    private JSContext context;

    @Before
    public void setUp() throws Exception {
        context = new JSContext();
    }

    public void setUp(JSContext context) {
        this.context = context;
    }

    @Test
    public void testJSFloat32Array() throws Exception {
        JSFloat32Array array = new JSFloat32Array(context,8);

        assertThat(array.byteLength(),is(8*4));
        assertThat(array.byteOffset(),is(0));
        assertTrue(JSTypedArray.isTypedArray(array));
        assertEquals(array.property("BYTES_PER_ELEMENT").toNumber().intValue(),4);
        android.util.Log.d("Test", "a");

        JSInt8Array i8 = new JSInt8Array(context,8);
        for (byte i=0; i<8; i++) i8.set(i,i);
        JSFloat32Array array2 = new JSFloat32Array(i8);
        for (int i=0; i<8; i++)
            assertEquals(array2.get(i),Float.valueOf(i));
        android.util.Log.d("Test", "b");

        List<Integer> ai = Arrays.asList(0,1,2,3,4,5,6,7);
        JSFloat32Array array3 = new JSFloat32Array(context,ai);
        context.property("array2",array2);
        context.property("array3",array3);
        assertEquals(array2.isEqual(array3),context.evaluateScript("array2==array3").toBoolean());
        assertEquals(array2,array3);
        assertThat(array3.size(),is(8));
        assertThat(array3.get(0),is(0f));
        assertThat(array3.get(1),is(1f));
        android.util.Log.d("Test", "c");

        JSFloat32Array ab = new JSFloat32Array(context,8);
        for (int i=0; i<8; i++) ab.set(i,(float)i);

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSFloat32Array array4 = new JSFloat32Array(ab.buffer(),16,2);
        assertThat(array4.size(),is(2));
        assertThat(array4.get(0),is(4f));
        assertThat(array4.get(1),is(5f));
        android.util.Log.d("Test", "d");

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSFloat32Array array5 = new JSFloat32Array(ab.buffer(),24);
        assertThat(array5.size(),is(2));
        assertThat(array5.get(0),is(6f));
        assertThat(array5.get(1),is(7f));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        JSFloat32Array array6 = new JSFloat32Array(ab.buffer());
        assertThat(array6.size(),is(8));
        assertThat(array6.get(0),is(0f));
        assertThat(array6.get(1),is(1f));
        assertThat(ab.buffer().byteLength(),is(array6.byteLength()));
        android.util.Log.d("Test", "e");

        JSFloat32Array array7 = new JSFloat32Array(context,Arrays.asList(5,4,3,2,1));
        JSFloat32Array array8 = array7.subarray(0,2);
        assertThat(array8.size(),is(2));
        assertThat(array8.get(0),is(5f));
        assertThat(array8.get(1),is(4f));

        JSFloat32Array array9 = array7.subarray(2);
        assertThat(array9.size(),is(3));
        assertThat(array9.get(0),is(3f));
        assertThat(array9.get(1),is(2f));
        assertThat(array9.get(2),is(1f));

        android.util.Log.d("Test", "f");

        context.evaluateScript("var float32a = new Float32Array(10);");
        assertEquals(context.property("float32a").toObject().getClass(),JSFloat32Array.class);

        List<Float> list = new JSFloat32Array(context, Arrays.asList(1,2,3,4,5));

        boolean exception = false;
        try {
            list.add(6f);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Object[] toarray = list.toArray();
        assertEquals(toarray[0],1f);
        assertEquals(toarray[1],2f);
        assertEquals(toarray[2],3f);

        assertThat(list.get(0),is(1f));
        assertThat(list.get(1),is(2f));
        assertThat(list.get(2),is(3f));

        assertThat(list.size(),is(5));

        @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
        List<Float> list2 = new JSFloat32Array(context,0);
        assertFalse(list.isEmpty());
        assertTrue(list2.isEmpty());

        assertTrue(list.contains(1f));
        assertTrue(list.contains(2f));
        assertTrue(list.contains(2f));
        assertFalse(list.contains(6f));

        int i = 0;
        for (Iterator<Float> it = list.iterator(); it.hasNext(); i++) {
            android.util.Log.d("Test", "i = " + i);
            Float next = it.next();
            assertTrue(list.contains(next));
        }
        assertThat(i,is(list.size()));

        android.util.Log.d("Test", "Calling new Float");
        Float[] arr1 = new Float[5];
        android.util.Log.d("Test", "Calling toArray");
        Float[] arr2 = list.toArray(arr1);
        android.util.Log.d("Test", "testing for equality");
        assertArrayEquals(arr2,new Float[] {1f,2f,3f,4f,5f});
        android.util.Log.d("Test", "got thru all of that");

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
            list.remove(2f);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        Collection<Float> collection = new ArrayList<>();
        collection.add(2f);
        collection.add(3f);
        collection.add(4f);
        Collection<Float> collection2 = new ArrayList<>(collection);
        collection2.add(25f);
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

        Float last1;
        try {
            list.set(10, 10f);
            last1 = 0f;
        } catch (IndexOutOfBoundsException e) {
            last1 = list.set(1, 20f);
        }
        assertTrue(last1.equals(2f));
        assertTrue(list.get(1).equals(20f));

        exception = false;
        try {
            list.add(1, 30f);
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

        list = new JSFloat32Array(context,Arrays.asList(0,1,2,3,0,1,2,3));
        assertThat(list.indexOf(0f),is(0));
        assertThat(list.indexOf(1f),is(1));
        assertThat(list.indexOf(2f),is(2));

        assertThat(list.lastIndexOf(0f),is(4));
        assertThat(list.lastIndexOf(1f),is(5));
        assertThat(list.lastIndexOf(2f),is(6));

        ListIterator<Float> it = list.listIterator();
        it.next();
        it.set(100f);
        assertTrue(list.get(0).equals(100f));

        exception = false;
        try {
            it.add(200f);
        } catch (UnsupportedOperationException e) {
            exception = true;
        } finally {
            assertThat(exception,is(true));
        }

        ListIterator<Float> it2 = list.listIterator(1);
        assertTrue(it2.next().equals(1f));

        assertEquals(list.subList(1, 4),Arrays.asList(1f,2f,3f));

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

        android.util.Log.d("Test", "all done");
    }
}