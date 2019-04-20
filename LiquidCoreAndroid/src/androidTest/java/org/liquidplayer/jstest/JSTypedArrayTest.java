/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.jstest;

import org.junit.Test;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSException;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSTypedArray;
import org.liquidplayer.javascript.JSValue;

import static org.junit.Assert.*;

public class JSTypedArrayTest {

    @Test
    public void testJSTypedArray() {
        JSContext context = new JSContext();
        testJSTypedArray(context);
    }

    public void testJSTypedArray(JSContext context) {
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