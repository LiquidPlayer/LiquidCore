/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import org.junit.Test;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSErrorTest {

    @Test
    public void TestJSErrorAndJSException() throws Exception {
        JSContext context = new JSContext();
        TestJSErrorAndJSException(context);
    }

    public void TestJSErrorAndJSException(JSContext context) throws Exception {

        JSError error = new JSError(context, "This is an error message");
        assertThat(error.message(),is("This is an error message"));
        assertThat(error.name(),is("Error"));

        JSError error2 = new JSError(context, "Message2");
        assertThat(error2.message(),is("Message2"));
        assertThat(error2.name(),is("Error"));

        JSError error3 = new JSError(context);
        assertThat(error3.message(),is("Error"));
        assertThat(error3.name(),is("Error"));

        JSFunction fail = new JSFunction(context, "_fail", new String[] {},
                "var undef; var foo = undef.accessme;",
                "fail.js", 10) {
        };
        try {
            fail.call();
            assertFalse(true); // should not get here
        } catch (JSException e) {
            JSError error4 = e.getError();
            assertThat(error4.message(),is("Cannot read property 'accessme' of undefined"));
            assertThat(e.getMessage(),is("Cannot read property 'accessme' of undefined"));
            assertThat(e.toString(),is("TypeError: Cannot read property 'accessme' of undefined"));
            assertThat(error4.name(),is("TypeError"));
            assertThat(e.name(),is("TypeError"));
            assertTrue(error4.stack().contains("at _fail (fail.js:11:"));
            assertTrue(e.stack().contains("at _fail (fail.js:11:"));
        }

        try {
            context.property("_error_",error);
            context.evaluateScript("throw _error_;","main.js",1);
            assertFalse(true); // should not get here
        } catch (JSException e) {
            JSError error4 = e.getError();
            assertThat(error4.message(),is("This is an error message"));
            assertThat(e.getMessage(),is("This is an error message"));
            assertThat(e.toString(),is("Error: This is an error message"));
            assertThat(error4.name(),is("Error"));
            assertThat(e.name(),is("Error"));
        }

        try {
            throw new JSException(error);
        } catch (JSException e) {
            JSError error5 = e.getError();
            assertThat(error5.message(),is("This is an error message"));
            assertThat(e.getMessage(),is("This is an error message"));
            assertThat(e.toString(),is("Error: This is an error message"));
            assertThat(error5.name(),is("Error"));
            assertThat(e.name(),is("Error"));
        }

        try {
            throw new JSException(context,"Another exception");
        } catch (JSException e) {
            JSError error5 = e.getError();
            assertThat(error5.message(),is("Another exception"));
            assertThat(e.getMessage(),is("Another exception"));
            assertThat(e.toString(),is("Error: Another exception"));
            assertThat(error5.name(),is("Error"));
            assertThat(e.name(),is("Error"));
        }

        try {
            throw new JSException(context,null);
        } catch (JSException e) {
            assertNotNull(e.getError());
            assertThat(e.getMessage(),is("Error"));
            assertThat(e.toString(),is("Error: Error"));
            assertThat(e.name(),is("Error"));
        }
    }

    @org.junit.After
    public void shutDown() {
        Runtime.getRuntime().gc();
    }
}