package org.liquidplayer.javascript;

import org.junit.Test;

import static org.junit.Assert.*;

public class JSTypedArrayTest {

    @Test
    public void testJSTypedArray() throws Exception {
        JSContext context = new JSContext();
        testJSTypedArray(context);
    }

    public void testJSTypedArray(JSContext context) throws Exception {
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