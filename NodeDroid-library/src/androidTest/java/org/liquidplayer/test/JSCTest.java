package org.liquidplayer.test;

import org.junit.Test;

import static org.junit.Assert.*;

/**
 * Created by Eric on 11/24/16.
 */
public class JSCTest {

    @Test
    public void testJavaScriptCoreBridge() throws Exception {
        JSC jsc = new JSC();
        assertEquals(0,jsc.testAPI());
    }

    @Test
    public void testJavaScriptCoreMiniDOM() throws Exception {
        JSC jsc = new JSC();
        assertEquals(0,jsc.testMinidom());
    }

}