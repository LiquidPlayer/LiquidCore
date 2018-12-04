/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
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

        assertFalse(new JSValue(context).isTypedArray());
    }
}