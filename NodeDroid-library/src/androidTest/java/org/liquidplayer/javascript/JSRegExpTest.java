package org.liquidplayer.javascript;

import org.junit.Test;

import static org.junit.Assert.*;

public class JSRegExpTest {
    @Test
    public void testJSRegExp() throws Exception {
        JSContext context = new JSContext();
        testJSRegExp(context);
    }

    public void testJSRegExp(JSContext context) throws Exception {
        JSRegExp regExp = new JSRegExp(context,"quick\\s(brown).+?(jumps)","ig");
        JSRegExp.ExecResult result = regExp.exec("The Quick Brown Fox Jumps Over The Lazy Dog");
        assertEquals(result.get(0),"Quick Brown Fox Jumps");
        assertEquals(result.get(1),"Brown");
        assertEquals(result.get(2),"Jumps");
        assertEquals(result.index().intValue(),4);
        assertEquals(result.input(),"The Quick Brown Fox Jumps Over The Lazy Dog");

        assertEquals(regExp.lastIndex().intValue(),25);
        assertEquals(regExp.ignoreCase(),true);
        assertEquals(regExp.global(),true);
        assertEquals(regExp.multiline(),false);
        assertEquals(regExp.source(),"quick\\s(brown).+?(jumps)");

        JSRegExp regExp2 = new JSRegExp(context,"quick\\s(brown).+?(jumps)");
        assertEquals(regExp2.ignoreCase(),false);
        assertEquals(regExp2.global(),false);
        assertEquals(regExp2.multiline(),false);

        String str = "hello world!";
        assertTrue(new JSRegExp(context,"^hello").test(str));
    }
}