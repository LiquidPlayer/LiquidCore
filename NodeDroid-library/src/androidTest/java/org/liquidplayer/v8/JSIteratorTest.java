package org.liquidplayer.v8;

import org.junit.Test;

import java.util.Arrays;

import static org.junit.Assert.*;

public class JSIteratorTest {

    @Test
    public void testJSIterator() throws Exception {
        JSContext context = new JSContext();
        JSArray<Integer> array = new JSArray<>(context, Arrays.asList(0,1,2,3,4), Integer.class);
        JSArray.KeysIterator iterator = array.keys();

        int i=0;
        for(; iterator.hasNext(); i++) {
            assertEquals(iterator.next(),array.get(i));
            boolean exception = false;
            try {
                iterator.remove();
            } catch (UnsupportedOperationException e) {
                exception = true;
            } finally {
                assertTrue(exception);
            }
        }
        assertEquals(i,array.size());
    }
}