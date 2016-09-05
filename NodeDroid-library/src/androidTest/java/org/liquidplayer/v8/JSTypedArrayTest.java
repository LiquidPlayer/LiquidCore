package org.liquidplayer.v8;

import org.junit.Test;

import static org.junit.Assert.*;

public class JSTypedArrayTest {

    @Test
    public void testJSTypedArray() throws Exception {
        JSContext context = new JSContext();
        boolean exception = false;
        try {
            JSTypedArray.from(new JSObject(context));
        } catch (JSException e) {
            exception = true;
        } finally {
            assertTrue(exception);
        }

        assertFalse(JSTypedArray.isTypedArray(new JSValue(context)));
    }
}