package org.liquidplayer.nodedroid;

import org.junit.Test;
import org.liquidplayer.v8.JSContext;

import static org.junit.Assert.*;

public class NodeTest {

    @Test
    public void testNewInstance() throws Exception {

        Node.getInstance();
        Thread.sleep(3000);
        Node.exitNode();
        Thread.sleep(3000);
        assertTrue(true);
    }

    @Test
    public void testJSContext() throws Exception {
        System.loadLibrary("node");
        System.loadLibrary("nodedroid");
        JSContext ctx = new JSContext();
        ctx.evaluateScript("'Hello' + ', World!'");
        assertTrue(true);
    }
}